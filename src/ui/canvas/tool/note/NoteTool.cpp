#include <QApplication>
#include <canvas/map/MapCanvas.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <tool/note/NoteTool.hpp>

// 析构NoteTool
NoteTool::~NoteTool() = default;

// 从Canvas转发过来的事件
void NoteTool::mousePressEvent(QMouseEvent* e) {
    // 首先调用基类，让它处理所有在物件上的通用交互
    BaseEditTool::mousePressEvent(e);
    if (get_pressArea() == MouseArea::EDIT) {
        // 如果鼠标没有悬浮在任何物件上，执行 NoteTool 职责：创建 Note
        if (!tool_interaction_state()->getHover().has_value()) {
            tool_command_queue()->push(ClearDragStateCommand{});

            if (e->button() == Qt::LeftButton) {
                if (e->modifiers().testFlag(Qt::ShiftModifier)) {
                    tool_command_queue()->push(
                        StartCreateNewCompositeNoteCommand{});
                } else {
                    tool_command_queue()->push(
                        StartCreateNewNormalNoteCommand{});
                }
            }
        }
    }
}

// 这是 NoteTool 的专属实现
void NoteTool::handleSingleObjectDragStart(
    QMouseEvent* e, const std::optional<MeshPartInfo>& hoveredInfo) {
    if (get_pressArea() == MouseArea::EDIT) {
        // 发送带有完整 hover 信息的 StartDragCommand，ECS
        // 系统可以据此进行移动或编辑
        tool_command_queue()->push(
            StartDragCommand{glm::vec2{e->pos().x(), e->pos().y()},
                             e->modifiers(), hoveredInfo.value()});
    }
}

void NoteTool::mouseMoveEvent(QMouseEvent* e) {
    BaseEditTool::mouseMoveEvent(e);
    if (get_pressArea() == MouseArea::EDIT) {
        // 根据是否正在创建新物件发送更新创建坐标指令
        if (!tool_interaction_state()->hasSelected()) {
            tool_command_queue()->push(UpdateCreateNodeCommand{});
        }
    }
}

void NoteTool::mouseReleaseEvent(QMouseEvent* e) {
    // 基类已经处理了拖拽结束和删除确认，这里只处理创建确认
    BaseEditTool::mouseReleaseEvent(e);
    if (get_pressArea() == MouseArea::EDIT) {
        if (e->button() == Qt::LeftButton &&
            tool_interaction_state()
                ->getDragState()
                .dragged_entitiesWithRes.empty()) {
            tool_command_queue()->push(ConfirmCreateNewNoteCommand{});
        }
    }
    clear_pressArea();
}
