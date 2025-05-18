#include "NoteGenerator.h"

#include <memory>
#include <thread>

#include "../../../../mmm/MapWorkProject.h"
#include "../../../../mmm/hitobject/Note/Note.h"
#include "../../MapWorkspaceCanvas.h"
#include "../../editor/MapEditor.h"
#include "generator/ObjectGenerator.h"

// 构造NoteGenerator
NoteGenerator::NoteGenerator(std::shared_ptr<MapEditor>& editor)
    : ObjectGenerator(editor) {}

// 析构NoteGenerator
NoteGenerator::~NoteGenerator() = default;

// 生成物件渲染指令
void NoteGenerator::generate(Note& note) {
    auto note_ptr = std::shared_ptr<HitObject>(&note, [](Note*) {});
    auto texture = note.note_type == NoteType::NOTE
                       ? editor_ref->ebuffer.note_texture
                       : editor_ref->ebuffer.head_texture;
    auto note_size =
        QSizeF(texture->width * editor_ref->ebuffer.object_size_scale *
                   editor_ref->canvas_ref->working_map->project_reference
                       ->config.object_width_ratio,
               texture->height * editor_ref->ebuffer.object_size_scale *
                   editor_ref->canvas_ref->working_map->project_reference
                       ->config.object_height_ratio);
    double note_visual_time =
        editor_ref->cstatus.current_visual_time_stamp +
        (note.timestamp - editor_ref->cstatus.current_visual_time_stamp) *
            editor_ref->cstatus.speed_zoom;

    // 物件距离判定线距离从下往上--反转
    // 当前物件头位置-中心
    auto note_center_pos_y =
        editor_ref->cstatus.canvas_size.height() *
            (1.0 - editor_ref->csettings.judgeline_position) -
        ((editor_ref->cstatus.canvas_pasued ? note.timestamp
                                            : note_visual_time) -
         editor_ref->cstatus.current_visual_time_stamp) *
            editor_ref->canvas_ref->working_map->project_reference->config
                .timeline_zoom;

    // 物件头中心位置
    auto note_center_pos_x = editor_ref->ebuffer.edit_area_start_pos_x +
                             editor_ref->ebuffer.orbit_width * note.orbit +
                             editor_ref->ebuffer.orbit_width / 2.0;

    // 物件头左上角位置
    double head_note_pos_x = note_center_pos_x - note_size.width() / 2.0;

    // 物件头的实际区域
    head_rect =
        QRectF(head_note_pos_x, note_center_pos_y - note_size.height() / 2.0,
               note_size.width(), note_size.height());

    // 节点
    const auto& complex_note_node_texture =
        editor_ref->canvas_ref->skin.get_object_texture(TexType::NODE,
                                                        ObjectStatus::COMMON);
    node_size = QSizeF(complex_note_node_texture->width *
                           editor_ref->ebuffer.object_size_scale * 0.75 * 0.75 *
                           editor_ref->canvas_ref->working_map
                               ->project_reference->config.object_width_ratio,
                       complex_note_node_texture->height *
                           editor_ref->ebuffer.object_size_scale * 0.75 * 0.75 *
                           editor_ref->canvas_ref->working_map
                               ->project_reference->config.object_height_ratio);

    // 生成图形
    if (head_rect.contains(editor_ref->canvas_ref->mouse_pos)) {
        // 优先使用悬停时的纹理
        head_texture = editor_ref->canvas_ref->skin.get_object_texture(
            TexType::HOLD_HEAD, ObjectStatus::HOVER);
        editor_ref->ebuffer.hover_object_info =
            std::make_shared<HoverObjectInfo>(note_ptr, note.beatinfo,
                                              HoverPart::HEAD);

        editor_ref->cstatus.is_hover_note = true;
    } else {
        auto in_select_bound =
            editor_ref->csettings.strict_select
                ? editor_ref->ebuffer.select_bound.contains(head_rect)
                : editor_ref->ebuffer.select_bound.intersects(head_rect);
        auto it = editor_ref->ebuffer.selected_hitobjects.find(note_ptr);
        if (in_select_bound ||
            it != editor_ref->ebuffer.selected_hitobjects.end()) {
            if (it == editor_ref->ebuffer.selected_hitobjects.end()) {
                // 未选中则选中此物件
                editor_ref->ebuffer.selected_hitobjects.emplace(note_ptr);
                // 发送更新选中物件信号
                emit editor_ref->canvas_ref->select_object(
                    note.beatinfo, note_ptr,
                    editor_ref->ebuffer.current_abs_timing);
            }
            // 选中时的纹理
            head_texture = editor_ref->canvas_ref->skin.get_object_texture(
                TexType::HOLD_HEAD, ObjectStatus::SELECTED);
        } else {
            // 正常纹理
            head_texture = editor_ref->canvas_ref->skin.get_object_texture(
                TexType::HOLD_HEAD, ObjectStatus::COMMON);
        }
    }
    objref = note_ptr;
}

