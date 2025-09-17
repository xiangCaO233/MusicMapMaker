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
    auto modifiers = QApplication::keyboardModifiers();
    auto hoveredinfo = tool_interaction_state()->getHover();
    auto& notes = canvas()->get_map()->note_set();

    if (hoveredinfo.has_value()) {
        auto selections = tool_interaction_state()->getSelection();
        auto info = hoveredinfo.value();

        // qDebug() << "当前帧空间索引:";
        // tool_system()->get_mesh_info_tree().print_tree();
        // qDebug() << "press Note:" << notes.get_note(info.handle)->toString();
        // qDebug() << "pressed note entity:"
        //          << static_cast<uint32_t>(info.source_entity);
        // qDebug() << "pressed entity valid?:"
        //          << tool_system()->get_registry().valid(info.source_entity);

        if (selections.size() > 1) {
            auto entity = hoveredinfo.value().source_entity;
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
            switch (hoveredinfo.value().part) {
                case NotePart::HEAD: {
                    // qDebug() << "发送开始拖拽普通物件事件";
                    tool_command_queue()->push(StartDragNormalNoteCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_HEAD: {
                    // qDebug() << "发送开始拖拽长条物件头事件";
                    tool_command_queue()->push(StartDragHoldHeadCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_BODY: {
                    // qDebug() << "发送开始拖拽长条物件身事件";
                    tool_command_queue()->push(StartDragHoldBodyCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_END: {
                    // qDebug() << "发送开始拖拽长条物件尾事件";
                    tool_command_queue()->push(StartDragHoldTailCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::HOLD_NODE: {
                    // qDebug() << "发送开始拖拽长条物件节点事件";
                    tool_command_queue()->push(StartDragHoldNodeCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::SLIDE_HEAD: {
                    // qDebug() << "发送开始拖拽滑键物件头事件";
                    tool_command_queue()->push(StartDragSlideHeadCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::SLIDE_BODY: {
                    // qDebug() << "发送开始拖拽滑键物件身事件";
                    tool_command_queue()->push(StartDragSlideBodyCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::SLIDE_END: {
                    // qDebug() << "发送开始拖拽滑键物件尾事件";
                    tool_command_queue()->push(StartDragSlideTailCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }
                case NotePart::SLIDE_NODE: {
                    // qDebug() << "发送开始拖拽长条物件节点事件";
                    tool_command_queue()->push(StartDragSlideNodeCommand{
                        glm::vec2{pos.x(), pos.y()}, modifiers,
                        hoveredinfo.value()});
                    break;
                }

                case NotePart::NONE: {
                    // 放置物件/shift防止长条
                    // qDebug() << "未发送任何拖拽事件";
                    // tool_command_queue()->push(
                    //     StartDragHoldTailCommand{glm::vec2{pos.x(), pos.y()},
                    //                              modifiers,
                    //                              *hoveredinfo.value()});
                    break;
                }
            }
        }
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
    auto pos = e->pos();
    tool_command_queue()->push(EndDragCommand{{pos.x(), pos.y()}});
    // qDebug() << "鼠标释放事件结束-位于qtui线程(已发送结束拖拽指令)";
}

void NoteTool::keyPressEvent(QKeyEvent* e) {}

void NoteTool::keyReleaseEvent(QKeyEvent* e) {}
