#version 410 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

// >> 新增: 从CPU传入的读取偏移量
uniform float u_read_offset;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    // >> 修改: 调整纹理坐标以实现滚动效果
    // mod(x, 1.0) 确保结果在 [0, 1] 范围内
    TexCoords = vec2(mod(aTexCoords.x + u_read_offset, 1.0), aTexCoords.y);
}
