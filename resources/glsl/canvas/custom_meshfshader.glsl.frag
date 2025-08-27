#version 410 core

// --- 输出/输入/Uniforms ---
out vec4 FragColor;

// 是否绘制线框
uniform bool u_IsDrawingWireframe;

// 使用的采样数组
uniform sampler2DArray u_samplerarray;

// 传输来的片段颜色信息
in vec2 v_Color;

// 传输来的片段纹理信息
in vec2 v_TexCoord;
in vec2 v_WorldPos;

// 纹理在层中的尺寸比例
flat in vec2 f_UVScale;

// 纹理层数
flat in int f_TextureLayerIdx;
flat in uint f_NoFilter;

// 纹理组尺寸
flat in vec2 f_GroupSize;

// 蒙版结构
struct MaskLayer {
    // {left, top, right, bottom}
    vec4 rect;
    // 效果参数，用一个vec4来存储，可以灵活解释
    // DARKEN, param.x = 暗化倍率 (0.0-1.0)
    // ALPHA_SHIFT, param.x = 透明度倍率 (0.0-1.0)
    // MULTIPLY_COLOR, param.xyz = 颜色, param.w = 强度
    vec4 effectParams;
    // 效果ID
    uint effect;
};

// 蒙版ubo
layout(std140) uniform MaskStackUBO {
    MaskLayer u_MaskStack[16];
};

// 活跃的蒙版数量
uniform int u_ActiveMaskLayerCount;

// 蒙版效果
const uint MASK_EFFECT_NONE = 1;
const uint MASK_EFFECT_DARKEN = 2;
const uint MASK_EFFECT_FILTER = 3;
const uint MASK_EFFECT_ALPHA_SHIFT = 4;

// 封装UV坐标和片段可见性
struct UVResult {
    // 片段是否可见 (是否在纹理的有效区域内)
    bool visible;
    // 计算出的采样UV坐标
    vec2 uv;
};

UVResult calcUV() {
    UVResult result;
    // 默认片段是可见的
    result.visible = true;
    // 初始化UV
    result.uv = vec2(0.0);

    // 检查uv合法性
    if (f_UVScale.x == 0.0 || f_UVScale.y == 0.0 || f_GroupSize.x == 0.0 || f_GroupSize.y == 0.0) {
        result.visible = false;
        return result;
    }
}
void main() {}
