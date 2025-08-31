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

// 矩形的像素尺寸
flat in vec2 f_QuadSize;

// 传输来的片段纹理信息
in vec2 v_TexCoord;
in vec2 v_WorldPos;

// 纹理在层中的尺寸比例
flat in vec2 f_UVScale;
// 纹理层数
flat in int f_TextureLayerIdx;
flat in uint f_NoFilter;
// 默认颜色
flat in vec4 f_DefColor;

// 圆角效果
flat in vec2 f_Radius;
flat in float f_RadiusEffectParam;
flat in uint f_RadiusEffect;

// 纹理贴图策略
flat in uint f_TexScaleStratergy;
flat in uint f_TexAlignStratergy;
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

// --- C++端 enum 值的常量定义 ---
// TexScaleMode
// enum class TexScaleMode : uint32_t {
//     // 自动缩放并裁切(确保填满矩形,并保持比例的缩放,始终对齐中心)
//     AUTO_SCALE_AND_CUT = 0x00000001,
//     // 强行根据宽度缩放(确保填满矩形宽度(不确保填满矩形),基于此宽度的缩放保持图像比例等比缩放高度)
//     SCALE_FORCE_BASEWIDTH = 0x00000002,
//     // 强行根据高度缩放(确保填满矩形高度(不确保填满矩形),基于此高度的缩放保持图像比例等比缩放宽度)
//     SCALE_FORCE_BASEHEIGHT = 0x00000003,
//     // 强制填充(不保持比例,直接用图像塞满矩形)
//     FORCE_FILL = 0x00000004,
//     // 直接平铺(确保填满矩形宽度,保持比例,保持尺寸,不够的高度使用重采样,确保最终填满矩形)
//     TILE_REPEAT = 0x00000005,
//     // 根据宽度平铺(确保填满矩形宽度,保持比例,不够的高度使用重采样,确保最终填满矩形)
//     TILE_BASEWIDTH_REPEAT = 0x00000006,
//     // 根据高度平铺(确保填满矩形高度,保持比例,不够的宽度使用重采样,确保最终填满矩形)
//     TILE_BASEHEIGHT_REPEAT = 0x00000007,
//     // 单独放一个图像(保持比例,根据对齐方式铺过去,不够则不填充颜色(无重采样),图像过大则应只显示一部分,需要配合对齐方式指定对齐位置)
//     SINGLE = 0x00000008,
//     仅绘制字符(不可使用圆角属性)-此情况下圆角半径属性存储的是uvOffset
//     CHARACTER = 0x00000009,
// };
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
// enum class RadiusEffect : uint32_t {
//     // 淡入淡出
//     FADE_IN_AND_OUT = 0x00000001,
// };
const uint FADE_IN_AND_OUT = 1u;
// 发光
const uint GLOWING = 2u;

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
    result.uv = vec2(0.);

    // 为 CHARACTER 模式添加特殊处理, 它不依赖GroupSize
    if (f_TexScaleStratergy != CHARACTER && (f_UVScale.x == 0. || f_UVScale.y == 0. || f_GroupSize.x == 0. || f_GroupSize.y == 0.)) {
        result.visible = false;
        return result;
    }

    vec2 actualTexSize;
    vec2 fragPosInQuad;
    // [优化] CHARACTER模式不需要这两个变量，可以跳过计算
    if (f_TexScaleStratergy != CHARACTER) {
        actualTexSize = f_GroupSize * f_UVScale;
        fragPosInQuad = v_TexCoord * f_QuadSize;
    }

    switch (f_TexScaleStratergy) {
        case AUTO_SCALE_AND_CUT:
        {
            // 计算宽高缩放比
            vec2 scaleRatios = f_QuadSize / actualTexSize;
            // 选择较大的缩放比作为最终的统一缩放因子
            float finalScale = max(scaleRatios.x, scaleRatios.y);
            // 计算缩放后纹理的尺寸
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 计算居中对齐的偏移量
            vec2 offset = (f_QuadSize - scaledTexSize) / 2.;
            // 计算在源纹理上的采样像素坐标
            vec2 samplePosInTex = (fragPosInQuad - offset) / finalScale;
            // 转换为UV坐标并应用平铺
            vec2 tiledUV = fract(samplePosInTex / actualTexSize);
            // 将平铺后的 UV (0-1范围) 映射到纹理层内的实际区域
            result.uv = tiledUV * f_UVScale;
            break;
        }
        case SCALE_FORCE_BASEWIDTH:
        {
            // 计算宽度基准的缩放因子
            float finalScale = f_QuadSize.x / actualTexSize.x;
            // 计算缩放后纹理的尺寸
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 根据对齐策略，计算垂直方向的偏移量
            float offsetY = 0.;
            if (f_TexAlignStratergy == ALIGN_TOP) {
                offsetY = 0.;
            } else if (f_TexAlignStratergy == ALIGN_BOTTOM) {
                offsetY = f_QuadSize.y - scaledTexSize.y;
            } else { // 默认居中对齐
                offsetY = (f_QuadSize.y - scaledTexSize.y) / 2.;
            }

            // 判断片段是否在垂直对齐后的纹理区域内
            if (fragPosInQuad.y < offsetY || fragPosInQuad.y > (offsetY + scaledTexSize.y))
            {
                // 在留白区域, 标记为不可见
                result.visible = false;
            }
            else
            {
                // 计算在源纹理上的采样像素坐标
                vec2 posRelativeToTex = fragPosInQuad - vec2(0., offsetY);
                vec2 samplePosInTex = posRelativeToTex / finalScale;
                // 转换为UV坐标并应用平铺
                vec2 tiledUV = fract(samplePosInTex / actualTexSize);
                // 将平铺后的 UV 映射到纹理层内的实际区域
                result.uv = tiledUV * f_UVScale;
            }
            break;
        }
        case SCALE_FORCE_BASEHEIGHT:
        {
            // 计算高度基准的缩放因子
            float finalScale = f_QuadSize.y / actualTexSize.y;
            // 计算缩放后纹理的尺寸
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 根据对齐策略，计算水平方向的偏移量
            float offsetX = 0.;
            if (f_TexAlignStratergy == ALIGN_LEFT) {
                offsetX = 0.;
            } else if (f_TexAlignStratergy == ALIGN_RIGHT) {
                offsetX = f_QuadSize.x - scaledTexSize.x;
            } else {
                // 默认居中对齐
                offsetX = (f_QuadSize.x - scaledTexSize.x) / 2.;
            }

            // 判断片段是否在水平对齐后的纹理区域内
            if (fragPosInQuad.x < offsetX || fragPosInQuad.x > (offsetX + scaledTexSize.x))
            {
                // 在留白区域, 标记为不可见
                result.visible = false;
            }
            else
            {
                // 计算在源纹理上的采样像素坐标
                vec2 posRelativeToTex = fragPosInQuad - vec2(offsetX, 0.);
                vec2 samplePosInTex = posRelativeToTex / finalScale;
                // 转换为UV坐标并应用平铺
                vec2 tiledUV = fract(samplePosInTex / actualTexSize);
                // 将平铺后的 UV 映射到纹理层内的实际区域
                result.uv = tiledUV * f_UVScale;
            }
            break;
        }
        case FORCE_FILL:
        {
            // 将此UV坐标乘以f_UVScale，确保只在纹理的有效区域内进行采样
            result.uv = v_TexCoord * f_UVScale;
            break;
        }
        case TILE_REPEAT:
        {
            // 直接以纹理的原始尺寸(actualTexSize)为单位进行平铺
            // 使用 mod 计算片段在原始纹理尺寸单元内的相对像素位置
            vec2 samplePosInTex = mod(fragPosInQuad, actualTexSize);
            // 将像素位置转换为归一化的UV坐标 [0,1]
            vec2 tiledUV = samplePosInTex / actualTexSize;
            // 映射到层内实际区域
            result.uv = tiledUV * f_UVScale;
            break;
        }
        case TILE_BASEWIDTH_REPEAT:
        {
            // 计算缩放
            float finalScale = f_QuadSize.x / actualTexSize.x;
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 根据对齐策略，确定主图像的垂直偏移 (平铺基准点)
            float offsetY = 0.;
            if (f_TexAlignStratergy == ALIGN_TOP) {
                offsetY = 0.;
            } else if (f_TexAlignStratergy == ALIGN_BOTTOM) {
                offsetY = f_QuadSize.y - scaledTexSize.y;
            } else {
                // 默认居中
                offsetY = (f_QuadSize.y - scaledTexSize.y) / 2.;
            }
            // 计算片段相对于平铺基准点的位置
            vec2 posRelativeToPrimaryTile = fragPosInQuad - vec2(0., offsetY);
            // 使用 mod() 实现无限平铺
            vec2 posWithinAnyTile = mod(posRelativeToPrimaryTile, scaledTexSize);
            // 将平铺后的坐标转换回原始纹理的UV空间
            vec2 samplePosInTex = posWithinAnyTile / finalScale;
            vec2 finalUV_normalized = samplePosInTex / actualTexSize;
            // 映射到层内实际区域
            result.uv = finalUV_normalized * f_UVScale;
            break;
        }
        case TILE_BASEHEIGHT_REPEAT:
        {
            // 计算缩放
            float finalScale = f_QuadSize.y / actualTexSize.y;
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 根据对齐策略，确定主图像的水平偏移 (平铺基准点)
            float offsetX = 0.;
            if (f_TexAlignStratergy == ALIGN_LEFT) {
                offsetX = 0.;
            } else if (f_TexAlignStratergy == ALIGN_RIGHT) {
                offsetX = f_QuadSize.x - scaledTexSize.x;
            } else {
                // 默认居中
                offsetX = (f_QuadSize.x - scaledTexSize.x) / 2.;
            }
            // 计算片段相对于平铺基准点的位置
            vec2 posRelativeToPrimaryTile = fragPosInQuad - vec2(offsetX, 0.);
            // 使用 mod() 实现无限平铺
            vec2 posWithinAnyTile = mod(posRelativeToPrimaryTile, scaledTexSize);
            // 将平铺后的坐标转换回原始纹理的UV空间
            vec2 samplePosInTex = posWithinAnyTile / finalScale;
            vec2 finalUV_normalized = samplePosInTex / actualTexSize;
            // 映射到层内实际区域
            result.uv = finalUV_normalized * f_UVScale;
            break;
        }
        case SINGLE:
        {
            // 根据对齐策略，计算纹理图块的偏移量
            // 水平对齐 (检查 LEFT 和 RIGHT)
            float offsetX = 0.;
            if (f_TexAlignStratergy == ALIGN_LEFT) {
                offsetX = 0.;
            } else if (f_TexAlignStratergy == ALIGN_RIGHT) {
                offsetX = f_QuadSize.x - actualTexSize.x;
            } else {
                // 默认水平居中
                offsetX = (f_QuadSize.x - actualTexSize.x) / 2.;
            }
            // 垂直对齐 (检查 TOP 和 BOTTOM)
            float offsetY = 0.;
            if (f_TexAlignStratergy == ALIGN_TOP) {
                offsetY = 0.;
            } else if (f_TexAlignStratergy == ALIGN_BOTTOM) {
                offsetY = f_QuadSize.y - actualTexSize.y;
            } else { // 默认垂直居中
                offsetY = (f_QuadSize.y - actualTexSize.y) / 2.;
            }

            // 判断片段是否在图块的边界内
            if (fragPosInQuad.x >= offsetX && fragPosInQuad.x < (offsetX + actualTexSize.x) &&
                    fragPosInQuad.y >= offsetY && fragPosInQuad.y < (offsetY + actualTexSize.y))
            {
                // 如果在边界内, 计算采样UV
                vec2 samplePosInTex = fragPosInQuad - vec2(offsetX, offsetY);
                vec2 uv = samplePosInTex / actualTexSize;
                // 映射到层内实际区域
                result.uv = uv * f_UVScale;
            }
            else
            {
                // 片段在图块之外，标记为不可见
                result.visible = false;
            }
            break;
        }
        case CHARACTER:
        {
            // 仅绘制字符模式:
            // 在此模式下, f_Radius 被复用为 uvOffset (字符在图集中的UV偏移量)。
            // f_UVScale 是字符的UV尺寸。
            // v_TexCoord 是在字符矩形上的插值坐标(0-1)。
            // 最终UV = 偏移量 + 插值坐标 * 尺寸
            result.uv = f_Radius + (v_TexCoord * f_UVScale);
            break;
        }
    }
    return result;
}

