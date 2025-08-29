#ifndef MMM_TIMELINESYSTEM_HPP
#define MMM_TIMELINESYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TimingComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/LinearTimeConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>

class TimeLineSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const LinearTimeConverter& converter, ILayer* layer,
                RenderDataBuffer& buffer) const {
        // 绘制timing
        // 生成物件的网格组件
        auto& registry = core.ecs_registry();
        const auto& realtime_info = info->realTimeInfo;
        // 获取轨道布局信息
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        // 遍历所有可见的timing实体
        auto view = registry.view<TimingComponent>();
        for (const auto& e : view) {
            const auto& [time] = registry.get<TimeComponent>(e);
            const auto& [bpm, beat_length, is_base_timing] =
                registry.get<TimingComponent>(e);
            // 转换时间线所处y位置
            const auto y = converter.timeToPixel(
                time, realtime_info.presentation_canvas_time);

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
    }
};

#endif  // MMM_TIMELINESYSTEM_HPP
