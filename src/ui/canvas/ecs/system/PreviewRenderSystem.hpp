#ifndef MMM_PREVIEWRENDERSYSTEM_HPP
#define MMM_PREVIEWRENDERSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>

// 渲染预览虚影物件和即将删除的物件
class PreviewRenderSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter,
                RenderDataBuffer& buffer) const {
        // 渲染虚影和即将删除的物件
        auto& registry = core.ecs_registry();
        // const auto& realtime_info = info->realTimeInfo;

        // 遍历所有需要渲染的普通实体(除去虚影和即将删除的)
        auto view = registry.view<TimeComponent, TransformComponent_1,
                                  TransformComponent_2>();
        for (auto& e : view) {
            auto& [mesh] = view.get<TransformComponent_2>(e);
            // 排序网格
            std::sort(mesh.begin(), mesh.end(),
                      [](const TransformComponent_2::Quad& quad1,
                         const TransformComponent_2::Quad& quad2) {
                          return quad1.zIndex < quad2.zIndex;
                      });
            // 生成网格的渲染指令
            for (const auto& quad : mesh) {
                QuadCommand cmd;
                cmd.cmdType = CommandType::QUAD;
                cmd.baseInfo.pos = quad.pos;
                cmd.baseInfo.size = quad.size;
                cmd.texturesInfo.texture = quad.texture;
                // 虚影物件
                if (registry.all_of<GhostComponent>(e)) {
                    // 半透明效果
                    cmd.baseInfo.color.a = 0.35f;
                }
                // 即将删除物件

                if (registry.all_of<DeleteMarkComponent>(e)) {
                    // 红色滤镜半透明效果
                    cmd.baseInfo.color.g = 0.f;
                    cmd.baseInfo.color.b = 0.f;
                    cmd.baseInfo.color.a = 0.35f;
                }
                buffer.add_QuadCommand(cmd);
            }

            // 绘制物件精确时间字符串
            // const auto& [entity_y] = view.get<TransformComponent_1>(e);
            // const auto& [time] = view.get<TimeComponent>(e);
            // // 绘制当前时间字符串
            // auto timestr = QString::number(uint32_t(time));
            // auto timestru32 = timestr.toStdU32String();

            // uint32_t xoffset{0};

            // for (const auto& character : timestru32) {
            //     // 获取字符纹理信息
            //     auto fontoption =
            //         layer->get("ComicShannsMono Nerd Font", 12, character);
            //     if (fontoption.has_value()) {
            //         const auto& charInfo = fontoption.value();
            //         const auto& charTexture = charInfo.character_texinfo;

            //         // 计算当前字符应该处于的位置
            //         glm::vec2 charpos = mesh.begin()->pos +
            //         glm::vec2{4.f, 4.f}; charpos.x += float(xoffset);
            //         charpos.y -= (charInfo.bearing.y);
            //         charpos.y += 8;
            //         // 提交渲染指令
            //         RenderCommand charcommand(
            //             {{charpos,
            //               charTexture.origin_size,
            //               0.f,
            //               {1.f, 0.f, 1.f, 1.f},
            //               true},
            //              {charTexture.uv_offset},
            //              {charTexture, TexAlignMode::CENTER,
            //               TexScaleMode::CHARACTER}});
            //         buffer.push_back(charcommand);

            //         xoffset += charInfo.xadvance / 64;
            //     }
            // }
        }
    }
};

#endif  // MMM_PREVIEWRENDERSYSTEM_HPP
