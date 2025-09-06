#version 410 core

// 混成纹理源
uniform sampler2D[8] sources;
uniform int active_sources;
// 辉光强度
uniform float bloomIntensity;

in vec2 vTexCoord;

out vec4 FragColor;

void main() {
    vec4 sceneColor = texture(sources[0], vTexCoord);
    vec4 finalColor = sceneColor;
    for (int i = 1; i < active_sources; i++) {
        vec4 bloomColor = texture(sources[i], vTexCoord);
        // 将辉光颜色乘以强度后再相加
        finalColor = finalColor + bloomColor * bloomIntensity;
    }

    FragColor = vec4(finalColor.rgb, sceneColor.a);
}
