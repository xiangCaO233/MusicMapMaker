#ifndef MMM_INTERACTSYSTEM_HPP
#define MMM_INTERACTSYSTEM_HPP

#include <deque>
#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
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
        // 绘制左键选择框
        auto selection_state = toolInteractionState->getSelectionState();
        auto left_it = selection_state.per_button_areas.find(Qt::LeftButton);
        if (left_it != selection_state.per_button_areas.end()) {
            for (const auto& current_mouseleft_select_area : left_it->second) {
                PrimitiveCommand mouseLeftCmd;
                mouseLeftCmd.cmdType = CommandType::PRIMITIVE;
                mouseLeftCmd.primitive = PrimitiveType::QUAD;
                mouseLeftCmd.baseInfo.pos = {current_mouseleft_select_area.x,
                                             current_mouseleft_select_area.y};
                mouseLeftCmd.baseInfo.size = {current_mouseleft_select_area.z,
                                              current_mouseleft_select_area.w};
                mouseLeftCmd.baseInfo.color = {0, 0, 0, 1};
                buffer.add_PrimitiveCommand(mouseLeftCmd);
            }
        }
    }
};

#endif  // MMM_INTERACTSYSTEM_HPP
