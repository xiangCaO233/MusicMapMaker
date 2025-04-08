#version 450 core

// 投影矩阵
uniform mat4 projection_mat;
uniform float texture_pool_type;

// 顶点数据
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 vuv;

// 矩形数据
layout(location = 2) in vec2 shape_pos;
layout(location = 3) in vec2 shape_size;
layout(location = 4) in float shape_rotation;
layout(location = 5) in float shape_texture_policy;
layout(location = 6) in float shape_texture_id;
layout(location = 7) in vec4 shape_fillcolor;

// 采样器数据
// 填充颜色
out vec4 fill_color;
// 图形纹理填充策略
out float texture_policy;
// 图形纹理id
out float texture_id;
// 预处理图形纹理uv
out vec2 texture_uv;

void main() {
  // 缩放矩形到指定大小
  vec2 scaled_pos = vpos.xy * shape_size * 0.5;

  // 旋转矩形
  float angle_rad = radians(shape_rotation);
  float cos_angle = cos(angle_rad);
  float sin_angle = sin(angle_rad);
  mat2 rotation_matrix = mat2(cos_angle, -sin_angle, sin_angle, cos_angle);
  vec2 rotated_pos = rotation_matrix * scaled_pos;

  // 传递纹理uv
  texture_uv = vuv;

  // 平移到指定位置
  vec2 final_pos = rotated_pos + shape_pos + shape_size / 2;
  // vec2 final_pos = shape_pos + scaled_pos;

  // 输出填充颜色
  fill_color = shape_fillcolor;
  // color = vec4(final_pos.x/100.0, final_pos.x/100.0, 1.0, 1.0);

  // 应用视图和投影矩阵
  vec4 glpos = projection_mat * vec4(final_pos, 0.0, 1.0);
  gl_Position = vec4(glpos.x - 1.0, glpos.y + 1.0, glpos.zw);
}
