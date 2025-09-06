#version 410 core

// 基本矩形顶点
// 构成矩形的6个顶点的位置 (本地坐标)
const vec2 positions[6] = vec2[6](
        vec2(-1.0, -1.0), // 左下
        vec2(1.0, -1.0), // 右下
        vec2(-1.0, 1.0), // 左上
        vec2(1.0, -1.0), // 右下
        vec2(1.0, 1.0), // 右上
        vec2(-1.0, 1.0) // 左上
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

out vec2 vTexCoord;

void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    vTexCoord = uvs[gl_VertexID];
}
