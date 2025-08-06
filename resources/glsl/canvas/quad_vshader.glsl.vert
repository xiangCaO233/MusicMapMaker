#version 410 core

// 只接收每个实例的数据
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aScale;
layout(location = 2) in float aRotation;
layout(location = 3) in vec4 aColor;

// 纹理未必占满采样器数组的一个layer
layout(location = 4) in vec2 aUVScale;
// 但是一定是在0,0开始填充的,所以UVoffset恒为0,0
// 贴图所在纹理组的组尺寸
layout(location = 5) in vec2 aGroupSize;

// 纹理ID
layout(location = 6) in uint aTextureLayerIdx;
// 是否禁止蒙版效果
layout(location = 7) in uint aNoFilter;

// 贴图策略
layout(location = 8) in uint aTexAlignStratergy;
layout(location = 9) in uint aTexScaleStratergy;

// Uniform 矩阵
uniform mat4 projection;

// 基本矩形顶点
// 构成矩形的6个顶点的位置 (本地坐标)
const vec2 positions[6] = vec2[6](
        vec2(-0.5, -0.5), // 左下
        vec2(0.5, -0.5), // 右下
        vec2(-0.5, 0.5), // 左上
        vec2(0.5, -0.5), // 右下
        vec2(0.5, 0.5), // 右上
        vec2(-0.5, 0.5) // 左上
    );
// 与上述6个顶点一一对应的UV坐标
const vec2 uvs[6] = vec2[6](
        vec2(0.0, 0.0), // 左下
        vec2(1.0, 0.0), // 右下
        vec2(0.0, 1.0), // 左上
        vec2(1.0, 0.0), // 右下
        vec2(1.0, 1.0), // 右上
        vec2(0.0, 1.0) // 左上
    );

// 输出到片段着色器
out vec2 v_TexCoord;
out vec2 v_WorldPos;

// flat 表示不进行插值
flat out vec2 f_QuadSize;
flat out uint f_TextureLayerIdx;
flat out uint f_NoFilter;
flat out vec2 f_UVScale;
flat out vec2 f_GroupSize;
flat out vec4 f_DefColor;
flat out uint f_TexScaleStratergy;
flat out uint f_TexAlignStratergy;

void main() {

    // 从数组中获取本地坐标和UV
    vec2 localPos = positions[gl_VertexID];
    v_TexCoord = uvs[gl_VertexID];

    // 变换坐标
    // 缩放 (Scale)
    vec2 scaledPos = localPos * vec2(aScale.x, aScale.y);

    // 旋转 (Rotate)
    float c = cos(aRotation);
    float s = sin(aRotation);
    vec2 rotatedPos = vec2(
            scaledPos.x * c - scaledPos.y * s,
            scaledPos.x * s + scaledPos.y * c
        );
    // 平移 (Translate)
    vec2 worldPos = rotatedPos + aPosition;
    v_WorldPos = worldPos;

    // --- 最终位置计算 ---
    // 将我们计算出的2D世界坐标，通过投影矩阵变换到最终的裁剪空间
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);

    // 将实例的纹理信息传递给片段着色器(非插值)
    f_QuadSize = aScale;
    f_TextureLayerIdx = aTextureLayerIdx;
    f_NoFilter = aNoFilter;
    f_UVScale = aUVScale;
    f_GroupSize = aGroupSize;
    f_DefColor = aColor;
    f_TexScaleStratergy = aTexScaleStratergy;
    f_TexAlignStratergy = aTexAlignStratergy;
}
