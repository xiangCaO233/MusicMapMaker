#version 410 core

// --- 输出/输入/Uniforms ---
// layout(location = 0) 画到当前FBO的 GL_COLOR_ATTACHMENT0
layout(location = 0) out vec4 out_SceneColor;

// layout(location = 1) 画到当前FBO的 GL_COLOR_ATTACHMENT1
layout(location = 1) out vec4 out_GlowMask;
// Uniform 投影矩阵
uniform mat4 projection;

// 是否绘制线框
uniform bool u_IsDrawingWireframe;

// 使用的采样数组
uniform sampler2DArray u_samplerarray;
// 原始的发光纹理遮罩
uniform sampler2D glowmask;

// 传输来的片段颜色信息
in vec4 v_Color;

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

    // 将顶点UV坐标(0.0-1.0)映射到纹理在层内的实际区域
    result.uv = v_TexCoord * f_UVScale;

    return result;
}

void main() {
    if (u_IsDrawingWireframe) {
        // 只绘制线框
        out_SceneColor = vec4(1.0, 1.0, 0.0, 1.0);
        out_GlowMask = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 并非线框
    vec4 finalColor;
    if (f_TextureLayerIdx == -1) {
        // 无纹理绘制
        finalColor = v_Color;
    } else {
        // 计算UV坐标并获取基础纹理颜色
        UVResult uvResult = calcUV();

        // 如果片段不在有效纹理区域内，直接丢弃，不进行后续计算
        if (!uvResult.visible) {
            discard;
        }

        // 采样纹理并直接混合顶点颜色
        finalColor = texture(u_samplerarray, vec3(uvResult.uv, f_TextureLayerIdx)) * v_Color;

        // 如果采样出的颜色是完全透明的，可以提前丢弃以优化
        if (finalColor.a == 0.0) {
            discard;
        }
    }

    // 蒙版处理
    if (f_NoFilter == 0) {
        // 应用蒙版
        for (int mask_index = 0; mask_index < u_ActiveMaskLayerCount; ++mask_index) {
            MaskLayer mask = u_MaskStack[mask_index];
            bool is_inside_mask =
                v_WorldPos.x >= mask.rect.x && v_WorldPos.x <= mask.rect.z &&
                    v_WorldPos.y >= mask.rect.y && v_WorldPos.y <= mask.rect.w;
            if (is_inside_mask) {
                switch (mask.effect) {
                    case MASK_EFFECT_NONE:
                    {
                        break;
                    }
                    case MASK_EFFECT_DARKEN:
                    {
                        // 暗化蒙版
                        finalColor.rgb *= mask.effectParams.x;
                        break;
                    }
                    case MASK_EFFECT_FILTER:
                    {
                        // 滤镜蒙版
                        finalColor *= mask.effectParams;
                        break;
                    }
                    case MASK_EFFECT_ALPHA_SHIFT:
                    {
                        finalColor.a *= mask.effectParams.x;
                        break;
                    }
                }
            }
        }
    }

    out_SceneColor = finalColor;
    vec4 glow_mask_color = vec4(0.0, 0.0, 0.0, 1.0);
    out_GlowMask = texture(glowmask, (projection * vec4(v_TexCoord, 0.0, 1.0)).xy) + glow_mask_color;
}
