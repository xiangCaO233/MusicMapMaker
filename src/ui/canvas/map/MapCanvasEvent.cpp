#include <QKeyEvent>
#include <QObject>
#include <canvas/map/MapCanvas.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/LayerManager.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/project/MProject.hpp>
#include <mmm/timing/Beat.hpp>
#include <string>

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
    auto layermanager =
        static_cast<MapLayerManager *>(dataloop()->layermanager());

    // qDebug() << "--- Wheel Event ---";
    // qDebug() << "angleDelta:"
    //          << e->angleDelta();  // 经典鼠标滚轮的步进值 (通常是 +/- 120)
    // qDebug() << "pixelDelta:" << e->pixelDelta();  //
    // 触控板或高精度鼠标的像素值 qDebug() << "Modifiers:"
    //          << e->modifiers();          // 按下的修饰键 (Ctrl, Shift, Alt)
    // qDebug() << "Phase:" << e->phase();  // 滚动阶段 (开始、更新、结束)

    // 先响应map滚动
    auto modifiers = e->modifiers();
    auto d = e->angleDelta();
    auto dy = d.y();
    if (dy == 0) {
        dy = d.x();
    }

    auto &editor_info = mapinfo->editorInfo;

    auto &scrollInfo = editor_info.scrollInfo;
    auto &previewInfo = editor_info.previewAreaInfo;

    if (modifiers.testFlag(Qt::ShiftModifier)) {
        // 按住shift倍乘dy
        dy *= 3.f;
    }

    if (editor_info.scrollInfo.scroll_natural) {
        // 自然滚动反转dy
        dy = -dy;
    }

    if (map) {
        auto maplayermananger =
            static_cast<MapLayerManager *>(dataloop()->layermanager());
        auto mouseState =
            maplayermananger->get_tool_interaction_state()->getMouseState();
        if (mouseState.area == MouseArea::EDIT) {
            if (modifiers.testFlag(Qt::AltModifier)) {
                if (modifiers.testFlag(Qt::ControlModifier)) {
                    // 修改分拍时按住ctrl倍乘dy
                    dy *= 2.f;
                }
                // 调整分拍
                auto converter =
                    maplayermananger->get_time_converter_manager()
                        ->getConverter(map->timing_set(), mapinfo->baseInfo,
                                       mapinfo->editorInfo.scrollInfo,
                                       mapinfo.get(),
                                       map->base_metadata().preference_bpm);
                auto &beat_timeline = map->beat_timeline();
                auto &beat_info = map->beat_info();
                auto mousePos = maplayermananger->get_tool_interaction_state()
                                    ->getMouseState()
                                    .current_pos;
                auto mouseTime = converter->distanceToTime(
                    height() - mousePos.y -
                        height() * mapinfo->editorInfo.judgeline_pos,
                    mapinfo->realTimeInfo.current_time_info
                        .presentation_canvas_time);
                auto beat = findBeatAtTime(mouseTime, beat_timeline, beat_info);
                XINFO("当前滚动分拍时间:[" + std::to_string(mouseTime) + "]");
                if (beat) {
                    wheelDyAccumulator += dy;
                    // 设置一个阈值，通常 angleDelta 的一格是 120
                    const int SNAP_THRESHOLD{120};
                    bool continuous_back{false};
                    while (std::abs(wheelDyAccumulator) >= SNAP_THRESHOLD) {
                        if (dy > 0) {
                            // 增加分拍数
                            beat->divisors++;
                            beat->is_manual = true;
                            layermanager->get_edit_eventq()->push(
                                {MMapEditEventType::BeatUpdated, beat});
                        } else {
                            // 减少分拍数
                            beat->divisors--;
                            beat->is_manual = true;
                            layermanager->get_edit_eventq()->push(
                                {MMapEditEventType::BeatUpdated, beat});
                        }
                        // 从累加器中减去已处理的部分，而不是直接清零
                        // 这可以保留用户快速滚动时的“多余”滚动量
                        if (wheelDyAccumulator != 0) {
                            if (wheelDyAccumulator > 0) {
                                wheelDyAccumulator -= SNAP_THRESHOLD;
                                if (wheelDyAccumulator > SNAP_THRESHOLD) {
                                    continuous_back = true;
                                }
                            } else {
                                wheelDyAccumulator += SNAP_THRESHOLD;
                                if (wheelDyAccumulator < -SNAP_THRESHOLD) {
                                    continuous_back = true;
                                }
                            }
                        }
                    }
                }
            } else if (modifiers.testFlag(Qt::ControlModifier)) {
                // 按住controll修改缩放
                scrollInfo.timeline_zoom +=
                    dy * scrollInfo.staticTimelineScrollRatio *
                    scrollInfo.timelineScrollRatio;
                // 边缘限制
                if (scrollInfo.timeline_zoom > 10.f) {
                    scrollInfo.timeline_zoom = 10.f;
                }
                if (scrollInfo.timeline_zoom < .25f) {
                    scrollInfo.timeline_zoom = .25f;
                }
            } else {
                if (mapinfo->editorInfo.magnet_to_divisor) {
                    wheelDyAccumulator += dy;
                    // 设置一个阈值，通常 angleDelta 的一格是 120
                    const int SNAP_THRESHOLD{120};
                    bool continuous_back{false};
                    while (std::abs(wheelDyAccumulator) >= SNAP_THRESHOLD) {
                        // 更新画布位置和音频位置
                        auto &beat_timeline = map->beat_timeline();
                        auto &beat_info = map->beat_info();
                        // static auto total_deltay{0};
                        // 吸附拍线-按dy符号确定移动方向
                        DivisorLineInfo divinfo;
                        if (dy > 0) {
                            // 向上吸附拍线
                            divinfo = findNearestDivisorLineInDirection(
                                mapinfo->realTimeInfo.current_time_info
                                    .presentation_canvas_time,
                                SearchDirection::AFTER, beat_info, 3);
                        } else {
                            // 向下吸附拍线
                            divinfo = findNearestDivisorLineInDirection(
                                mapinfo->realTimeInfo.current_time_info
                                    .presentation_canvas_time,
                                SearchDirection::PREVIOUS, beat_info,
                                3 - (continuous_back
                                         ? (mapinfo->realTimeInfo.offset_info
                                                .global_static_offset_ms +
                                            mapinfo->realTimeInfo.offset_info
                                                .global_offset_ms)
                                         : 0));
                        }
                        if (divinfo.is_valid()) {
                            mapinfo->realTimeInfo.current_time_info =
                                divinfo.divisor_time;
                            set_maintrack_pos(std::chrono::milliseconds(
                                mapinfo->realTimeInfo.current_time_info
                                    .raw_audio_time_ms));
                        }
                        // 从累加器中减去已处理的部分，而不是直接清零
                        // 这可以保留用户快速滚动时的“多余”滚动量
                        if (wheelDyAccumulator != 0) {
                            if (wheelDyAccumulator > 0) {
                                wheelDyAccumulator -= SNAP_THRESHOLD;
                                if (wheelDyAccumulator > SNAP_THRESHOLD) {
                                    continuous_back = true;
                                }
                            } else {
                                wheelDyAccumulator += SNAP_THRESHOLD;
                                if (wheelDyAccumulator < -SNAP_THRESHOLD) {
                                    continuous_back = true;
                                }
                            }
                        }
                    }
                } else {
                    auto step = scrollInfo.pageScrollStepRatio * dy;
                    // XINFO(std::format("移动画布,步长:[{}]", step));
                    // 非吸附拍线-按dy值步长移动

                    auto res = int64_t(mapinfo->realTimeInfo.current_time_info
                                           .presentation_canvas_time +
                                       step);

                    set_maintrack_pos(std::chrono::milliseconds(res));
                }
            }

        } else if (mouseState.area == MouseArea::PREVIEW) {
            // 在预览区内滚动-直接修改预览缩放倍率
            // 反向使向上滑动为放大
            previewInfo.areaRatio -= dy * scrollInfo.staticPreviewScrollRatio *
                                     scrollInfo.previewScrollRatio;
            // 边缘限制
            if (previewInfo.areaRatio > 10.f) {
                previewInfo.areaRatio = 10.f;
            }
            if (previewInfo.areaRatio < 2.f) {
                previewInfo.areaRatio = 2.f;
            }
        }
    }
    current_tool->wheelEvent(e);
}
