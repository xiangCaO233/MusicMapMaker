#include "MapEditor.h"

#include <qlogging.h>

#include <mutex>

#include "../../../mmm/MapWorkProject.h"
#include "../../mmm/Beat.h"
#include "../../mmm/map/osu/OsuMap.h"
#include "../../mmm/map/rm/RMMap.h"
#include "../MapWorkspaceCanvas.h"
#include "colorful-log.h"
#include "edit/HitObjectEditor.h"
#include "edit/TimingEditor.h"
#include "editor/edit/IVMTimingEditor.h"
#include "info/EditorEnumerations.h"

MapEditor::MapEditor(MapWorkspaceCanvas* canvas) : canvas_ref(canvas) {
    // 注册物件编辑器
    obj_editors[EditMethodPreference::IVM] =
        std::make_shared<IVMObjectEditor>(this);
    obj_editors[EditMethodPreference::MMM] =
        std::make_shared<MMMObjectEditor>(this);
    // 注册timing编辑器
    timing_editors[EditMethodPreference::IVM] =
        std::make_shared<IVMTimingEditor>(this);
}

MapEditor::~MapEditor() = default;

// 撤销
void MapEditor::undo() {
    XINFO("撤回操作");
    if (operation_type_stack.empty()) return;
    // 加入撤回栈
    undo_type_stack.push(operation_type_stack.top());

    // 根据类型调用编辑器撤回
    switch (operation_type_stack.top().first) {
        case HITOBJECT: {
            obj_editors[operation_type_stack.top().second]->undo();
            break;
        }
        case TIMING: {
            timing_editors[operation_type_stack.top().second]->undo();
            break;
        }
    }
    // 弹出此操作
    operation_type_stack.pop();
}

// 重做
void MapEditor::redo() {
    XINFO("重做");
    if (undo_type_stack.empty()) return;

    // 加入操作栈
    operation_type_stack.push(undo_type_stack.top());
    // 根据类型调用编辑器撤回
    switch (undo_type_stack.top().first) {
        case HITOBJECT: {
            obj_editors[operation_type_stack.top().second]->redo();
            break;
        }
        case TIMING: {
            timing_editors[operation_type_stack.top().second]->redo();
            break;
        }
    }
    // 弹出此撤回
    undo_type_stack.pop();
}

// 复制
void MapEditor::copy() {}

// 剪切
void MapEditor::cut() {}

// 粘贴
void MapEditor::paste() {}

// 画布更新尺寸
void MapEditor::update_size(const QSize& current_canvas_size) {
    cstatus.canvas_size = current_canvas_size;
    // 编辑区x起始位置
    ebuffer.edit_area_start_pos_x =
        current_canvas_size.width() * csettings.infoarea_width_scale;
    // 编辑区宽度
    ebuffer.edit_area_width =
        current_canvas_size.width() *
        (1.0 - csettings.infoarea_width_scale - csettings.preview_width_scale);
    // 预览区x起始位置
    ebuffer.preview_area_start_pos_x =
        current_canvas_size.width() * (1 - csettings.preview_width_scale);
    // 预览区宽度
    ebuffer.preview_area_width =
        current_canvas_size.width() * csettings.preview_width_scale;
    // 物件头的纹理
    ebuffer.head_texture = canvas_ref->skin.get_object_texture(
        TexType::HOLD_HEAD, ObjectStatus::COMMON);
    // 普通键的纹理
    ebuffer.note_texture = canvas_ref->skin.get_object_texture(
        TexType::NORMAL_NOTE, ObjectStatus::COMMON);

    update_areas();
    if (!canvas_ref->working_map) return;

    // 更新轨道数
    ebuffer.max_orbit = canvas_ref->working_map->orbits;

    // 更新轨道宽度
    ebuffer.orbit_width = ebuffer.edit_area_width / ebuffer.max_orbit;
    ebuffer.preview_orbit_width =
        ebuffer.preview_area_width / ebuffer.max_orbit;

    // 依据轨道宽度自动适应物件纹理尺寸
    // 物件尺寸缩放--相对于纹理尺寸
    ebuffer.width_scale =
        (ebuffer.orbit_width * 1.25) / double(ebuffer.head_texture->width);
    ebuffer.preview_width_scale = (ebuffer.preview_orbit_width * 1.25) /
                                  double(ebuffer.head_texture->width);

    // 不大于1--不放大纹理
    ebuffer.object_size_scale = std::min(ebuffer.width_scale, 1.0);
    ebuffer.preview_object_size_scale =
        std::min(ebuffer.preview_width_scale, 1.0);

    // 更新判定线位置和视觉位置
    ebuffer.judgeline_position =
        (1 - csettings.judgeline_position) * current_canvas_size.height();
    ebuffer.judgeline_visual_position =
        ebuffer.judgeline_position +
        (cstatus.static_time_offset) * cstatus.speed_zoom;
}

