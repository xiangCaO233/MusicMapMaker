#version 450 core

layout(location = 0) in vec2 vpos;
layout(location = 1) in vec2 vuv;
layout(location = 2) in vec2 characterid;

out vec2 glyph_uv;
out float glyph_id;

void main() {
  gl_Position = vec4(vpos, 1.0, 1.0);
  glyph_uv = vuv;
}
