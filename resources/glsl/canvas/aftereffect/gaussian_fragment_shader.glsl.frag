#version 410 core

// 应用模糊的原始纹理
uniform sampler2D gaussian_source;

in vec2 vTexCoord;

out vec4 FragColor;

uniform bool horizontal;

void main() {
    // 5个权重，用于9点采样 (中心点 + 左右/上下各4个)
    float weight[5];
    weight[0] = 0.227027;
    weight[1] = 0.1945946;
    weight[2] = 0.1216216;
    weight[3] = 0.054054;
    weight[4] = 0.016216;

    vec2 tex_offset = 1.0 / textureSize(gaussian_source, 0); // 获取单个像素的UV大小
    vec3 result = texture(gaussian_source, vTexCoord).rgb * weight[0]; // 中心点采样

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(gaussian_source, vTexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(gaussian_source, vTexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(gaussian_source, vTexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(gaussian_source, vTexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
