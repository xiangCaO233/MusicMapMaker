#version 410 core

uniform bool u_IsDrawingWireframe;
// 使用的采样数组
uniform sampler2DArray u_samplerarray;

// 传输来的片段纹理信息
in vec2 v_TexCoord;
in vec2 v_WorldPos;
// 纹理在层中的实际尺寸
flat in vec2 f_UVScale;
// 纹理层数
flat in uint f_TextureLayerIdx;
flat in uint f_NoFilter;
// 默认颜色
flat in vec4 f_DefColor;

// 纹理贴图策略
flat in uint f_TexScaleStratergy;
flat in uint f_TexAlignStratergy;

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
// 纹理的蒙版效果
// enum class MaskEffect : uint32_t {
//     // 无效果
//     NONE = 0x00000001,
//     // 暗化
//     DARKEN = 0x00000002,
//     // 滤镜
//     FILTER = 0x00000003,
//     // 透明变换
//     ALPHA_SHIFT = 0x00000004,
// };
const uint MASK_EFFECT_NONE = 1;
const uint MASK_EFFECT_DARKEN = 2;
const uint MASK_EFFECT_FILTER = 3;
const uint MASK_EFFECT_ALPHA_SHIFT = 4;

// 输出颜色
out vec4 FragColor;

void main() {
    if (u_IsDrawingWireframe) {
        // FragColor = vec4(1.0, 1.0, 0.0, 1.0);
        FragColor = vec4(float(u_ActiveMaskLayerCount));
    } else {
        // 采样纹理并混矩形默认颜色
        vec4 texcolor = texture(u_samplerarray, vec3(v_TexCoord, f_TextureLayerIdx)) * f_DefColor;
        if (f_NoFilter == 0) {
            // 应用蒙版
            for (int mask_index = 0; mask_index < u_ActiveMaskLayerCount; ++mask_index) {
                MaskLayer mask = u_MaskStack[mask_index];
                bool is_inside_mask =
                    v_WorldPos.x > mask.rect.x && v_WorldPos.x < mask.rect.z &&
                        v_WorldPos.y > mask.rect.y && v_WorldPos.y < mask.rect.w;
                if (is_inside_mask) {
                    switch (mask.effect) {
                        case MASK_EFFECT_NONE:
                        {
                            break;
                        }
                        case MASK_EFFECT_DARKEN:
                        {
                            // 暗化蒙版
                            texcolor.rgb *= (1.0 - mask.effectParams.x);
                            break;
                        }
                        case MASK_EFFECT_FILTER:
                        {
                            // 滤镜蒙版
                            texcolor *= mask.effectParams;
                            break;
                        }
                        case MASK_EFFECT_ALPHA_SHIFT:
                        {
                            texcolor.a *= mask.effectParams.x;
                            break;
                        }
                    }
                }
            }
        }
        FragColor = texcolor;
    }
}
