#version 410 core
// 顶点数据
layout (location = 0) in vec3 vpos;
layout (location = 1) in vec2 vuv;

// 矩形数据
layout (location = 2) in vec3 shape_pos;
layout (location = 3) in vec3 shape_size;
layout (location = 4) in vec3 shape_rotation;
layout (location = 5) in vec3 shape_texture_policy;
layout (location = 6) in vec3 shape_texture_id;
layout (location = 7) in vec3 shape_fillcolor;

void main() {
  gl_Position = vec4(vpos.xyz, 1.0);  
}
