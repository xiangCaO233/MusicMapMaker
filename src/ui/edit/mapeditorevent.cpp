#include <edit/mapeditor.h>

#include <canvas/map/MapCanvas.hpp>

// 向画布传输按键事件
void MapEditor::keyPressEvent(QKeyEvent *e) { canvas()->keyPressEvent(e); }

void MapEditor::keyReleaseEvent(QKeyEvent *e) { canvas()->keyReleaseEvent(e); }