// 生成预览物件
void NoteGenerator::generate_preview(Note& note) {
    auto note_ptr = std::shared_ptr<HitObject>(&note, [](Note*) {});
    auto texture = note.note_type == NoteType::NOTE
                       ? editor_ref->ebuffer.note_texture
                       : editor_ref->ebuffer.head_texture;
    auto head_note_size = QSizeF(
        texture->width * editor_ref->ebuffer.preview_object_size_scale *
            editor_ref->canvas_ref->working_map->project_reference->config
                .object_width_ratio,
        texture->height * editor_ref->ebuffer.preview_object_size_scale * 0.75 *
            editor_ref->canvas_ref->working_map->project_reference->config
                .object_height_ratio);
    auto preview_height =
        editor_ref->cstatus.canvas_size.height() /
            editor_ref->canvas_ref->working_map->project_reference->config
                .preview_time_scale +
        editor_ref->cstatus.static_time_offset *
            editor_ref->canvas_ref->working_map->project_reference->config
                .timeline_zoom;

    // 物件距离判定线距离从下往上--反转
    // 当前物件头位置-中心
    auto note_center_pos_y =
        editor_ref->ebuffer.judgeline_position -
        // 偏移到预览画布判定线处
        std::abs(editor_ref->ebuffer.judgeline_position -
                 editor_ref->cstatus.canvas_size.height() / 2.0) +
        preview_height * (0.5 - editor_ref->csettings.judgeline_position) -
        (note.timestamp - editor_ref->cstatus.current_visual_time_stamp) *
            editor_ref->canvas_ref->working_map->project_reference->config
                .timeline_zoom /
            editor_ref->canvas_ref->working_map->project_reference->config
                .preview_time_scale;

    // 物件头中心位置
    auto note_center_pos_x =
        editor_ref->ebuffer.preview_area_start_pos_x +
        editor_ref->ebuffer.preview_orbit_width * note.orbit +
        editor_ref->ebuffer.preview_orbit_width / 2.0;

    // 物件头左上角位置
    double head_note_pos_x = note_center_pos_x - head_note_size.width() / 2.0;

    // 物件头的实际区域
    head_rect = QRectF(head_note_pos_x,
                       note_center_pos_y - head_note_size.height() / 2.0,
                       head_note_size.width(), head_note_size.height());

    // 节点
    const auto& complex_note_node_texture =
        editor_ref->canvas_ref->skin.get_object_texture(TexType::NODE,
                                                        ObjectStatus::COMMON);
    node_size = QSizeF(complex_note_node_texture->width *
                           editor_ref->ebuffer.preview_object_size_scale * 0.5 *
                           editor_ref->canvas_ref->working_map
                               ->project_reference->config.object_width_ratio,
                       complex_note_node_texture->height *
                           editor_ref->ebuffer.preview_object_size_scale * 0.5 *
                           editor_ref->canvas_ref->working_map
                               ->project_reference->config.object_height_ratio);

    // 生成图形
    if (head_rect.contains(editor_ref->canvas_ref->mouse_pos)) {
        // 优先使用悬停时的纹理
        head_texture = editor_ref->canvas_ref->skin.get_object_texture(
            TexType::HOLD_HEAD, ObjectStatus::HOVER);
        editor_ref->ebuffer.hover_object_info =
            std::make_shared<HoverObjectInfo>(note_ptr, note.beatinfo,
                                              HoverPart::HEAD);

        editor_ref->cstatus.is_hover_note = true;
    } else {
        auto in_select_bound =
            editor_ref->csettings.strict_select
                ? editor_ref->ebuffer.select_bound.contains(head_rect)
                : editor_ref->ebuffer.select_bound.intersects(head_rect);
        auto it = editor_ref->ebuffer.selected_hitobjects.find(note_ptr);
        if (in_select_bound ||
            it != editor_ref->ebuffer.selected_hitobjects.end()) {
            if (it == editor_ref->ebuffer.selected_hitobjects.end()) {
                // 未选中则选中此物件
                editor_ref->ebuffer.selected_hitobjects.emplace(note_ptr);
                // 发送更新选中物件信号
                emit editor_ref->canvas_ref->select_object(
                    note.beatinfo, note_ptr,
                    editor_ref->ebuffer.current_abs_timing);
            }
            // 选中时的纹理
            head_texture = editor_ref->canvas_ref->skin.get_object_texture(
                TexType::HOLD_HEAD, ObjectStatus::SELECTED);
        } else {
            // 正常纹理
            head_texture = editor_ref->canvas_ref->skin.get_object_texture(
                TexType::HOLD_HEAD, ObjectStatus::COMMON);
        }
    }
    objref = note_ptr;
}

// 完成物件头的生成-添加到渲染队列
void NoteGenerator::object_enqueue() {
    shape_queue.emplace(head_rect.x(), head_rect.y(), head_rect.width(),
                        head_rect.height(), head_texture, objref);
}

// 完成生成-添加到预览渲染列表
void NoteGenerator::preview_object_enqueue() {
    preview_shape_queue.emplace(head_rect.x(), head_rect.y(), head_rect.width(),
                                head_rect.height(), head_texture, objref);
}
