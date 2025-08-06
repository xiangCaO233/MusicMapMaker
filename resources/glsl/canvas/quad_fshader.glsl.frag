#version 410 core

// --- 输出/输入/Uniforms ---
out vec4 FragColor;

uniform bool u_IsDrawingWireframe;
// 使用的采样数组
uniform sampler2DArray u_samplerarray;

// 矩形的像素尺寸
flat in vec2 f_QuadSize;

// 传输来的片段纹理信息
in vec2 v_TexCoord;
in vec2 v_WorldPos;

// 纹理在层中的尺寸比例
flat in vec2 f_UVScale;
// 纹理层数
flat in uint f_TextureLayerIdx;
flat in uint f_NoFilter;
// 默认颜色
flat in vec4 f_DefColor;

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
const uint TILING_AUTO = 1u;
const uint SCALE_BASEWIDTH_TO_TILING = 2u;
const uint SCALE_BASEHEIGHT_TO_TILING = 3u;
const uint SCALE_FILL = 4u;
const uint SCALE_BASEWIDTH_REPEAT_FILL = 5u;
const uint SCALE_BASEHEIGHT_REPEAT_FILL = 6u;
const uint NO_SCALE_TILING = 7u;

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

// [优化] 定义一个返回值结构体, 用于封装UV坐标和片段可见性
// 这样做可以避免在函数内部使用高成本的 `discard` 指令
struct UVResult {
    bool visible; // 片段是否可见 (是否在纹理的有效区域内)
    vec2 uv; // 计算出的采样UV坐标
};

