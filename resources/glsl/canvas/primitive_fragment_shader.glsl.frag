#version 410 core
// --- 输入：从几何着色器接收 ---

// 被插值的属性
in vec2 v_TexCoord;
in vec2 v_WorldPos;

// 不被插值的属性 (flat)
flat in int gs_out_TextureLayerIdx;
flat in uint gs_out_NoFilter;
flat in vec2 gs_out_UVScale;
flat in vec2 gs_out_GroupSize;
flat in vec4 gs_out_DefColor;
flat in vec2 gs_out_Radius;
flat in float gs_out_RadiusEffectParam;
flat in uint gs_out_RadiusEffect;
flat in uint gs_out_TexScaleStratergy;
flat in uint gs_out_TexAlignStratergy;

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

// --- C++端 enum 值的常量定义 ---
const uint AUTO_SCALE_AND_CUT = 1u;
const uint SCALE_FORCE_BASEWIDTH = 2u;
const uint SCALE_FORCE_BASEHEIGHT = 3u;
const uint FORCE_FILL = 4u;
const uint TILE_REPEAT = 5u;
const uint TILE_BASEWIDTH_REPEAT = 6u;
const uint TILE_BASEHEIGHT_REPEAT = 7u;
const uint SINGLE = 8u;
const uint CHARACTER = 9u;

// TexAlignMode
const uint ALIGN_CENTER = 16u; // 0x10
const uint ALIGN_LEFT = 32u; // 0x20
const uint ALIGN_RIGHT = 48u; // 0x30
const uint ALIGN_TOP = 64u; // 0x40
const uint ALIGN_BOTTOM = 80u; // 0x50

// 纹理的蒙版效果
const uint MASK_EFFECT_NONE = 1;
const uint MASK_EFFECT_DARKEN = 2;
const uint MASK_EFFECT_FILTER = 3;
const uint MASK_EFFECT_ALPHA_SHIFT = 4;

// 圆角效果
const uint FADE_IN_AND_OUT = 1u;

// 封装UV坐标和片段可见性
struct UVResult {
    // 片段是否可见 (是否在纹理的有效区域内)
    bool visible;
    // 计算出的采样UV坐标
    vec2 uv;
};

void main() {}
