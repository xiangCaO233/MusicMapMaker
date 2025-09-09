#ifndef MMM_TIMELINESYSTEM_HPP
#define MMM_TIMELINESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TimeLineComponents.hpp>
#include <ecs/component/TimingComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/LinearTimeConverter.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class TimeLineSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const LinearTimeConverter& converter,
                const TimePixelConverter& converter2, ILayer* layer,
                RenderDataBuffer& buffer) const {
        // 绘制timing
        // 生成物件的网格组件
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;
        const auto& mousepos = info->realTimeInfo.mousePos;
        // 获取轨道布局信息
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        // 遍历所有可见的timing实体
        auto view = registry.view<TimingComponent>();
        for (const auto& e : view) {
            const auto& [time] = registry.get<TimeComponent>(e);
            const auto& [bpm, beat_length, is_base_timing] =
                registry.get<TimingComponent>(e);
            // 转换时间线所处y位置
            const auto y = converter2.timeToPixel(
                time, realtime_info.presentation_canvas_time, info);

            PrimitiveCommand cmd;
            cmd.cmdType = CommandType::PRIMITIVE;
            cmd.primitive = PrimitiveType::QUAD;
            if (is_base_timing) {
                // 是参考bpm的timing-绘制在轨道左侧(红线)
                auto bpmstr = QString("bpm=%1").arg(bpm, 'f', 2);
                auto bpmstru32 = bpmstr.toStdU32String();
                cmd.baseInfo.pos = {all_tracks_rect.x - 20 - 2, y - 2};
                cmd.baseInfo.size = {20, 4};
                cmd.baseInfo.color = {1, 0, 0, 1};

                // uint32_t xoffset{0};
                // for (const auto& character : bpmstru32) {
                //     // 获取字符纹理信息
                //     auto fontoption =
                //         layer->get("ComicShannsMono Nerd Font", 12,
                //         character);
                //     if (fontoption.has_value()) {
                //         const auto& charInfo = fontoption.value();
                //         const auto& charTexture = charInfo.character_texinfo;

                //         // 计算当前字符应该处于的位置
                //         glm::vec2 charpos = {all_tracks_rect.x - 9 * 12, y -
                //         2}; charpos.x += float(xoffset); charpos.y -=
                //         (charInfo.bearing.y); charpos.y += 6;
                //         // 提交渲染指令
                //         QuadCommand charcommand{
                //             {CommandType::QUAD,
                //              {charTexture, TexAlignMode::CENTER,
                //               TexScaleMode::CHARACTER}},
                //             {charpos,
                //              charTexture.origin_size,
                //              0.f,
                //              {1.f, 0.f, 0.f, 1.f},
                //              true},
                //             {charTexture.uv_offset},
                //         };
                //         buffer.add_QuadCommand(charcommand);
                //         xoffset += charInfo.xadvance / 64;
                //     }
                // }
            } else {
                // 是变速timing-绘制在轨道右侧(绿线)
                auto speedstr =
                    QString("%1x").arg(-100.0 / beat_length, 'f', 2);
                auto speedstru32 = speedstr.toStdU32String();
                cmd.baseInfo.pos = {all_tracks_rect.x + all_tracks_rect.z + 2,
                                    y - 2};
                cmd.baseInfo.size = {20, 4};
                cmd.baseInfo.color = {0, 1, 0, 1};

                // uint32_t xoffset{0};
                // for (const auto& character : speedstru32) {
                //     // 获取字符纹理信息
                //     auto fontoption =
                //         layer->get("ComicShannsMono Nerd Font", 12,
                //         character);
                //     if (fontoption.has_value()) {
                //         const auto& charInfo = fontoption.value();
                //         const auto& charTexture = charInfo.character_texinfo;

                //         // 计算当前字符应该处于的位置
                //         glm::vec2 charpos =
                //             cmd.baseInfo.pos + glm::vec2{20, -2};
                //         charpos.x += float(xoffset);
                //         charpos.y -= (charInfo.bearing.y);
                //         charpos.y += 6;
                //         // 提交渲染指令
                //         QuadCommand charcommand{
                //             {CommandType::QUAD,
                //              {charTexture, TexAlignMode::CENTER,
                //               TexScaleMode::CHARACTER}},
                //             {charpos,
                //              charTexture.origin_size,
                //              0.f,
                //              {0.f, 1.f, 0.f, 1.f},
                //              true},
                //             {charTexture.uv_offset},
                //         };
                //         buffer.add_QuadCommand(charcommand);
                //         xoffset += charInfo.xadvance / 64;
                //     }
                // }
            }
            buffer.add_PrimitiveCommand(cmd);
        }

        // 生成拍
        struct BeatRenderData {
            int64_t timestamp;
            uint32_t divisors;
            double beat_length;

            bool operator<(const BeatRenderData& other) const {
                return timestamp < other.timestamp;
            }
        };
        std::vector<BeatRenderData> sorted_beats;
        auto beatview = registry.view<TimeComponent, BeatComponent>();
        sorted_beats.reserve(beatview.size_hint());

        for (const auto& e : beatview) {
            const auto& [time] = registry.get<TimeComponent>(e);
            const auto& [divisors, beat_length] =
                registry.get<BeatComponent>(e);
            sorted_beats.push_back({time, divisors, beat_length});
        }
        std::sort(sorted_beats.begin(), sorted_beats.end());
        int64_t next_beat_timestamp = INT64_MAX;
        for (auto it = sorted_beats.rbegin(); it != sorted_beats.rend(); ++it) {
            const auto& current_beat = *it;
            // 转换时间线所处y位置
            const auto y = converter2.timeToPixel(
                current_beat.timestamp, realtime_info.presentation_canvas_time,
                info);
            // 生成拍头线
            PrimitiveCommand cmd;
            cmd.cmdType = CommandType::PRIMITIVE;
            cmd.primitive = PrimitiveType::QUAD;
            cmd.baseInfo.pos = {all_tracks_rect.x, y - 3};
            cmd.baseInfo.size = {all_tracks_rect.z, 6};
            cmd.baseInfo.color = {1, 1, 1, 1};
            buffer.add_PrimitiveCommand(cmd);
            // 生成分拍线 (加入检查逻辑)
            if (current_beat.divisors > 1) {
                auto divtheme = info->editorInfo.skin->get_divisors_color_theme(
                    current_beat.divisors);

                for (uint32_t j = 1; j < current_beat.divisors; ++j) {
                    const int64_t subdivision_timestamp = static_cast<int64_t>(
                        static_cast<double>(current_beat.timestamp) +
                        current_beat.beat_length /
                            static_cast<double>(current_beat.divisors) *
                            static_cast<double>(j));

                    // 使用上一轮记录的 next_beat_timestamp
                    if (subdivision_timestamp >= next_beat_timestamp) {
                        continue;
                    }

                    auto theme = divtheme[j - 1];
                    auto divy = converter2.timeToPixel(
                        subdivision_timestamp,
                        realtime_info.presentation_canvas_time, info);

                    PrimitiveCommand sub_cmd;
                    sub_cmd.cmdType = CommandType::PRIMITIVE;
                    sub_cmd.primitive = PrimitiveType::QUAD;
                    sub_cmd.baseInfo.pos = {all_tracks_rect.x,
                                            divy - theme.second / 2.f};
                    sub_cmd.baseInfo.size = {all_tracks_rect.z, theme.second};
                    auto color = theme.first;
                    sub_cmd.baseInfo.color = {
                        color.red() / 256.f, color.green() / 256.f,
                        color.blue() / 256.f, color.alpha() / 256.f};
                    buffer.add_PrimitiveCommand(sub_cmd);
                }
            }

            // 更新“下一拍”的时间戳
            next_beat_timestamp = current_beat.timestamp;
        }
        // for (const auto& e : beatview) {
        //     const auto& [time] = registry.get<TimeComponent>(e);
        //     const auto& [divisors, beat_length] =
        //         registry.get<BeatComponent>(e);
        //     // 转换时间线所处y位置
        //     const auto y = converter2.timeToPixel(
        //         time, realtime_info.presentation_canvas_time, info);
        //     // 生成拍头线
        //     PrimitiveCommand cmd;
        //     cmd.cmdType = CommandType::PRIMITIVE;
        //     cmd.primitive = PrimitiveType::QUAD;
        //     cmd.baseInfo.pos = {all_tracks_rect.x, y - 3};
        //     cmd.baseInfo.size = {all_tracks_rect.z, 6};
        //     cmd.baseInfo.color = {1, 1, 1, 1};
        //     buffer.add_PrimitiveCommand(cmd);

        //     auto divtheme =
        //         info->editorInfo.skin->get_divisors_color_theme(divisors);
        //     // 生成小节线
        //     for (int i{1}; i < divisors; ++i) {
        //         auto theme = divtheme[i - 1];
        //         auto divy = converter2.timeToPixel(
        //             int64_t(double(time) +
        //                     beat_length / double(divisors) * double(i)),
        //             realtime_info.presentation_canvas_time, info);

        //         PrimitiveCommand cmd;
        //         cmd.cmdType = CommandType::PRIMITIVE;
        //         cmd.primitive = PrimitiveType::QUAD;
        //         cmd.baseInfo.pos = {all_tracks_rect.x,
        //                             divy - theme.second / 2.f};
        //         cmd.baseInfo.size = {all_tracks_rect.z, theme.second};
        //         auto color = theme.first;
        //         cmd.baseInfo.color = {
        //             color.red() / 256.f, color.green() / 256.f,
        //             color.blue() / 256.f, color.alpha() / 256.f};
        //         buffer.add_PrimitiveCommand(cmd);
        //     }
        // }
    }
};

#endif  // MMM_TIMELINESYSTEM_HPP
