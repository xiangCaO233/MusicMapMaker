#version 450 core

// 渲染管线前传递来的颜色
in vec4 fill_color;

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

// 渲染管线前传递来的采样器数据
in float texture_policy;
in float texture_id;

void main() {
  FragColor = fill_color;
}
