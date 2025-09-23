#ifndef MMM_INTERACTSYSTEM_HPP
#define MMM_INTERACTSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/time2pixel/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class InteractSystem {
   public:
    // 初始化现实时钟
    InteractSystem() = default;

    void update(const ECSCore& core, const MapCanvasInfo* info, ILayer* layer,
                ToolInteractionState* toolInteractionState,
                RenderDataBuffer& buffer) const {
        auto selection_state = toolInteractionState->getSelectionState();
        auto skin = info->editorInfo.skin;
        auto border_width =
            info->editorInfo.project_config->canvas_config.select_border_width;
        auto left_border_texture =
            skin->get_selected_border_texture(SelectBorderDirection::LEFT);
        auto top_border_texture =
            skin->get_selected_border_texture(SelectBorderDirection::TOP);
        auto right_border_texture =
            skin->get_selected_border_texture(SelectBorderDirection::RIGHT);
        auto bottom_border_texture =
            skin->get_selected_border_texture(SelectBorderDirection::BOTTOM);

        // 绘制左键选择框
        auto left_it = selection_state.per_button_areas.find(Qt::LeftButton);
        if (left_it != selection_state.per_button_areas.end()) {
            for (const auto& area : left_it->second) {
                // 规范化矩形，处理负数宽高
                float norm_x = (area.z < 0) ? area.x + area.z : area.x;
                float norm_y = (area.w < 0) ? area.y + area.w : area.y;
                float norm_w = std::abs(area.z);
                float norm_h = std::abs(area.w);

                // 如果宽高过小，可以不绘制，避免出现渲染问题
                if (norm_w < border_width * 2 || norm_h < border_width * 2) {
                    continue;
                }
                // 绘制半透明的粉红色填充背景
                // 这一步必须在绘制边框之前，以确保背景在下层
                PrimitiveCommand fillCmd;
                fillCmd.cmdType = CommandType::PRIMITIVE;
                fillCmd.primitive = PrimitiveType::QUAD;
                fillCmd.baseInfo.pos = {norm_x, norm_y};
                fillCmd.baseInfo.size = {norm_w, norm_h};

                // 设置颜色为半透明粉红 (R, G, B, Alpha) - 你可以调整这些值
                fillCmd.baseInfo.color = glm::vec4(1.f, .7f, .8f, .18f);

                // 按照要求，不设置 textureInfo，渲染器将使用 baseInfo.color

                buffer.add_PrimitiveCommand(fillCmd);

                // 使用规范化后的值进行绘制

                // 左侧边框
                PrimitiveCommand leftBorderCmd;
                leftBorderCmd.cmdType = CommandType::PRIMITIVE;
                leftBorderCmd.primitive = PrimitiveType::QUAD;
                leftBorderCmd.baseInfo.pos = {norm_x, norm_y};
                leftBorderCmd.baseInfo.size = {border_width,
                                               norm_h};  // 使用完整高度
                leftBorderCmd.texturesInfo = {left_border_texture,
                                              TexAlignMode::CENTER,
                                              TexScaleMode::FORCE_FILL};
                buffer.add_PrimitiveCommand(leftBorderCmd);

                // 右侧边框
                PrimitiveCommand rightBorderCmd;
                rightBorderCmd.cmdType = CommandType::PRIMITIVE;
                rightBorderCmd.primitive = PrimitiveType::QUAD;
                // 从 (右边缘 - 边框宽度) 的位置开始绘制
                rightBorderCmd.baseInfo.pos = {norm_x + norm_w - border_width,
                                               norm_y};
                rightBorderCmd.baseInfo.size = {border_width,
                                                norm_h};  // 使用完整高度
                rightBorderCmd.texturesInfo = {right_border_texture,
                                               TexAlignMode::CENTER,
                                               TexScaleMode::FORCE_FILL};
                buffer.add_PrimitiveCommand(rightBorderCmd);

                // 顶部边框
                PrimitiveCommand topBorderCmd;
                topBorderCmd.cmdType = CommandType::PRIMITIVE;
                topBorderCmd.primitive = PrimitiveType::QUAD;
                // 顶部边框绘制在左右边框“之间”，避免边角重叠
                topBorderCmd.baseInfo.pos = {norm_x + border_width, norm_y};
                // 宽度减去左右两个边框的宽度
                topBorderCmd.baseInfo.size = {norm_w - 2 * border_width,
                                              border_width};
                topBorderCmd.texturesInfo = {top_border_texture,
                                             TexAlignMode::CENTER,
                                             TexScaleMode::FORCE_FILL};
                buffer.add_PrimitiveCommand(topBorderCmd);

                // 底部边框
                PrimitiveCommand bottomBorderCmd;
                bottomBorderCmd.cmdType = CommandType::PRIMITIVE;
                bottomBorderCmd.primitive = PrimitiveType::QUAD;
                bottomBorderCmd.baseInfo.pos = {norm_x + border_width,
                                                norm_y + norm_h - border_width};
                // 宽度减去左右两个边框的宽度
                bottomBorderCmd.baseInfo.size = {norm_w - 2 * border_width,
                                                 border_width};
                bottomBorderCmd.texturesInfo = {bottom_border_texture,
                                                TexAlignMode::CENTER,
                                                TexScaleMode::FORCE_FILL};
                buffer.add_PrimitiveCommand(bottomBorderCmd);
            }
        }
    }
};

#endif  // MMM_INTERACTSYSTEM_HPP
