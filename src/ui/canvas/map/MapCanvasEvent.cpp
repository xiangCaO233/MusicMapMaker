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

void MapCanvas::wheelEvent(QWheelEvent *e) {
    GLCanvas::wheelEvent(e);
    auto mapinfo = info<MapCanvasInfo>();
    // 先响应map滚动
    auto modifiers = e->modifiers();
    auto d = e->angleDelta();
    auto dy = d.y();

    if (map) {
        if (modifiers.testFlag(Qt::ShiftModifier)) {
            if (dy > 0) {
                mapinfo->realTimeInfo.current_time_info +=
                    (mapinfo->editorInfo.pageScrollStep * 3);
            }
            if (dy < 0) {
                mapinfo->realTimeInfo.current_time_info +=
                    -(mapinfo->editorInfo.pageScrollStep * 3);
            }
            audio_callback->set_playpos_for(
                map->base_metadata().main_audio_path.generic_string(),
                std::chrono::milliseconds(
                    mapinfo->realTimeInfo.current_time_info.raw_audio_time_ms));
        } else if (modifiers.testFlag(Qt::ControlModifier)) {
            // 修改缩放
            if (dy > 0) {
                mapinfo->baseInfo.timeline_zoom +=
                    mapinfo->editorInfo.timelineScrollStep;
                if (mapinfo->baseInfo.timeline_zoom > 5.f) {
                    mapinfo->baseInfo.timeline_zoom = 5.f;
                }
            }
            if (dy < 0) {
                mapinfo->baseInfo.timeline_zoom -=
                    mapinfo->editorInfo.timelineScrollStep;
                if (mapinfo->baseInfo.timeline_zoom < .1f) {
                    mapinfo->baseInfo.timeline_zoom = .1f;
                }
            }
        } else {
            if (dy > 0) {
                mapinfo->realTimeInfo.current_time_info +=
                    mapinfo->editorInfo.pageScrollStep;
            }
            if (dy < 0) {
                mapinfo->realTimeInfo.current_time_info +=
                    -mapinfo->editorInfo.pageScrollStep;
            }
            audio_callback->set_playpos_for(
                map->base_metadata().main_audio_path.generic_string(),
                std::chrono::milliseconds(
                    mapinfo->realTimeInfo.current_time_info.raw_audio_time_ms));
        }
    }
    current_tool->wheelEvent(e);
}
