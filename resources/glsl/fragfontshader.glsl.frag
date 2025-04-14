#version 450 core

in vec4 glyph_color;
in vec2 glyph_uv;
in float glyph_layer_index;

uniform sampler2DArray glyph_atlas_array;

out vec4 FragColor;

void main() {
  vec3 uv3d = vec3(glyph_uv / 1024.0, int(glyph_layer_index));
	vec4 color = glyph_color / 255.0;
  FragColor = vec4(color.r , color.g ,
                   color.b , texture(glyph_atlas_array, uv3d).r);
  // FragColor = vec4(glyph_uv.x / 2);
}
