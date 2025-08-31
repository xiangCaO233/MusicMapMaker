#version 410 core
// layout(location = 0) 画到当前FBO的 GL_COLOR_ATTACHMENT0
layout(location = 0) out vec4 out_SceneColor;

// layout(location = 1) 画到当前FBO的 GL_COLOR_ATTACHMENT1
layout(location = 1) out vec4 out_GlowMask;

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

const uint FADE_IN_AND_OUT = 1u;
// 发光
const uint GLOWING = 2u;

void main() {
    // out_SceneColor = finalColor;
    out_GlowMask = vec4(0.0, 0.0, 0.0, 1.0);
}
