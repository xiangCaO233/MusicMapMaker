#include <tool/note/NoteTool.hpp>

NoteTool::NoteTool(MapCanvas* cvs) : BaseTool(cvs) {
    setType(EditToolType::NOTE);
}

// 析构NoteTool
NoteTool::~NoteTool() = default;

// 从Canvas转发过来的事件
void NoteTool::mousePressEvent(QMouseEvent* event) {}

void NoteTool::mouseMoveEvent(QMouseEvent* event) {}

void NoteTool::mouseReleaseEvent(QMouseEvent* event) {}

void NoteTool::keyPressEvent(QKeyEvent* e) {}

void NoteTool::keyReleaseEvent(QKeyEvent* e) {}
