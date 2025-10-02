#ifndef MMM_TIMELINESYSTEM_HPP
#define MMM_TIMELINESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TimeLineComponents.hpp>
#include <ecs/component/TimingComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <ecs/system/time2pixel/maintrack/EffectedTimeConverter.hpp>
#include <ecs/system/time2pixel/maintrack/LinearTimeConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <iterator>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>
#include <tool/ToolInteractionState.hpp>
#include <vector>

class TimeLineSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter, ILayer* layer,
                const ToolInteractionState* interactState,
                RenderDataBuffer& buffer, bool is_preview = false) const {
        // 绘制timing
        // 生成物件的网格组件
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;
        const auto& mousepos = info->realTimeInfo.mousePos;
        const auto pconfig = info->editorInfo.map->project()->cfg();
        // 获取轨道布局信息
        glm::vec4 all_tracks_rect;
        if (is_preview) {
            auto& main_track_layout = info->editorInfo.track_layout;
            // 移动轨道布局到预览区
            auto xpos = main_track_layout.x + main_track_layout.z;
            all_tracks_rect = {xpos, 0,
                               info->baseInfo.canvasSize.width() - xpos,
                               info->baseInfo.canvasSize.height()};
        } else {
            all_tracks_rect = info->editorInfo.track_layout;
        }

        // 生成拍
        // 获取当前视口所有可见拍
        auto& beats = core.get_beat_group();

        // 字符指令缓冲
        std::vector<PrimitiveCommand> str_cmds;
        // 预留(4分拍+1分拍策略) * 12空间给字符串绘制指令缓冲区
        str_cmds.reserve(beats.size() * 5 * 12);

        int64_t next_beat_timestamp = INT64_MAX;
        for (auto it = beats.rbegin(); it != beats.rend(); ++it) {
            const auto& current_beat_entity = *it;
            if (!is_preview &&
                !registry.all_of<InMaintrackComponent>(current_beat_entity))
                // 非预览绘制且没在主轨道内,跳过绘制拍
                continue;
            const auto& [time] =
                registry.get<TimeComponent>(current_beat_entity);
            if (it != beats.rend() && std::next(it) != beats.rend()) {
                auto next_beat_entity = *std::next(it);
                const auto& [next_beat_time] =
                    registry.get<TimeComponent>(next_beat_entity);
                // 时间间隔过短,跳过绘制
                if (std::abs(int(next_beat_time) - int(time)) < 6) {
                    continue;
                }
                const auto& next_y = converter.timeToPixel(
                    next_beat_time,
                    realtime_info.current_time_info.presentation_canvas_time,
                    info);
            }
            const auto& [divisors, beat_length, beat_index, is_manual] =
                registry.get<BeatComponent>(current_beat_entity);
            // 转换时间线所处y位置
            const auto y = converter.timeToPixel(
                time, realtime_info.current_time_info.presentation_canvas_time,
                info);
            const auto y_endbeat = converter.timeToPixel(
                time + beat_length,
                realtime_info.current_time_info.presentation_canvas_time, info);

            // 可见性检测
            if (y < all_tracks_rect.y ||
                y > all_tracks_rect.y + all_tracks_rect.w) {
            } else {
                // 生成拍头线
                PrimitiveCommand cmd;
                cmd.cmdType = CommandType::PRIMITIVE;
                cmd.primitive = PrimitiveType::QUAD;
                if (is_preview) {
                    cmd.baseInfo.pos = {all_tracks_rect.x, y - 1};
                    cmd.baseInfo.size = {all_tracks_rect.z, 2};
                    cmd.baseInfo.color = {1, 1, 1, .5f};
                } else {
                    cmd.baseInfo.pos = {0, y - 3};
                    cmd.baseInfo.size = {all_tracks_rect.x + all_tracks_rect.z,
                                         6};
                    cmd.baseInfo.color = {
                        1, 1, 1, pconfig->canvas_config.beatline_alpha};
                    // 不是在绘制预览则计算好字符指令
                    // 拍头时间戳字符串
                    auto head_time = QString("%1").arg(time).toStdU32String();
                    auto head_time_metric = layer->stringMetrics(
                        "ComicShannsMono Nerd Font", 24, head_time);
                    auto head_time_pos = glm::vec2{
                        all_tracks_rect.x - head_time_metric.x - 4,
                        cmd.baseInfo.pos.y - head_time_metric.y / 2.f + 18};
                    auto head_time_cmds = layer->generateStringCommands(
                        "ComicShannsMono Nerd Font", 24, head_time,
                        head_time_pos,
                        {1.f, 1.f, 1.f, pconfig->canvas_config.beatline_alpha});
                    str_cmds.insert(str_cmds.end(), head_time_cmds.begin(),
                                    head_time_cmds.end());

                    // 本拍拍号字符串
                    auto index_str =
                        QString("#%1").arg(beat_index).toStdU32String();
                    auto index_metric = layer->stringMetrics(
                        "ComicShannsMono Nerd Font", 32, index_str);
                    auto index_str_pos =
                        glm::vec2{4, y - index_metric.y / 2.f + 24};
                    auto index_cmds = layer->generateStringCommands(
                        "ComicShannsMono Nerd Font", 32, index_str,
                        index_str_pos,
                        {.85f, .35f, .35f,
                         pconfig->canvas_config.beatline_alpha});
                    str_cmds.insert(str_cmds.end(), index_cmds.begin(),
                                    index_cmds.end());

                    // 本拍分拍策略字符串(画在拍号上面)
                    auto div_stratergy =
                        QString("1/%1").arg(divisors).toStdU32String();
                    auto div_metric = layer->stringMetrics(
                        "ComicShannsMono Nerd Font", 24, div_stratergy);
                    auto div_str_pos = glm::vec2{
                        index_str_pos.x, index_str_pos.y - div_metric.y - 4};
                    auto div_cmds = layer->generateStringCommands(
                        "ComicShannsMono Nerd Font", 24, div_stratergy,
                        div_str_pos,
                        {.85f, .85f, .85f,
                         pconfig->canvas_config.beatline_alpha});
                    str_cmds.insert(str_cmds.end(), div_cmds.begin(),
                                    div_cmds.end());
                }

                buffer.add_PrimitiveCommand(cmd);
            }
            if (is_preview || std::abs(y_endbeat - y) < 2) {
                // 拍像素距离过小或预览绘制:跳过分拍线绘制
                // 提前更新“下一拍”的时间戳
                next_beat_timestamp = time;
                continue;
            }

            auto mousepos = interactState->getMouseState().current_pos;
            auto divtime = beat_length / double(divisors);
            // 生成分拍线 (加入检查逻辑)
            if (divisors > 1) {
                auto divtheme =
                    info->editorInfo.skin->get_divisors_color_theme(divisors);

                for (uint32_t j = divisors - 1; j > 0; --j) {
                    const int64_t subdivision_timestamp = static_cast<int64_t>(
                        static_cast<double>(time) +
                        beat_length / static_cast<double>(divisors) *
                            static_cast<double>(j));

                    // 使用上一轮记录的 next_beat_timestamp
                    if (subdivision_timestamp >= next_beat_timestamp) {
                        continue;
                    }

                    auto theme = divtheme[j - 1];
                    auto divy =
                        converter.timeToPixel(subdivision_timestamp,
                                              realtime_info.current_time_info
                                                  .presentation_canvas_time,
                                              info);
                    auto predivy =
                        converter.timeToPixel(subdivision_timestamp - divtime,
                                              realtime_info.current_time_info
                                                  .presentation_canvas_time,
                                              info);
                    // 可见性检测
                    if (divy < all_tracks_rect.y ||
                        divy > all_tracks_rect.y + all_tracks_rect.w)
                        continue;

                    PrimitiveCommand sub_cmd;
                    sub_cmd.cmdType = CommandType::PRIMITIVE;
                    sub_cmd.primitive = PrimitiveType::QUAD;
                    sub_cmd.baseInfo.pos = {all_tracks_rect.x,
                                            divy - theme.second / 2.f};
                    sub_cmd.baseInfo.size = {all_tracks_rect.z, theme.second};
                    auto color = theme.first;
                    sub_cmd.baseInfo.color = {
                        color.red() / 256.f, color.green() / 256.f,
                        color.blue() / 256.f,
                        pconfig->canvas_config.beatline_alpha};
                    buffer.add_PrimitiveCommand(sub_cmd);

                    // 当前一小分拍的跨度
                    auto div_distance = std::abs(divy - predivy);

                    // 当前拍线监测区域
                    auto current_div_area = glm::vec4{
                        glm::vec2{sub_cmd.baseInfo.pos.x,
                                  divy - div_distance / 2.f},
                        glm::vec2{sub_cmd.baseInfo.size.x, div_distance}};
                    if (divisors <= 24 ||
                        mutil::checkPointInRect(mousepos, current_div_area)) {
                        // 分拍线时间字符串
                        auto div_time = QString("%1")
                                            .arg(subdivision_timestamp)
                                            .toStdU32String();
                        auto div_time_metric = layer->stringMetrics(
                            "ComicShannsMono Nerd Font", 16, div_time);
                        auto div_time_pos =
                            glm::vec2{all_tracks_rect.x - div_time_metric.x - 4,
                                      sub_cmd.baseInfo.pos.y -
                                          div_time_metric.y / 2.f + 12};
                        auto div_time_cmds = layer->generateStringCommands(
                            "ComicShannsMono Nerd Font", 16, div_time,
                            div_time_pos, sub_cmd.baseInfo.color);
                        str_cmds.insert(str_cmds.end(), div_time_cmds.begin(),
                                        div_time_cmds.end());
                    }
                }
            }

            // 更新“下一拍”的时间戳
            next_beat_timestamp = time;
        }

        if (is_preview) {
            // 预览下绘制完整的所有timing线(可覆盖拍线)
            // 遍历所有可见的timing实体
            auto view = registry.view<TimingComponent>();
            for (const auto& e : view) {
                const auto& [time] = registry.get<TimeComponent>(e);
                const auto& [bpm, beat_length, is_base_timing] =
                    registry.get<TimingComponent>(e);
                // 转换时间线所处y位置
                const auto y = converter.timeToPixel(
                    time,
                    realtime_info.current_time_info.presentation_canvas_time,
                    info);
                // 可见性检测
                if (y < all_tracks_rect.y ||
                    y > all_tracks_rect.y + all_tracks_rect.w)
                    continue;

                PrimitiveCommand cmd;
                cmd.cmdType = CommandType::PRIMITIVE;
                cmd.primitive = PrimitiveType::QUAD;
                if (is_base_timing) {
                    // 是参考bpm的timing-绘制红线
                    auto bpmstr = QString("bpm=%1").arg(bpm, 'f', 2);
                    auto bpmstru32 = bpmstr.toStdU32String();
                    cmd.baseInfo.pos = {all_tracks_rect.x, y - 1};
                    cmd.baseInfo.size = {all_tracks_rect.z, 2};
                    cmd.baseInfo.color = {1, 0, 0, .5f};

                } else {
                    // 是变速timing-绘制绿线
                    auto speedstr =
                        QString("%1x").arg(-100.0 / beat_length, 'f', 2);
                    auto speedstru32 = speedstr.toStdU32String();
                    cmd.baseInfo.pos = {all_tracks_rect.x, y - 1};
                    cmd.baseInfo.size = {all_tracks_rect.z, 2};
                    cmd.baseInfo.color = {0, 1, 0, .5f};
                }
                buffer.add_PrimitiveCommand(cmd);
            }
        }

        // 一次提交所有字符指令
        for (auto& charcmd : str_cmds) {
            buffer.add_PrimitiveCommand(charcmd);
        }
    }
};

#endif  // MMM_TIMELINESYSTEM_HPP
