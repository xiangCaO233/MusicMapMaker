#include <QApplication>
#include <tool/note/NoteTool.hpp>

// 析构NoteTool
NoteTool::~NoteTool() = default;

// 从Canvas转发过来的事件
void NoteTool::mousePressEvent(QMouseEvent* e) {
    //
    BaseTool::mousePressEvent(e);
    auto pos = e->pos();
    auto modifiers = QApplication::keyboardModifiers();
    auto hoveredinfo = tool_interaction_state()->getHover();
    if (hoveredinfo.has_value()) {
        auto selections = tool_interaction_state()->getSelection();
        if (selections.size() > 1) {
            auto entity = hoveredinfo.value()->source_entity;
            // 是在选中了大量物件下的情况拖动了某一个物件
            if (!selections.contains(entity)) {
                // 悬浮位置的物件不在选中集合内
                // 添加拖动物件
                selections.insert(entity);
            }

            // 发送拖拽选中内容命令
            tool_command_queue()->push(StartDragSelectionCommand{
                glm::vec2{pos.x(), pos.y()}, modifiers, selections});
        } else {
            // 是在某个物件的某个部位开始按下的,根据拖拽部位发送不同拖拽开始命令
            switch (hoveredinfo.value()->part) {
                case NotePart::HEAD: {
                    tool_command_queue()->push(StartDragNormalNoteCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        *hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_HEAD: {
                    tool_command_queue()->push(StartDragHoldHeadCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        *hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_BODY: {
                    tool_command_queue()->push(StartDragHoldBodyCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        *hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_END: {
                    tool_command_queue()->push(StartDragHoldTailCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        *hoveredinfo.value()});
                    break;
                }
                case NotePart::NONE:
                case NotePart::SLIDE_HEAD:
                case NotePart::SLIDE_BODY:
                case NotePart::SLIDE_END: {
                    // tool_command_queue()->push(
                    //     StartDragHoldTailCommand{glm::vec2{pos.x(), pos.y()},
                    //                              modifiers,
                    //                              *hoveredinfo.value()});
                    break;
                }
            }
        }
    }
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
    BaseTool::mouseReleaseEvent(e);
    auto pos = e->pos();
    tool_command_queue()->push(EndDragCommand{{pos.x(), pos.y()}});
}

void NoteTool::keyPressEvent(QKeyEvent* e) {}

void NoteTool::keyReleaseEvent(QKeyEvent* e) {}
