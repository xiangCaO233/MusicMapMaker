#version 410 core

// 只接收每个实例的数据
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aScale;
layout(location = 2) in float aRotation;

// 纹理ID
layout(location = 3) in int aTextureID;

// Uniform 矩阵
uniform mat4 view;
uniform mat4 projection;

// 基本矩形顶点
vec2 positions[4] = vec2[](
        vec2(-0.5, -0.5), // 左下
        vec2(0.5, -0.5), // 右下
        vec2(-0.5, 0.5), // 左上
        vec2(0.5, 0.5) // 右上
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
flat out int f_TextureID;

void main() {
    // 使用 gl_VertexID (0-5) 来查找当前顶点应该使用哪个角的数据
    int corner_index = indices[gl_VertexID];

    // 从数组中获取本地坐标和UV
    vec2 localPos = positions[corner_index];
    v_TexCoord = uvs[corner_index];

    // 变换坐标
    // 在GPU上即时构建变换矩阵
    float c = cos(aRotation);
    float s = sin(aRotation);
    mat3 model = mat3(
            aScale.x * c, aScale.x * s, 0.0,
            -aScale.y * s, aScale.y * c, 0.0,
            aPosition.x, aPosition.y, 1.0
        );
    vec3 transformedPos = model * vec3(localPos, 1.0);

    // 使用实例数据进行坐标变换
    gl_Position = projection * view * aInstanceModel * vec4(transformedPos, 0.0, 1.0);

    // 将实例的纹理ID传递给片段着色器
    f_TextureID = aTextureID;
}
