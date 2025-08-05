#version 410 core

// 使用的采样数组
uniform sampler2DArray u_samplerarray;

// 传输来的片段纹理信息
in vec2 v_TexCoord;
// 纹理层数
flat in uint f_TextureLayerIdx;
// 默认颜色
flat in vec4 f_DefColor;

// 纹理贴图策略
flat in uint f_TexScaleStratergy;
flat in uint f_TexAlignStratergy;

// 输出颜色
out vec4 FragColor;

void main() {
    vec4 texcolor = texture(u_samplerarray, vec3(v_TexCoord, f_TextureLayerIdx));
    FragColor = texcolor * f_DefColor;
}
