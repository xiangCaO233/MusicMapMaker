#version 410 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_texture;
uniform float u_min_db;
uniform float u_max_db;

// 将归一化的值 [0, 1] 映射到一个颜色梯度
vec3 getColor(float t) {
    t = clamp(t, 0.0, 1.0);
    // 一个经典的黑 -> 紫 -> 红 -> 黄 的梯度
    if (t < 0.5) {
        return mix(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0), t * 2.0);
    } else {
        return mix(vec3(1.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0), (t - 0.5) * 2.0);
    }
}

void main() {
    // 从纹理中获取dB值 (注意是 .r 因为我们用的是单通道红色纹理)
    float db = texture(u_texture, TexCoords).r;

    // 将dB值归一化到 [0, 1] 范围
    float normalized_val = (db - u_min_db) / (u_max_db - u_min_db);

    vec3 color = getColor(normalized_val);
    FragColor = vec4(color, 1.0);
}
