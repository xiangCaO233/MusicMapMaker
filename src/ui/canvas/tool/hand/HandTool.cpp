#include <tool/hand/HandTool.hpp>

// 析构SelectTool
HandTool::~HandTool() = default;

// 从Canvas转发过来的事件
void HandTool::mousePressEvent(QMouseEvent* e) {
    // 调用基类处理物件上的交互
    BaseEditTool::mousePressEvent(e);

    // 如果没有悬浮在物件上，则开始框选
    if (!tool_interaction_state()->getHover().has_value()) {
        tool_command_queue()->push(StartSelectCommand{
            {glm::vec2{e->pos().x(), e->pos().y()}, e->modifiers()},
            e->button(),
            e->modifiers().testFlag(Qt::ControlModifier)});
    }
}

void HandTool::mouseMoveEvent(QMouseEvent* e) {
    BaseTool::mouseMoveEvent(e);
    // 发送更新选中区域命令
    tool_command_queue()->push(UpdateSelectAreaCommand{e->buttons()});
}

void HandTool::mouseReleaseEvent(QMouseEvent* e) {
    BaseEditTool::mouseReleaseEvent(e);
    tool_command_queue()->push(EndSelectCommand{e->button()});
}

// SelectTool 的专属实现
void HandTool::handleSingleObjectDragStart(
    QMouseEvent* e, const std::optional<MeshPartInfo>& hoveredInfo) {
    // SelectTool 的职责是选择和移动。当拖拽单个物件时：
    // 发送一个明确的“拖拽选中项”命令，而不是可能触发编辑的 StartDragCommand
    tool_command_queue()->push(StartDragSelectionCommand{
        glm::vec2{e->pos().x(), e->pos().y()},
        e->modifiers(),
        {hoveredInfo.has_value() ? hoveredInfo.value() : MeshPartInfo{}},
        {hoveredInfo.value().source_entity}});
}