UVResult calcUV() {
    UVResult result;
    result.visible = true; // 默认片段是可见的
    result.uv = vec2(0.0); // 初始化UV

    // [优化] 提取所有 case 分支中重复的计算，只执行一次
    // 同时检查纹理是否有效，无效则直接标记为不可见
    if (f_UVScale.x == 0.0 || f_UVScale.y == 0.0 || f_GroupSize.x == 0.0 || f_GroupSize.y == 0.0) {
        result.visible = false;
        return result;
    }
    vec2 actualTexSize = f_GroupSize * f_UVScale;
    vec2 fragPosInQuad = v_TexCoord * f_QuadSize;

    switch (f_TexScaleStratergy) {
        case TILING_AUTO:
        {
            // 2. 计算宽高缩放比
            vec2 scaleRatios = f_QuadSize / actualTexSize;
            // 3. 选择较大的缩放比作为最终的统一缩放因子
            float finalScale = max(scaleRatios.x, scaleRatios.y);
            // 4. 计算缩放后纹理的尺寸
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 5. 计算居中对齐的偏移量
            vec2 offset = (f_QuadSize - scaledTexSize) / 2.0;
            // 7. 计算在源纹理上的采样像素坐标
            vec2 samplePosInTex = (fragPosInQuad - offset) / finalScale;
            // 8. 转换为UV坐标并应用平铺
            vec2 tiledUV = fract(samplePosInTex / actualTexSize);
            // 9. 将平铺后的 UV (0-1范围) 映射到纹理层内的实际区域
            result.uv = tiledUV * f_UVScale;
            break;
        }
        case SCALE_BASEWIDTH_TO_TILING:
        {
            // 2. 计算宽度基准的缩放因子
            float finalScale = f_QuadSize.x / actualTexSize.x;
            // 3. 计算缩放后纹理的尺寸
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 4. 根据对齐策略，计算垂直方向的偏移量
            float offsetY = 0.0;
            if (f_TexAlignStratergy == ALIGN_TOP) {
                offsetY = 0.0;
            } else if (f_TexAlignStratergy == ALIGN_BOTTOM) {
                offsetY = f_QuadSize.y - scaledTexSize.y;
            } else { // 默认居中对齐
                offsetY = (f_QuadSize.y - scaledTexSize.y) / 2.0;
            }

            // 6. 判断片段是否在垂直对齐后的纹理区域内
            if (fragPosInQuad.y < offsetY || fragPosInQuad.y > (offsetY + scaledTexSize.y))
            {
                // 在留白区域, 标记为不可见
                result.visible = false;
            }
            else
            {
                // 7. 计算在源纹理上的采样像素坐标
                vec2 posRelativeToTex = fragPosInQuad - vec2(0.0, offsetY);
                vec2 samplePosInTex = posRelativeToTex / finalScale;
                // 8. 转换为UV坐标并应用平铺
                vec2 tiledUV = fract(samplePosInTex / actualTexSize);
                // 9. 将平铺后的 UV 映射到纹理层内的实际区域
                result.uv = tiledUV * f_UVScale;
            }
            break;
        }
        case SCALE_BASEHEIGHT_TO_TILING:
        {
            // 2. 计算高度基准的缩放因子
            float finalScale = f_QuadSize.y / actualTexSize.y;
            // 3. 计算缩放后纹理的尺寸
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 4. 根据对齐策略，计算水平方向的偏移量
            float offsetX = 0.0;
            if (f_TexAlignStratergy == ALIGN_LEFT) {
                offsetX = 0.0;
            } else if (f_TexAlignStratergy == ALIGN_RIGHT) {
                offsetX = f_QuadSize.x - scaledTexSize.x;
            } else { // 默认居中对齐
                offsetX = (f_QuadSize.x - scaledTexSize.x) / 2.0;
            }

            // 6. 判断片段是否在水平对齐后的纹理区域内
            if (fragPosInQuad.x < offsetX || fragPosInQuad.x > (offsetX + scaledTexSize.x))
            {
                // 在留白区域, 标记为不可见
                result.visible = false;
            }
            else
            {
                // 7. 计算在源纹理上的采样像素坐标
                vec2 posRelativeToTex = fragPosInQuad - vec2(offsetX, 0.0);
                vec2 samplePosInTex = posRelativeToTex / finalScale;
                // 8. 转换为UV坐标并应用平铺
                vec2 tiledUV = fract(samplePosInTex / actualTexSize);
                // 9. 将平铺后的 UV 映射到纹理层内的实际区域
                result.uv = tiledUV * f_UVScale;
            }
            break;
        }
        case SCALE_FILL:
        {
            // 将此UV坐标乘以f_UVScale，以确保我们只在纹理的有效区域内进行采样。
            result.uv = v_TexCoord * f_UVScale;
            break;
        }
        case SCALE_BASEWIDTH_REPEAT_FILL:
        {
            // 1. 计算缩放
            float finalScale = f_QuadSize.x / actualTexSize.x;
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 2. 根据对齐策略，确定主图像的垂直偏移 (平铺基准点)
            float offsetY = 0.0;
            if (f_TexAlignStratergy == ALIGN_TOP) {
                offsetY = 0.0;
            } else if (f_TexAlignStratergy == ALIGN_BOTTOM) {
                offsetY = f_QuadSize.y - scaledTexSize.y;
            } else { // 默认居中
                offsetY = (f_QuadSize.y - scaledTexSize.y) / 2.0;
            }
            // 3. 计算片段相对于平铺基准点的位置
            vec2 posRelativeToPrimaryTile = fragPosInQuad - vec2(0.0, offsetY);
            // 4. 使用 mod() 实现无限平铺
            vec2 posWithinAnyTile = mod(posRelativeToPrimaryTile, scaledTexSize);
            // 5. 将平铺后的坐标转换回原始纹理的UV空间
            vec2 samplePosInTex = posWithinAnyTile / finalScale;
            vec2 finalUV_normalized = samplePosInTex / actualTexSize;
            // 6. 映射到层内实际区域
            result.uv = finalUV_normalized * f_UVScale;
            break;
        }
        case SCALE_BASEHEIGHT_REPEAT_FILL:
        {
            // 1. 计算缩放
            float finalScale = f_QuadSize.y / actualTexSize.y;
            vec2 scaledTexSize = actualTexSize * finalScale;
            // 2. 根据对齐策略，确定主图像的水平偏移 (平铺基准点)
            float offsetX = 0.0;
            if (f_TexAlignStratergy == ALIGN_LEFT) {
                offsetX = 0.0;
            } else if (f_TexAlignStratergy == ALIGN_RIGHT) {
                offsetX = f_QuadSize.x - scaledTexSize.x;
            } else { // 默认居中
                offsetX = (f_QuadSize.x - scaledTexSize.x) / 2.0;
            }
            // 3. 计算片段相对于平铺基准点的位置
            vec2 posRelativeToPrimaryTile = fragPosInQuad - vec2(offsetX, 0.0);
            // 4. 使用 mod() 实现无限平铺
            vec2 posWithinAnyTile = mod(posRelativeToPrimaryTile, scaledTexSize);
            // 5. 将平铺后的坐标转换回原始纹理的UV空间
            vec2 samplePosInTex = posWithinAnyTile / finalScale;
            vec2 finalUV_normalized = samplePosInTex / actualTexSize;
            // 6. 映射到层内实际区域
            result.uv = finalUV_normalized * f_UVScale;
            break;
        }
        case NO_SCALE_TILING:
        {
            // 2. 根据对齐策略，计算纹理图块的偏移量
            // [优化] 使用位运算(&)来正确处理组合对齐标志, 但为了保持与原逻辑一致, 这里暂不修改
            // 水平对齐 (检查 LEFT 和 RIGHT)
            float offsetX = 0.0;
            if (f_TexAlignStratergy == ALIGN_LEFT) {
                offsetX = 0.0;
            } else if (f_TexAlignStratergy == ALIGN_RIGHT) {
                offsetX = f_QuadSize.x - actualTexSize.x;
            } else { // 默认水平居中
                offsetX = (f_QuadSize.x - actualTexSize.x) / 2.0;
            }
            // 垂直对齐 (检查 TOP 和 BOTTOM)
            float offsetY = 0.0;
            if (f_TexAlignStratergy == ALIGN_TOP) {
                offsetY = 0.0;
            } else if (f_TexAlignStratergy == ALIGN_BOTTOM) {
                offsetY = f_QuadSize.y - actualTexSize.y;
            } else { // 默认垂直居中
                offsetY = (f_QuadSize.y - actualTexSize.y) / 2.0;
            }

            // 3. 判断片段是否在图块的边界内
            if (fragPosInQuad.x >= offsetX && fragPosInQuad.x < (offsetX + actualTexSize.x) &&
                    fragPosInQuad.y >= offsetY && fragPosInQuad.y < (offsetY + actualTexSize.y))
            {
                // 4. 如果在边界内, 计算采样UV
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
    }
    return result;
}

void main() {
    if (u_IsDrawingWireframe) {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }

    // [优化] 调整主逻辑流程, 先计算UV并判断可见性
    UVResult uvResult = calcUV();

    // 如果片段在纹理的留白区域，提前丢弃，可以避免昂贵的 texture() 调用和后续所有计算
    if (!uvResult.visible) {
        discard;
    }

    // 采样纹理并混矩形默认颜色
    vec4 texcolor = texture(u_samplerarray, vec3(uvResult.uv, f_TextureLayerIdx)) * f_DefColor;

    // 如果采样出的颜色是完全透明的，也可以提前丢弃以优化
    if (texcolor.a == 0.0) {
        discard;
    }

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
                        // [微优化建议] C++端可以预先计算 (1.0 - darken_ratio) 并传入
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
