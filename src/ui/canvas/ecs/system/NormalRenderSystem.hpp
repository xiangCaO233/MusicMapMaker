#ifndef MMM_NORMALRENDERSYSTEM_HPP
#define MMM_NORMALRENDERSYSTEM_HPP

#include <algorithm>
#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <mmm/map/MMap.hpp>

class NormalRenderSystem {
   public:
    void update(
        const entt::registry& registry,
        std::unordered_map<entt::entity, GeneratedMesh>& generated_meshes,
        const MapCanvasInfo* info, const TimePixelConverter& converter,
        ILayer* layer, RenderDataBuffer& buffer) const {
        // 渲染普通物件
        // const auto& realtime_info = info->realTimeInfo;

        // 遍历所有需要渲染的普通实体(除去虚影和即将删除的)
        auto view =
            registry.view<TimeComponent, NoteComponent, TransformComponent>();
        for (auto& e : view) {
            auto& meshinfo = generated_meshes[e];
            auto& mesh = meshinfo.mesh;
            // 排序网格
            std::sort(mesh.begin(), mesh.end(),
                      [](const GeneratedMesh::Quad& quad1,
                         const GeneratedMesh::Quad& quad2) {
                          return quad1.zIndex < quad2.zIndex;
                      });
            // 生成网格的渲染指令
            for (const auto& quad : mesh) {
                PrimitiveCommand cmd;
                cmd.cmdType = CommandType::PRIMITIVE;
                cmd.baseInfo.pos = quad.pos;
                cmd.baseInfo.size = quad.size;
                cmd.texturesInfo.texture = quad.texture;

                // 整个网格的附加状态
                switch (meshinfo.state) {
                    case MeshState::GHOST: {
                        cmd.baseInfo.color = {1.f, 1.f, 1.f, .4f};
                        break;
                    }
                    case MeshState::MARKDELETE: {
                        cmd.baseInfo.color = {1.f, .1f, .1f, .8f};
                    }
                    default:
                        break;
                }

                // 当前部分的发光状态
                switch (quad.state) {
                    case PartState::GLOW: {
                        cmd.radiusInfo.radius_effect_param = 1.f;
                        cmd.radiusInfo.radius_effect = RadiusEffect::GLOWING;
                    }
                    default:
                        break;
                }

                buffer.add_PrimitiveCommand(cmd);
            }
            // 绘制物件精确时间字符串到鼠标旁边
            if (meshinfo.state != MeshState::NONE) {
                const auto& [track, uuid] = view.get<NoteComponent>(e);
                auto src_time =
                    info->editorInfo.map->note_set()
                        .get_note(
                            info->editorInfo.map->note_uuids().get_handle(uuid))
                        ->timestamp();
                auto des_time = converter.pixelToTime(
                    info->baseInfo.canvasSize.height() -
                        (info->realTimeInfo.mousePos.y()) -
                        info->baseInfo.canvasSize.height() *
                            info->baseInfo.judgeline_pos,
                    info->realTimeInfo.current_time_info
                        .presentation_canvas_time);

                // 绘制当前时间字符串

                auto timestr =
                    QString("%1->%2").arg(uint32_t(src_time)).arg(des_time);
                auto timestru32 = timestr.toStdU32String();
                glm::vec2 charpos = glm::vec2{info->realTimeInfo.mousePos.x(),
                                              info->realTimeInfo.mousePos.y()};
                // 字符串尺寸
                auto metric = layer->stringMetrics("ComicShannsMono NerdFont",
                                                   14, timestru32);
                auto strpos =
                    glm::vec2{charpos.x - metric.x / 2, charpos.y - metric.y};
                auto timestr_cmds = layer->generateStringCommands(
                    "ComicShannsMono Nerd Font", 14, timestru32,
                    {strpos.x, strpos.y + 8}, {1.f, 1.f, 1.f, 1.f});

                // 绘制框
                // PrimitiveCommand frameCmd;
                // frameCmd.cmdType = CommandType::PRIMITIVE;
                // frameCmd.baseInfo.pos = strpos;
                // frameCmd.baseInfo.size = metric;
                // frameCmd.baseInfo.color = {0.23f, .23f, .23f, .9f};
                // buffer.add_PrimitiveCommand(frameCmd);

                for (auto& cmd : timestr_cmds) {
                    buffer.add_PrimitiveCommand(cmd);
                }

                //     // uint32_t xoffset{0};

                //     // for (const auto& character : timestru32) {
                //     //     // 获取字符纹理信息
                //     //     auto fontoption = layer->get(
                //     //         "ComicShannsMono Nerd Font", 12, character);
                //     //     if (fontoption.has_value()) {
                //     //         const auto& charInfo = fontoption.value();
                //     //         const auto& charTexture =
                //     //             charInfo.character_texinfo;

                //     //         // 计算当前字符应该处于的位置
                //     //         glm::vec2 charpos =
                //     //             glm::vec2{info->realTimeInfo.mousePos.x(),
                //     //                       info->realTimeInfo.mousePos.y()}
                //     +
                //     //             glm::vec2{4.f, -4.f};
                //     //         charpos.x += float(xoffset) - 4 *
                //     //         timestru32.size(); charpos.y -=
                //     //         (charInfo.bearing.y);
                //     //         // 提交渲染指令
                //     //         PrimitiveCommand charcommand{
                //     //             {CommandType::PRIMITIVE,
                //     //              {charTexture, TexAlignMode::CENTER,
                //     //               TexScaleMode::CHARACTER}},
                //     //             {charpos,
                //     //              charTexture.origin_size,
                //     //              0.f,
                //     //              {.8f, 0.f, .8f, 1.f},
                //     //              true},
                //     //             {charTexture.uv_offset},
                //     //             PrimitiveType::QUAD};

                //     //         buffer.add_PrimitiveCommand(charcommand);

                //     //         xoffset += charInfo.xadvance / 64;
                //     //     }
                //     // }
                // }
            }
        }
    }
};

#endif  // MMM_NORMALRENDERSYSTEM_HPP
