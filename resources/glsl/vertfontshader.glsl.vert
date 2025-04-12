#version 450 core

// 投影矩阵
uniform mat4 projection_mat;
// uniform float viewport_width;
// uniform float viewport_height;

layout(location = 0) in vec2 vpos;
layout(location = 1) in vec2 vuv;

layout(location = 2) in vec2 character_pos;
layout(location = 3) in vec2 character_size;
layout(location = 4) in float character_rotation;
layout(location = 5) in float character_glyph_id;
layout(location = 6) in vec4 character_color;
// layout(location = 7) in float character_yoffset;

// character_uv[7] 会占用连续的 locations 7~10
layout(location = 7) in vec2 character_uvs[4];

const float pi = 3.14159265;

out vec4 glyph_color;
out vec2 glyph_uv;
out float glyph_id;

void main() {
  // 缩放矩形到指定大小
  vec2 scaled_pos = vpos.xy * character_size * 0.5;

  // 旋转矩形
  float angle_rad = radians(character_rotation);
  float cos_angle = cos(angle_rad);
  float sin_angle = sin(angle_rad);
  mat2 rotation_matrix = mat2(cos_angle, -sin_angle, sin_angle, cos_angle);
  vec2 rotated_pos = rotation_matrix * scaled_pos;

  // 平移到指定位置
  vec2 final_pos = rotated_pos + character_pos + character_size / 2;
  // final_pos.y = viewport_height - final_pos.y - character_size.y;

  // 应用视图和投影矩阵
  vec4 glpos = projection_mat * vec4(final_pos.x, final_pos.y, 0.0, 1.0);
  gl_Position = vec4(glpos.x - 1.0, glpos.y + 1.0, glpos.zw);

  // 选择uv光栅化
  glyph_uv = character_uvs[gl_VertexID];
  // glyph_uv = vuv;

  glyph_id = character_glyph_id;
  glyph_color = character_color;
}