// 更新区域信息
void MapEditor::update_areas() {
    // 更新区域缓存
    // 信息区
    cstatus.info_area.setX(0);
    cstatus.info_area.setY(0);
    cstatus.info_area.setWidth(cstatus.canvas_size.width() *
                               csettings.infoarea_width_scale);
    cstatus.info_area.setHeight(cstatus.canvas_size.height());

    // 编辑区
    cstatus.edit_area.setX(ebuffer.edit_area_start_pos_x);
    cstatus.edit_area.setY(0);
    cstatus.edit_area.setWidth(ebuffer.edit_area_width);
    cstatus.edit_area.setHeight(cstatus.canvas_size.height());

    // 预览区
    cstatus.preview_area.setX(ebuffer.preview_area_start_pos_x);
    cstatus.preview_area.setY(0);
    cstatus.preview_area.setWidth(ebuffer.preview_area_width);
    cstatus.preview_area.setHeight(cstatus.canvas_size.height());
}

// 鼠标按下
void MapEditor::mouse_pressed(QMouseEvent* e) {
    // 选中框更新
    if (ebuffer.select_bound_locate_points) {
        // 关闭选中框
        ebuffer.select_bound_locate_points = nullptr;
        ebuffer.select_bound.setWidth(0);
        ebuffer.select_bound.setHeight(0);
    }

    bool clear_selections{true};
    switch (edit_mode) {
        case MouseEditMode::PLACE_NOTE:
        case MouseEditMode::PLACE_LONGNOTE: {
            // 编辑物件模式
            // 检查鼠标区域并传递事件给物件编辑器或切换模式
            switch (cstatus.operation_area) {
                case MouseOperationArea::EDIT: {
                    // 使用放置物件模式
                    // 仅在编辑区触发时根据项目编辑偏好
                    // 向编辑器分派编辑事件
                    if (canvas_ref->working_map) {
                        obj_editors[canvas_ref->working_map->project_reference
                                        ->config.edit_method]
                            ->mouse_pressed(e);
                    }
                    break;
                }
                default: {
                    // 在其他区域触发的鼠标事件直接无视
                    // TODO(xiang 2025-05-07): 若启用自动切换模式则切换
                    break;
                }
            }
            break;
        }
        case MouseEditMode::PLACE_TIMING: {
            // 编辑timing模式-传递事件给timing编辑器
            // 检查鼠标区域并传递事件给timing编辑器或切换模式
            switch (cstatus.operation_area) {
                case MouseOperationArea::INFO: {
                    // 使用放置timing模式-在信息区才传递编辑timing事件
                    if (canvas_ref->working_map) {
                        timing_editors[canvas_ref->working_map
                                           ->project_reference->config
                                           .edit_method]
                            ->mouse_pressed(e);
                    }
                    break;
                }
                default: {
                    // 在其他区域触发的鼠标事件直接无视
                    // TODO(xiang 2025-05-07): 若启用自动切换模式则切换
                    break;
                }
            }
            break;
        }
        case MouseEditMode::SELECT: {
            // 选择模式-不分操作区都可触发
            // 按住controll左键多选
            if ((e->modifiers() & Qt::ControlModifier)) {
                // 未按住controll清空选中列表
                clear_selections = false;
            }
            // 未悬浮在任何物件上
            // 更新选中信息
            update_selections();
            break;
        }
        case MouseEditMode::NONE: {
            // 预览模式不传递任何事件-有选择区则清空
            break;
        }
    }

    if (clear_selections) {
        std::lock_guard<std::mutex> lock1(ebuffer.selected_hitobjects_mtx);
        // 其他模式直接清空选中列表
        ebuffer.selected_hitobjects.clear();
        std::lock_guard<std::mutex> lock2(ebuffer.selected_timingss_mts);
        ebuffer.selected_timingss.clear();
    }
}

