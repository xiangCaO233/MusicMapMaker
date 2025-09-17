#include <QApplication>
#include <canvas/map/MapCanvas.hpp>
#include <ecs/system/ToolSystem.hpp>
#include <tool/note/NoteTool.hpp>

// 析构NoteTool
NoteTool::~NoteTool() = default;

// 从Canvas转发过来的事件
void NoteTool::mousePressEvent(QMouseEvent* e) {
    // qDebug() << "鼠标按下事件开始-位于qtui线程";
    //
    BaseTool::mousePressEvent(e);
    auto pos = e->pos();
    auto modifiers = e->modifiers();
    auto buttons = e->buttons();
    auto hoveredinfo = tool_interaction_state()->getHover();
    auto& notes = canvas()->get_map()->note_set();

    if (hoveredinfo.has_value()) {
        // 当前有悬浮物件
        auto selections = tool_interaction_state()->getSelection();
        auto info = hoveredinfo.value();

        if (selections.size() > 1) {
            // 多项操作
            // 若当前悬浮物件不在选中集合内/添加到选中集合
            auto entity = hoveredinfo.value().source_entity;
            // 是在选中了大量物件下的情况拖动了某一个物件
            if (!selections.contains(entity)) {
                // 悬浮位置的物件不在选中集合内
                // 添加拖动物件
                selections.insert(entity);
            }
            if (buttons.testFlag(Qt::LeftButton)) {
                // 发送拖拽选中内容命令
                tool_command_queue()->push(StartDragSelectionCommand{
                    glm::vec2{pos.x(), pos.y()}, modifiers, selections});
            } else if (buttons.testFlag(Qt::RightButton)) {
                // 右键按下-发送即将删除
                tool_command_queue()->push(
                    MarkDeleteCommand{hoveredinfo.value(), selections});
            }

        } else {
            if (buttons.testFlag(Qt::LeftButton)) {
                // 单一操作
                if (modifiers.testFlag(Qt::ShiftModifier)) {
                    // 按住shift拖动
                } else {
                    // 直接拖动
                    // 是在某个物件的某个部位开始按下的,根据拖拽部位发送不同拖拽开始命令
                    tool_command_queue()->push(
                        StartDragCommand{glm::vec2{pos.x(), pos.y()}, modifiers,
                                         hoveredinfo.value()});
                }
            } else if (buttons.testFlag(Qt::RightButton)) {
                // 右键按下-发送即将删除单个
                tool_command_queue()->push(MarkDeleteCommand{
                    hoveredinfo.value(), {hoveredinfo.value().source_entity}});
            }
        }
    } else {
        // 清除选中物件
    }

    // qDebug() << "鼠标按下事件结束-位于qtui线程";
}

void NoteTool::mouseMoveEvent(QMouseEvent* e) {
    BaseTool::mouseMoveEvent(e);
    // auto modifiers = QApplication::keyboardModifiers();
    // auto a = modifiers.testFlag(Qt::ShiftModifier);
    // auto mousepos = e->pos();
    // auto candidate = tool_system()->query({mousepos.x(), mousepos.y()});
    // if (candidate.has_value()) {
    //     auto note =
    //         canvas()->get_map()->note_set().get_note(candidate.value()->handle);
    //     qDebug() << "hover at note:";
    //     qDebug() << note->toString();
    // }
}

void NoteTool::mouseReleaseEvent(QMouseEvent* e) {
    // qDebug() << "鼠标释放事件开始-位于qtui线程";

    BaseTool::mouseReleaseEvent(e);
    auto button = e->button();
    auto pos = e->pos();
    if (button == Qt::LeftButton) {
        // 左键松开-发送结束拖拽命令
        tool_command_queue()->push(EndDragCommand{{pos.x(), pos.y()}});
    } else if (button == Qt::RightButton) {
        auto hover_state = tool_interaction_state()->getHover();
        // 右键松开-发送确认删除命令
        tool_command_queue()->push(ConfirmDeleteCommand{
            hover_state.has_value() &&
            tool_interaction_state()
                ->getDeleteMarkStates()
                .marked_entities.contains(hover_state->source_entity)});
    }
    // qDebug() << "鼠标释放事件结束-位于qtui线程(已发送结束拖拽指令)";
}

void NoteTool::keyPressEvent(QKeyEvent* e) {}

void NoteTool::keyReleaseEvent(QKeyEvent* e) {}
