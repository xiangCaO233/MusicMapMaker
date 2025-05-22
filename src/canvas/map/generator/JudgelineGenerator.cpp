#include "JudgelineGenerator.h"

#include "../../../mmm/MapWorkProject.h"
#include "../../RenderBuffer.hpp"
#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"
#include "texture/Texture.h"

// 构造JudgelineGenerator
JudgelineGenerator::JudgelineGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {}

// 析构JudgelineGenerator
JudgelineGenerator::~JudgelineGenerator() = default;

// 生成轨道渲染指令
void JudgelineGenerator::generate(BufferWrapper *bufferwrapper) {
    // 主区域判定线
    auto orbit_judge_texture =
        editor_ref->canvas_ref->skin.get_orbit_judge_texture();
    auto &orbit_params_list = bufferwrapper->orbits_datas.emplace();

    // 计算每个轨道的区域
    for (int i = 0; i < editor_ref->canvas_ref->working_map->orbits; ++i) {
        auto &orbit_params = orbit_params_list.emplace_back();

        // 第i个轨道
        // 轨道的x位置
        auto orbit_xpos = editor_ref->ebuffer.edit_area_start_pos_x +
                          editor_ref->ebuffer.orbit_width * i;

        // 轨道的实际绘制纹理宽度-物件缩放*轨道纹理宽度尺寸
        auto orbit_texture_width =
            editor_ref->ebuffer.object_size_scale * orbit_judge_texture->width;
        auto orbit_texture_height =
            editor_ref->ebuffer.object_size_scale * orbit_judge_texture->height;

        // 居中
        orbit_xpos +=
            (editor_ref->ebuffer.orbit_width - orbit_texture_width) / 2.0;

        orbit_params.func_type = FunctionType::MRECT;
        orbit_params.render_settings.texture_fillmode = TextureFillMode::FILL;
        orbit_params.xpos = orbit_xpos;
        orbit_params.ypos = editor_ref->ebuffer.judgeline_visual_position -
                            orbit_texture_height / 2.0;
        orbit_params.width = orbit_texture_width;
        orbit_params.height = orbit_texture_height;
        orbit_params.texture = orbit_judge_texture;
        orbit_params.is_volatile = false;
    }
}