void MapEditor::mouse_released(QMouseEvent* e) {
    // 鼠标按键释放
    switch (edit_mode) {
        case MouseEditMode::PLACE_NOTE:
        case MouseEditMode::PLACE_LONGNOTE: {
            // 编辑物件模式
            // 检查鼠标区域并传递事件给物件编辑器或切换模式
            switch (cstatus.operation_area) {
                case MouseOperationArea::EDIT: {
                    // 使用放置物件模式
                    // 仅在编辑区触发时根据项目编辑偏好
                    // 向编辑器分派编辑事件
                    if (canvas_ref->working_map) {
                        obj_editors[canvas_ref->working_map->project_reference
                                        ->config.edit_method]
                            ->mouse_released(e);
                    }
                    break;
                }
                default: {
                    // 在其他区域触发的鼠标事件直接无视
                    // TODO(xiang 2025-05-07): 若启用自动切换模式则切换
                    break;
                }
            }
            break;
        }
        case MouseEditMode::PLACE_TIMING: {
            // 编辑timing模式-传递事件给timing编辑器
            // 检查鼠标区域并传递事件给timing编辑器或切换模式
            switch (cstatus.operation_area) {
                case MouseOperationArea::INFO: {
                    // 使用放置timing模式-在信息区才传递编辑timing事件
                    if (canvas_ref->working_map) {
                        timing_editors[canvas_ref->working_map
                                           ->project_reference->config
                                           .edit_method]
                            ->mouse_released(e);
                    }
                    break;
                }
                default: {
                    // 在其他区域触发的鼠标事件直接无视
                    // TODO(xiang 2025-05-07): 若启用自动切换模式则切换
                    break;
                }
            }
            break;
        }
        case MouseEditMode::SELECT: {
            break;
        }
    }
}

// 鼠标移动
void MapEditor::mouse_moved(QMouseEvent* e) {
    // 更新鼠标位置的对应时间戳
    cstatus.mouse_pos_time =
        cstatus.current_time_stamp +
        (ebuffer.judgeline_visual_position - canvas_ref->mouse_pos.y()) /
            (canvas_ref->working_map
                 ? canvas_ref->working_map->project_reference->config
                       .timeline_zoom
                 : 1.0);
    // XINFO(QString("mouse_time:[%1]").arg(cstatus.mouse_pos_time).toStdString());
    // 更新鼠标位置的对应轨道
    auto pos_off = (e->pos().x() - ebuffer.edit_area_start_pos_x);
    if (pos_off < 0 || pos_off > ebuffer.edit_area_width) {
        cstatus.mouse_pos_orbit = -1;
    } else {
        cstatus.mouse_pos_orbit = pos_off / ebuffer.orbit_width;
    }

    // 更新鼠标操作区
    if (cstatus.edit_area.contains(canvas_ref->mouse_pos)) {
        cstatus.operation_area = MouseOperationArea::EDIT;
    } else if (cstatus.preview_area.contains(canvas_ref->mouse_pos)) {
        cstatus.operation_area = MouseOperationArea::PREVIEW;
    } else {
        cstatus.operation_area = MouseOperationArea::INFO;
    }

    if (cstatus.mouse_left_pressed && !cstatus.mouse_right_pressed) {
        // 正在拖动
        switch (edit_mode) {
            case MouseEditMode::SELECT: {
                // 选择模式
                // 更新选中信息-不区分区域
                // 未悬浮在任何物件上-更新选中框定位点
                update_selection_area(e->pos(),
                                      e->modifiers() & Qt::ControlModifier);
                break;
            }
            case MouseEditMode::PLACE_NOTE:
            case MouseEditMode::PLACE_LONGNOTE: {
                // 编辑谱面模式-拖动鼠标事件
                // 仅在编辑区域触发
                switch (cstatus.operation_area) {
                    case MouseOperationArea::EDIT: {
                        // 根据编辑方式分派编辑拖动事件
                        if (canvas_ref->working_map) {
                            obj_editors[canvas_ref->working_map
                                            ->project_reference->config
                                            .edit_method]
                                ->mouse_dragged(e);
                        }
                        break;
                    }
                    default: {
                        // 其他区域拖动鼠标且当前模式是编辑-自动切换模式
                        break;
                    }
                }
            }
            case MouseEditMode::PLACE_TIMING: {
                // 编辑timing的拖动事件
                if (canvas_ref->working_map) {
                    timing_editors[canvas_ref->working_map->project_reference
                                       ->config.edit_method]
                        ->mouse_dragged(e);
                }
                break;
            }
            case MouseEditMode::NONE: {
                // 观察模式
                break;
            }
        }
    }
}

