#version 410 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

// Uniform 投影矩阵
uniform mat4 projection;

// 输出纹理信息
out vec2 v_TexCoord;
out vec2 v_WorldPos;

void main() {}
