#version 450 core

// 投影矩阵
uniform mat4 projection_mat;
uniform float texture_pool_type;

// 基本顶点数据
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 vuv;

// 实例矩形数据
layout(location = 2) in vec2 shape_pos;
layout(location = 3) in vec2 shape_size;
layout(location = 4) in float shape_rotation;
layout(location = 5) in float shape_texture_policy;
layout(location = 6) in float shape_texture_id;
layout(location = 7) in vec4 shape_fillcolor;
// 圆角半径
layout(location = 8) in float shape_radius;

// 1-使用纹理图集
uniform int useatlas;

// 采样器数据
// 填充颜色
out vec4 fill_color;
// 图形纹理填充策略
out float texture_policy;
// 图形纹理id
out float texture_id;
// 预处理图形纹理uv
out vec2 texture_uv;
// 当前绘制形状的边界矩形尺寸
out vec2 bound_size;
// 当前绘制形状的边界矩形圆角半径
out float radius;

void main() {
  // 缩放矩形到指定大小
  vec2 scaled_pos = vpos.xy * shape_size * 0.5;

  // 旋转矩形
  float angle_rad = radians(shape_rotation);
  float cos_angle = cos(angle_rad);
  float sin_angle = sin(angle_rad);
  mat2 rotation_matrix = mat2(cos_angle, -sin_angle, sin_angle, cos_angle);
  vec2 rotated_pos = rotation_matrix * scaled_pos;

  // 传递数据
  texture_id = shape_texture_id;
  texture_policy = shape_texture_policy;
  fill_color = shape_fillcolor;
  bound_size = shape_size;

  // 平移到指定位置
  vec2 final_pos = rotated_pos + shape_pos + shape_size / 2;

  // 应用视图和投影矩阵
  vec4 glpos = projection_mat * vec4(final_pos, 0.0, 1.0);
  gl_Position = vec4(glpos.x - 1.0, glpos.y + 1.0, glpos.zw);

  // 传递纹理uv
  texture_uv = vuv;
  // 传递圆角半径
  radius = shape_radius;
}