// 鼠标滚动
void MapEditor::mouse_scrolled(QWheelEvent* e) {
    // 修饰符
    auto modifiers = e->modifiers();
    auto dy = e->angleDelta().y();

    switch (cstatus.operation_area) {
        case MouseOperationArea::EDIT: {
            // 编辑区
            if (modifiers & Qt::ControlModifier) {
                // 在编辑区-按下controll滚动
                // 修改时间线缩放
                scroll_update_timelinezoom(dy);
                return;
            }
            if (modifiers & Qt::AltModifier) {
                // 在编辑区-按下alt滚动
                // 获取鼠标位置的拍--修改此拍分拍策略/改为自定义
                return;
            }
            if (!cstatus.canvas_pasued) return;
            update_timepos(dy, modifiers & Qt::ShiftModifier);
            break;
        }
        case MouseOperationArea::PREVIEW: {
            // 预览区滚动-切换预览区时间倍率
            double res_preview_scale =
                canvas_ref->working_map->project_reference->config
                    .preview_time_scale +
                0.2 * (dy > 0 ? 1.0 : -1.0);
            // 限定区间2.5x~10.0x
            if (res_preview_scale < 2.5) {
                canvas_ref->working_map->project_reference->config
                    .preview_time_scale = 2.5;
                break;
            }
            if (res_preview_scale > 10.0) {
                canvas_ref->working_map->project_reference->config
                    .preview_time_scale = 10.0;
                break;
            }
            canvas_ref->working_map->project_reference->config
                .preview_time_scale = res_preview_scale;

            canvas_ref->working_map->project_reference->canvasconfig_node
                .attribute("preview-time-scale")
                .set_value(res_preview_scale);
            break;
        }
        case MouseOperationArea::INFO: {
            // 信息区滚动
            if (e->modifiers() & Qt::ShiftModifier) {
                // 更新默认分拍策略
                if (canvas_ref->working_map) {
                    canvas_ref->working_map->project_reference->config
                        .default_divisors += (dy > 0 ? 1 : -1);
                }
            } else {
                // 若此位置存在拍则更新此拍分拍策略
                auto beats = canvas_ref->working_map->query_beat_before_time(
                    cstatus.mouse_pos_time);

                for (const auto& beat : beats) {
                    // 只修改所处的拍
                    if (beat->start_timestamp < cstatus.mouse_pos_time &&
                        beat->end_timestamp > cstatus.mouse_pos_time) {
                        beat->divisors_customed = true;
                        auto res_divisors =
                            (dy > 0 ? beat->divisors + 1 : beat->divisors - 1);
                        if (res_divisors < 1) {
                            res_divisors = 1;
                            break;
                        }
                        beat->divisors = res_divisors;
                    }
                }
            }

            break;
        }
    }
}

// 更新时间线缩放-滚动
void MapEditor::scroll_update_timelinezoom(int dy) {
    double res_timeline_zoom =
        canvas_ref->working_map->project_reference->config.timeline_zoom;
    if (dy > 0) {
        res_timeline_zoom += 0.05;
    } else {
        res_timeline_zoom -= 0.05;
    }
    if (res_timeline_zoom >= 0.2 && res_timeline_zoom <= 3.0) {
        canvas_ref->working_map->project_reference->config.timeline_zoom =
            res_timeline_zoom;

        canvas_ref->working_map->project_reference->canvasconfig_node
            .attribute("timeline-zoom")
            .set_value(res_timeline_zoom);
        // 调节成功传递调节信号
        emit canvas_ref->timeline_zoom_adjusted(
            canvas_ref->working_map->project_reference->config.timeline_zoom *
            100);
    }
}

