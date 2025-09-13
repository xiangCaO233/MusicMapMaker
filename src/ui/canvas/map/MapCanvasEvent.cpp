#include <QKeyEvent>
#include <QObject>
#include <canvas/map/MapCanvas.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/LayerManager.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/project/MProject.hpp>

void MapCanvas::resizeEvent(QResizeEvent *e) {
    GLCanvas::resizeEvent(e);
    // 更新工具系统空间索引世界尺寸
    // static_cast<MapLayerManager *>(dataloop()->layermanager())
    //     ->get_tool_system()
    //     ->update_world_boundbox({{0, 0}, {width(), height()}});
    if (map) {
        // 更新轨道布局
        info<MapCanvasInfo>()->update_trackLayout();
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
