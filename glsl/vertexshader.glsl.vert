#version 410 core
// 顶点数据
layout (location = 0) in vec3 vpos;
layout (location = 1) in vec2 vuv;

// 矩形数据

void main() {
  gl_Position = vec4(vpos.xyz, 1.0);  
}
