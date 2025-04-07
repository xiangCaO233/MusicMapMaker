#version 450 core

// 渲染管线前传递来的颜色
in vec4 fill_color;

// 渲染管线前传递来的采样器数据
in float texture_policy;
in float texture_id;

// 纹理池相关uniform

// 指定纹理池使用方式
// 0-单独使用采样器
// 1-使用纹理图集
// 2-使用同尺寸纹理采样器数组
uniform int texture_pool_usage;

// 使用方式为单独或纹理图集
uniform sampler2D samplers[16];

// 使用方式为纹理采样器数组
uniform sampler2DArray samplerarray;

// 使用纹理数组时的采样器数组偏移地址
uniform int texture_array_offset;

// 渲染颜色结果(向后传输着色结果)
out vec4 FragColor;

void main() {
  FragColor = fill_color;
}
