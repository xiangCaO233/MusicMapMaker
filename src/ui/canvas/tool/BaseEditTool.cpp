#include <map/MapCanvas.hpp>
#include <tool/BaseEditTool.hpp>

// 析构BaseEditTool
BaseEditTool::~BaseEditTool() {}

// 从Canvas转发过来的事件
void BaseEditTool::mousePressEvent(QMouseEvent* e) {
    BaseTool::mousePressEvent(e);
    auto hoveredinfo_opt = tool_interaction_state()->getHover();

    // 如果没有悬浮在任何物件上，则由子类处理（如创建或框选）
    if (!hoveredinfo_opt.has_value()) {
        return;
    }

    // 处理悬浮在物件上的情况
    auto selections = tool_interaction_state()->getSelection();
    auto buttons = e->buttons();
    auto hoveredinfo = hoveredinfo_opt.value();
    auto entity = hoveredinfo.source_entity;

    // if (!selections.contains(entity)) {
    //     // 立即选中此物件
    //     selections.insert(entity);
    // }

    // 右键标记删除
    if (buttons.testFlag(Qt::RightButton)) {
        // 如果是多选，则标记整个选区
        if (selections.size() > 1 && selections.contains(entity)) {
            tool_command_queue()->push(
                MarkDeleteCommand{hoveredinfo, selections});
        } else {  // 否则只标记当前这一个
            tool_command_queue()->push(
                MarkDeleteCommand{hoveredinfo, {entity}});
        }
        // 右键逻辑已处理完毕
        return;
    }

    // 左键拖拽多选物件
    if (buttons.testFlag(Qt::LeftButton)) {
        // 确保悬浮物件在选区内，且选区大于1
        if (selections.size() > 1 && selections.contains(entity)) {
            tool_command_queue()->push(
                StartDragSelectionCommand{glm::vec2{e->pos().x(), e->pos().y()},
                                          e->modifiers(), selections});
            return;  // 多选拖拽逻辑已处理完毕
        }

        // --- 委托逻辑: 左键拖拽单个物件 ---
        // 此时，一定是拖拽单个物件（无论它之前是否被选中）
        // 调用虚函数，让子类决定具体行为
        handleSingleObjectDragStart(e, hoveredinfo);
    }
}

void BaseEditTool::mouseReleaseEvent(QMouseEvent* e) {
    BaseTool::mouseReleaseEvent(e);
    auto button = e->button();
    auto pos = e->pos();

    // 结束拖拽
    if (button == Qt::LeftButton && !tool_interaction_state()
                                         ->getDragState()
                                         .dragged_entitiesWithRes.empty()) {
        tool_command_queue()->push(EndDragCommand{{pos.x(), pos.y()}});
        return;
    }

    // 确认删除
    if (button == Qt::RightButton) {
        auto hover_state = tool_interaction_state()->getHover();
        tool_command_queue()->push(ConfirmDeleteCommand{
            hover_state.has_value() &&
            tool_interaction_state()
                ->getDeleteMarkStates()
                .marked_entities.contains(hover_state->source_entity)});
        return;
    }
}
