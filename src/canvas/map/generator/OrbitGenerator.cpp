#include "OrbitGenerator.h"

#include "../../../mmm/MapWorkProject.h"
#include "../../RenderBuffer.hpp"
#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"
#include "texture/Texture.h"

// 构造OrbitGenerate
OrbitGenerator::OrbitGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {}

// 析构OrbitGenerate
OrbitGenerator::~OrbitGenerator() = default;

// 生成轨道渲染指令
void OrbitGenerator::generate(BufferWrapper *bufferwrapper) {
    auto &params_list = bufferwrapper->orbits_datas.emplace();

    auto orbit_texture = editor_ref->canvas_ref->skin.get_orbit_bg_texture();
    // 计算每个轨道的区域
    for (int i = 0; i < editor_ref->canvas_ref->working_map->orbits; ++i) {
        auto &params = params_list.emplace_back();

        // 第i个轨道
        // 轨道的x位置
        auto orbit_xpos = editor_ref->ebuffer.edit_area_start_pos_x +
                          editor_ref->ebuffer.orbit_width * i;
        // 轨道的实际绘制纹理宽度-物件缩放*轨道纹理宽度尺寸
        auto orbit_texture_width =
            editor_ref->ebuffer.object_size_scale * orbit_texture->width;
        // 居中
        orbit_xpos +=
            (editor_ref->ebuffer.orbit_width - orbit_texture_width) / 2.0;

        params.func_type = FunctionType::MRECT;
        // 可以直接拉伸-纵向
        params.render_settings.texture_fillmode = TextureFillMode::FILL;
        params.texture = orbit_texture;
        params.xpos = orbit_xpos;
        params.ypos = 0;
        params.width = orbit_texture_width;
        params.height = editor_ref->cstatus.canvas_size.height();
        params.is_volatile = false;
    }
}
