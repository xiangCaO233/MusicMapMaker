#include <QApplication>
#include <ecs/system/ToolSystem.hpp>
#include <info/NotePart.hpp>
#include <map/MapCanvas.hpp>
#include <tool/note/NoteTool.hpp>

// 析构NoteTool
NoteTool::~NoteTool() = default;

// 从Canvas转发过来的事件
void NoteTool::mousePressEvent(QMouseEvent* event) {}

void NoteTool::mouseMoveEvent(QMouseEvent* event) {
    auto modifiers = QApplication::keyboardModifiers();
    // auto a = modifiers.testFlag(Qt::ShiftModifier);
    auto mousepos = event->pos();
    auto candidate = tool_system()->query({mousepos.x(), mousepos.y()});
    if (candidate.has_value()) {
        auto note =
            canvas()->get_map()->note_set().get_note(candidate.value()->handle);
        qDebug() << "hover at note:";
        qDebug() << note->toString();
    }
}

void NoteTool::mouseReleaseEvent(QMouseEvent* event) {}

void NoteTool::keyPressEvent(QKeyEvent* e) {}

void NoteTool::keyReleaseEvent(QKeyEvent* e) {}