/**
* @brief 计算一个点到圆角矩形的有向距离 (SDF).
* @param p 点的坐标, 相对于矩形中心.
* @param b 矩形的半尺寸 (half-extents).
* @param r 圆角的半径.
* @return float 距离值. 负数在内部, 0在边缘, 正数在外部.
*/
float sdRoundedBox(vec2 p, vec2 b, vec2 r) {
    r = (p.x > 0.) ? r : vec2(r.x, r.y);
    r = (p.y > 0.) ? r : vec2(r.x, r.y);
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.) + length(max(q, 0.)) - r.x;
}

void main() {
    if (u_IsDrawingWireframe) {
        out_SceneColor = vec4(1., 1., 0., 1.);
        out_GlowMask = vec4(0., 0., 0., 1.);
        return;
    }
    // else {
    //     FragColor = vec4(0.0, 1.0, 1.0, 1.0);
    //     return;
    // }
    vec4 finalColor;
    if (f_TextureLayerIdx == -1) {
        finalColor = f_DefColor;
    } else {
        // 先计算UV并判断可见性
        UVResult uvResult = calcUV();

        // 如果片段在纹理的留白区域，提前丢弃，避免昂贵的 texture() 调用和后续所有计算
        if (!uvResult.visible) {
            discard;
        }

        // 采样纹理并混矩形默认颜色
        if (f_TexScaleStratergy == CHARACTER) {
            // 对于单通道位图字体:
            // 从R通道获取字符的覆盖率/Alpha值
            float coverage = texture(u_samplerarray, vec3(uvResult.uv, f_TextureLayerIdx)).r;

            // 使用 f_DefColor 作为基础颜色，并用覆盖率调制其Alpha通道
            finalColor = vec4(f_DefColor.rgb, f_DefColor.a * coverage);
        } else {
            // 对于所有其他RGBA纹理模式，使用原有逻辑
            finalColor = texture(u_samplerarray, vec3(uvResult.uv, f_TextureLayerIdx)) * f_DefColor;
        }
    }

    // 如果采样出的颜色是完全透明的，也可以提前丢弃以优化
    if (finalColor.a == 0.) {
        discard;
    }

    if (f_TexScaleStratergy == CHARACTER) {
        out_SceneColor = finalColor;
        vec4 source_glow_mask = texture(glowmask, (projection * vec4(v_TexCoord, 0.0, 1.0)).xy);
        vec4 new_glow_mask;
        if (f_RadiusEffect == GLOWING) {
            new_glow_mask =
                vec4(finalColor.rgb * f_RadiusEffectParam, 1.);
        } else {
            new_glow_mask = vec4(0., 0., 0., 1.);
        }
        out_GlowMask =
            1.0 - (1.0 - source_glow_mask) * (1.0 - new_glow_mask);
        return;
    }

    // --- 圆角和渐变效果处理 ---
    if (f_Radius.x > 0. || f_Radius.y > 0.) {
        // 计算绘制区域的总尺寸 (包含了外部效果区域)
        vec2 drawingAreaSize = f_QuadSize + 2. * f_RadiusEffectParam;

        // 计算当前片元相对于绘制区域中心点的坐标
        vec2 pos = (v_TexCoord - .5) * drawingAreaSize;

        // 计算圆角的像素半径
        // f_Radius 是一个比例值, 乘以矩形半高/半宽得到实际像素半径
        vec2 radiusInPixels = f_Radius * (f_QuadSize / 2.);

        // 计算到圆角矩形的SDF距离
        float dist = sdRoundedBox(pos, f_QuadSize / 2., radiusInPixels);

        // 根据SDF距离和效果类型，计算最终的 alpha 值
        if (f_RadiusEffect == FADE_IN_AND_OUT) {
            // 使用 smoothstep 实现从边缘(dist=0)到外部渐变范围(dist=f_RadiusParams)的平滑过渡
            // 当 dist 从 0 -> f_RadiusParams, alpha 从 1 -> 0
            float fade_alpha = smoothstep(f_RadiusEffectParam, 0., dist);
            finalColor.a *= fade_alpha;
        } else {
            // 没有渐变效果，就是一个硬切的圆角矩形
            if (dist > 0.) {
                discard;
            }
        }

        // 如果经过效果处理后，片元完全透明，则丢弃，避免后续蒙版计算
        if (finalColor.a == 0.) {
            discard;
        }
    }
    // --- 圆角处理结束 ---

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

    vec4 source_glow_mask = texture(glowmask, (projection * vec4(v_TexCoord, 0.0, 1.0)).xy);
    vec4 new_glow_mask;
    if (f_RadiusEffect == GLOWING) {
        new_glow_mask =
            finalColor * f_RadiusEffectParam;
    } else {
        new_glow_mask = vec4(0., 0., 0., 1.);
    }
    out_GlowMask =
        1.0 - (1.0 - source_glow_mask) * (1.0 - new_glow_mask);
}
