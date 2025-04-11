#version 450 core

in vec2 glyph_uv;
in float glyph_id;

uniform sampler2DArray glyph_atlas_metadata;
uniform sampler2DArray glyph_atlas_array;

out vec4 FragColor;

void main() { FragColor = vec4(1.0); }
