#version 410 core

// 只接收每个实例的数据
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aScale;
layout(location = 2) in float aRotation;
layout(location = 3) in vec4 aColor;

// 纹理未必占满采样器数组的一个layer
layout(location = 4) in vec2 aUVScale;
// 但是一定是在0,0开始填充的,所以UVoffset恒为0,0

// 纹理ID
layout(location = 5) in uint aTextureLayerIdx;

layout(location = 6) in uint aTexScaleStratergy;
layout(location = 7) in uint aTexAlignStratergy;

// Uniform 矩阵
uniform mat4 projection;

// 基本矩形顶点
vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0)
    );
vec2 uvs[4] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );
int indices[6] = int[](0, 1, 2, 1, 3, 2);

// 输出到片段着色器
out vec2 v_TexCoord;

// flat 表示不进行插值
flat out uint f_TextureLayerIdx;
flat out vec4 f_DefColor;
flat out uint f_TexScaleStratergy;
flat out uint f_TexAlignStratergy;

void main() {
    // 使用 gl_VertexID (0-5) 来查找当前顶点应该使用哪个角的数据
    int corner_index = indices[gl_VertexID];

    // 从数组中获取本地坐标和UV
    vec2 localPos = positions[corner_index];
    v_TexCoord = uvs[corner_index] * aUVScale;

    // 变换坐标
    // a. 缩放 (Scale)
    vec2 scaledPos = localPos * vec2(aScale.x, -aScale.y);

    // b. 旋转 (Rotate)
    float c = cos(aRotation);
    float s = sin(aRotation);
    vec2 rotatedPos = vec2(
            scaledPos.x * c - scaledPos.y * s,
            scaledPos.x * s + scaledPos.y * c
        );
    // c. 平移 (Translate)
    vec2 worldPos = rotatedPos + aPosition;

    // --- 最终位置计算 ---
    // 将我们计算出的2D世界坐标，通过投影矩阵变换到最终的裁剪空间
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);

    // 将实例的纹理信息传递给片段着色器
    f_TextureLayerIdx = aTextureLayerIdx;
    f_DefColor = aColor;
    f_TexScaleStratergy = aTexScaleStratergy;
    f_TexAlignStratergy = aTexAlignStratergy;
}
