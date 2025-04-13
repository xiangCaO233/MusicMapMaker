#version 450 core

in vec4 glyph_color;
in vec2 glyph_uv;
in float glyph_id;

uniform sampler2DArray glyph_atlas_array;

out vec4 FragColor;

void main() {
  float layer = float(int(glyph_id) >> 16);
  vec3 uv3d = vec3(glyph_uv / 1024.0, layer);
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(glyph_atlas_array, uv3d).r);
  FragColor = glyph_color * sampled;
  // FragColor = vec4(glyph_uv.x / 2);
}
