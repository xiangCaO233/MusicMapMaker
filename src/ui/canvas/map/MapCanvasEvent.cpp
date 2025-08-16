#include <QKeyEvent>
#include <QObject>
#include <canvas/map/MapCanvas.hpp>
#include <info/MapCanvasInfo.hpp>
#include <mmm/project/MProject.hpp>

void MapCanvas::resizeEvent(QResizeEvent *e) {
    GLCanvas::resizeEvent(e);
    if (map) {
        auto mapinfo = info<MapCanvasInfo>();
        auto &layout = map->project()->cfg()->canvas_config.canvas_layout;
        auto canvas_size = size();
        auto x = float(canvas_size.width()) * layout.w;
        auto y = float(canvas_size.height()) * layout.x;
        auto w = float(canvas_size.width()) * layout.y - x;
        auto h = float(canvas_size.height()) * layout.z - y;
        mapinfo->editorInfo.track_layout = {x, y, w, h};
        update_sharedInfo();
    }
}

void MapCanvas::keyPressEvent(QKeyEvent *e) {
    GLCanvas::keyPressEvent(e);
    current_tool->keyPressEvent(e);
}

void MapCanvas::keyReleaseEvent(QKeyEvent *e) {
    GLCanvas::keyReleaseEvent(e);
    current_tool->keyReleaseEvent(e);
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e) {
    GLCanvas::mouseMoveEvent(e);
    current_tool->mouseMoveEvent(e);
}

void MapCanvas::mousePressEvent(QMouseEvent *e) {
    GLCanvas::mousePressEvent(e);
    current_tool->mousePressEvent(e);
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *e) {
    GLCanvas::mouseReleaseEvent(e);
    current_tool->mouseReleaseEvent(e);
}