// 吸附到附近分拍线
void MapEditor::scroll_magnet_to_divisor(int scrolldy) {
    // 获取当前时间附近的拍
    // 找到第一个拍起始时间大于或等于当前时间的拍迭代器
    auto current_beat_it =
        canvas_ref->working_map->beats.lower_bound(std::make_shared<Beat>(
            200, cstatus.current_time_stamp, cstatus.current_time_stamp));

    // 待吸附列表
    std::vector<std::shared_ptr<Beat>> magnet_beats;

    // 区分分支
    if (current_beat_it == canvas_ref->working_map->beats.end()) {
        // 没有比当前时间更靠后的拍了
        current_beat_it--;
        auto last_beat = *current_beat_it;
        // 添加最后一拍
        magnet_beats.emplace_back(last_beat);

        if (cstatus.current_time_stamp == last_beat->start_timestamp) {
            // 在拍头--添加倒数第二拍的小节线和本拍小节线
            current_beat_it--;
            // auto second_tolast_beat = *current_beat_it;
            magnet_beats.emplace_back(*current_beat_it);
        } else if (cstatus.current_visual_time_stamp >
                       last_beat->start_timestamp &&
                   cstatus.current_visual_time_stamp <
                       last_beat->end_timestamp) {
            // 在拍内--可只添加本拍小节线
        } else {
            // 在拍外--可只添加最后一拍的最后一个小节线
        }
    } else if (current_beat_it == canvas_ref->working_map->beats.begin()) {
        // 只有第一拍比当前时间更靠后了-比拍头更靠前或在拍头
        auto first_beat = *current_beat_it;
        magnet_beats.emplace_back(first_beat);
        // if (current_time_stamp < first_beat->start_timestamp) {
        //   // 在拍外--只添加第一拍的拍头
        //   magnet_beats.emplace_back(first_beat);
        // } else {
        //   // 在拍头--只添加本拍小节线
        //   magnet_beats.emplace_back(first_beat);
        // }
        //  在拍内--不会走此分支
    } else {
        // 附近都有拍--本拍前一拍后一拍的所有小节线全部加入吸附
        // auto current_beat = *current_beat_it;
        magnet_beats.emplace_back(*current_beat_it);
        --current_beat_it;
        // auto previous_beat = *current_beat_it;
        magnet_beats.emplace_back(*current_beat_it);
        ++current_beat_it;
        ++current_beat_it;
        if (current_beat_it != canvas_ref->working_map->beats.end()) {
            // auto next_beat = *current_beat_it;
            magnet_beats.emplace_back(*current_beat_it);
        }
    }
    // 待吸附时间集合
    std::set<double> divisor_times;
    // 初始化时间集合
    for (const auto& magnet_beat : magnet_beats) {
        for (int i = 0; i < magnet_beat->divisors; i++) {
            divisor_times.insert(magnet_beat->start_timestamp +
                                 i *
                                     (magnet_beat->end_timestamp -
                                      magnet_beat->start_timestamp) /
                                     magnet_beat->divisors);
        }
    }

    // 找到第一个时间大于或等于当前时间的小节线时间迭代器
    auto current_divisor_it =
        divisor_times.lower_bound(cstatus.current_time_stamp);
    // 根据滑动方向选择吸附哪一个
    if (scrolldy * cstatus.scroll_direction > 0) {
        // 向上吸附小节线
        if (std::abs(*current_divisor_it - cstatus.current_time_stamp) < 10) {
            // 防止非法访问最后一个元素后的元素
            auto lasttime = divisor_times.end();
            lasttime--;
            if (current_divisor_it != lasttime) {
                // 找到自己了-吸下一个
                ++current_divisor_it;
                cstatus.current_time_stamp = *current_divisor_it;
            } else {
                // 不动
            }
        } else {
            // 选择的大于自己的第一个
            cstatus.current_time_stamp = *current_divisor_it;
        }
    } else {
        // 防止非法访问第一个元素前的元素
        if (current_divisor_it != divisor_times.begin()) {
            // 向下吸附小节线--不管是找到自己还是找到下一个,吸上一个
            --current_divisor_it;
            cstatus.current_time_stamp = *current_divisor_it;
        }
    }
}

