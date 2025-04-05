#version 450 core

// 渲染管线前传递来的颜色
in vec4 color;

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

void main() {
  FragColor = color;
}
