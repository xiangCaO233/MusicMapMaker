#include "PreviewGenerator.h"

#include "../../../mmm/MapWorkProject.h"
#include "../MapWorkspaceCanvas.h"
#include "../RenderBuffer.hpp"
#include "../editor/MapEditor.h"
#include "general/HoldGenerator.h"
#include "general/NoteGenerator.h"
#include "general/SlideGenerator.h"

// 构造PreviewGenerator
PreviewGenerator::PreviewGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {
    // 注册物件生成器
    // 单物件
    objgenerators[NoteType::NOTE] = std::make_shared<NoteGenerator>(editor);
    // 面条物件
    objgenerators[NoteType::HOLD] = std::make_shared<HoldGenerator>(editor);
    // 滑键物件
    objgenerators[NoteType::SLIDE] = std::make_shared<SlideGenerator>(editor);
}

// 析构PreviewGenerator
PreviewGenerator::~PreviewGenerator() = default;

// 生成预览
void PreviewGenerator::generate(BufferWrapper *bufferwrapper) {
    if (!editor_ref->canvas_ref->working_map) return;
    auto &preview_params_list = bufferwrapper->preview_datas.emplace();
    // 确定区域

    auto area_center_time = (editor_ref->ebuffer.current_time_area_end +
                             editor_ref->ebuffer.current_time_area_start) /
                            2.0;

    auto parea_start_time =
        area_center_time -
        std::abs(area_center_time -
                 editor_ref->ebuffer.current_time_area_start) *
            editor_ref->canvas_ref->working_map->project_reference->config
                .preview_time_scale;
    auto parea_end_time =
        area_center_time +
        std::abs(area_center_time - editor_ref->ebuffer.current_time_area_end) *
            editor_ref->canvas_ref->working_map->project_reference->config
                .preview_time_scale;

    // 查询此区域内所有物件
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> temp_objects;
    editor_ref->canvas_ref->working_map->query_object_in_range(
        temp_objects, int32_t(parea_start_time), int32_t(parea_end_time), true);

    // 防止重复绘制
    std::unordered_map<std::shared_ptr<HitObject>, bool> drawed_objects;
    // 渲染物件
    // 计算图形
    for (const auto &obj : temp_objects) {
        if (!obj->is_note || obj->object_type == HitObjectType::RMCOMPLEX)
            continue;
        auto note = std::static_pointer_cast<Note>(obj);
        if (!note) continue;
        if (drawed_objects.find(note) == drawed_objects.end())
            drawed_objects.insert({note, true});
        else
            continue;
        if (note) {
            // 生成物件
            const auto &generator = objgenerators[note->note_type];
            generator->generate_preview(obj);
            if (note->note_type == NoteType::NOTE) {
                // 除单键外的物件已经处理了头
                generator->preview_object_enqueue();
            }
        }
    }

    RendererManagerSettings settings;
    // 切换纹理绘制方式为填充
    settings.texture_fillmode = TextureFillMode::FILL;
    // 切换纹理绘制补齐方式为重采样
    settings.texture_complementmode = TextureComplementMode::REPEAT_TEXTURE;
    settings.texture_effect = TextureEffect::NONE;

    // 按计算层级渲染预览图形
    while (!ObjectGenerator::preview_shape_queue.empty()) {
        auto &shape = ObjectGenerator::preview_shape_queue.front();
        auto &preview_params = preview_params_list.emplace_back();

        preview_params.func_type = FunctionType::MRECT;
        preview_params.render_settings = settings;
        preview_params.xpos = shape.x;
        preview_params.ypos = shape.y;
        preview_params.width = shape.w;
        preview_params.height = shape.h;
        preview_params.texture = shape.tex;
        preview_params.is_volatile = true;

        ObjectGenerator::preview_shape_queue.pop();
    }

    auto &preview_params1 = preview_params_list.emplace_back();
    // 预览区区域显示
    auto preview_xpos = editor_ref->ebuffer.preview_area_start_pos_x;
    // 绘制一层滤镜
    preview_params1.func_type = FunctionType::MRECT;
    preview_params1.render_settings = settings;
    preview_params1.xpos = preview_xpos;
    preview_params1.ypos = 0.0;
    preview_params1.width = editor_ref->ebuffer.preview_area_width;
    preview_params1.height = editor_ref->cstatus.canvas_size.height();
    preview_params1.r = 6;
    preview_params1.g = 6;
    preview_params1.b = 6;
    preview_params1.a = 75;
    preview_params1.is_volatile = false;

    // 预览区当前区域
    auto preview_height =
        editor_ref->cstatus.canvas_size.height() /
            editor_ref->canvas_ref->working_map->project_reference->config
                .preview_time_scale +
        editor_ref->cstatus.graphic_offset *
            editor_ref->canvas_ref->working_map->project_reference->config
                .timeline_zoom;

    // 预览区域判定线
    // 居中
    auto preview_judgeline_ypos =
        editor_ref->cstatus.canvas_size.height() / 2.0 +
        preview_height * (0.5 - editor_ref->csettings.judgeline_position);

    // 绘制预览区域判定线
    auto &preview_params2 = preview_params_list.emplace_back();
    preview_params2.func_type = FunctionType::MLINE;
    preview_params2.render_settings = settings;
    preview_params2.x1 = preview_xpos;
    preview_params2.y1 = preview_judgeline_ypos;
    preview_params2.x2 = editor_ref->cstatus.canvas_size.width();
    preview_params2.y2 = preview_judgeline_ypos;
    preview_params2.line_width = 2;
    preview_params2.r = 0;
    preview_params2.g = 255;
    preview_params2.b = 255;
    preview_params2.a = 220;
    preview_params2.is_volatile = false;

    auto preview_ypos =
        preview_judgeline_ypos -
        preview_height * (1 - editor_ref->csettings.judgeline_position);

    // 绘制预览区当前区域
    // 绘制一层滤镜
    auto &preview_params3 = preview_params_list.emplace_back();
    preview_params3.func_type = FunctionType::MRECT;
    preview_params3.render_settings = settings;

    preview_params3.xpos = preview_xpos;
    preview_params3.ypos = preview_ypos;
    preview_params3.width = editor_ref->ebuffer.preview_area_width;
    preview_params3.height = preview_height;
    preview_params3.r = 233;
    preview_params3.g = 233;
    preview_params3.b = 233;
    preview_params3.a = 75;
    preview_params3.is_volatile = false;
}