// 更新谱面位置
void MapEditor::update_timepos(int scrolldy, bool is_shift_down) {
    double temp_scroll_ration{1.0};
    if (is_shift_down) {
        // 在编辑区-按下shift滚动
        // 短暂增加滚动倍率
        temp_scroll_ration = 3.0;
    }

    if (cstatus.is_magnet_to_divisor) {
        scroll_magnet_to_divisor(scrolldy);
    } else {
        auto scroll_unit =
            (scrolldy > 0 ? 1.0 : -1.0) *
            canvas_ref->working_map->project_reference->config.timeline_zoom *
            cstatus.canvas_size.height() / 10.0;
        cstatus.current_time_stamp += scroll_unit * temp_scroll_ration *
                                      cstatus.scroll_ratio *
                                      cstatus.scroll_direction;
    }
    if (cstatus.current_time_stamp < 0) {
        cstatus.current_time_stamp = 0;
        if (!cstatus.canvas_pasued) {
            cstatus.canvas_pasued = true;
            emit canvas_ref->pause_signal(cstatus.canvas_pasued);
        }
    }
    if (cstatus.current_time_stamp > canvas_ref->working_map->map_length) {
        cstatus.current_time_stamp = canvas_ref->working_map->map_length;
        if (!cstatus.canvas_pasued) {
            cstatus.canvas_pasued = true;
            emit canvas_ref->pause_signal(cstatus.canvas_pasued);
        }
    }
    cstatus.current_visual_time_stamp =
        cstatus.current_time_stamp + cstatus.static_time_offset;
    canvas_ref->played_effects_objects.clear();

    // 更新项目中自己的位置
    canvas_ref->working_map->project_reference->map_canvasposes.at(
        canvas_ref->working_map) = cstatus.current_time_stamp;
    emit canvas_ref->current_time_stamp_changed(cstatus.current_time_stamp);

    canvas_ref->effect_thread->sync_music_time(cstatus.current_time_stamp);
}

// 更新选中信息
void MapEditor::update_selections() {
    std::lock_guard<std::mutex> lock(ebuffer.selected_hitobjects_mtx);
    // 选中
    if (ebuffer.hover_object_info) {
        // 有悬浮在物件上
        ebuffer.selected_hitobjects.emplace(
            ebuffer.hover_object_info->hoverobj);
    }
    if (ebuffer.hover_timings) {
        // 有悬浮在timing上
        ebuffer.selected_timingss.emplace(ebuffer.hover_timings);
    }

    // 发送更新选中物件信号
    if (ebuffer.hover_object_info) {
        emit canvas_ref->select_object(ebuffer.hover_object_info->hoverbeat,
                                       ebuffer.hover_object_info->hoverobj,
                                       ebuffer.current_abs_timing);
    } else {
        emit canvas_ref->select_object(nullptr, nullptr, nullptr);
    }
    // 发送更新选中timing信号
    emit canvas_ref->select_timing(ebuffer.hover_timings);
}

// 更新选中区域
void MapEditor::update_selection_area(QPoint&& p, bool ctrl_down) {
    if (!ebuffer.select_bound_locate_points) {
        ebuffer.select_bound_locate_points =
            std::make_shared<std::pair<QPointF, QPointF>>(
                cstatus.mouse_left_press_pos, p);
    } else {
        ebuffer.select_bound_locate_points->second = p;
    }

    // 更新选中区域
    ebuffer.select_bound.setX(
        std::min(ebuffer.select_bound_locate_points->first.x(),
                 ebuffer.select_bound_locate_points->second.x()));
    ebuffer.select_bound.setY(
        std::min(ebuffer.select_bound_locate_points->first.y(),
                 ebuffer.select_bound_locate_points->second.y()));
    ebuffer.select_bound.setWidth(
        std::abs(ebuffer.select_bound_locate_points->first.x() -
                 ebuffer.select_bound_locate_points->second.x()));
    ebuffer.select_bound.setHeight(
        std::abs(ebuffer.select_bound_locate_points->first.y() -
                 ebuffer.select_bound_locate_points->second.y()));
}
