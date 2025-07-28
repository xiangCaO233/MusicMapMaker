#version 410 core

// 声道绘制颜色
uniform vec4 channel_color;

out vec4 FragColor;

void main() {
    FragColor = vec4(channel_color.rgb, 0.4);
}
