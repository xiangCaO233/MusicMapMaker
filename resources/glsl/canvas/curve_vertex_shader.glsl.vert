#version 410 core
// 顶点位置
layout(location = 0) in vec2 vPosition;
// 顶点uv
layout(location = 1) in vec2 vTexCoord;
// 顶点颜色
layout(location = 2) in vec4 vColor;

// 顶点纹理信息
// 顶点使用的纹理在层中的尺寸比例
layout(location = 3) in vec2 aUVScale;
// 顶点使用的纹理的textureArray的统一尺寸
layout(location = 4) in vec2 aGroupSize;
// 顶点使用的纹理在层中层索引
layout(location = 5) in int aTextureLayerIdx;

// 顶点使用的纹理是否应用ubo蒙版效果
layout(location = 6) in uint aNoFilter;

// Uniform 投影矩阵
uniform mat4 projection;

// 输出颜色信息
out vec4 v_Color;

// 纹理在层中的尺寸比例
// 在textureArray中未必占满整层
// 不过必定绘制在0,0位置(所以可能有留黑)
// 但是每一层尺寸都必须是一样的
flat out vec2 f_UVScale;

// 纹理所处的层数
flat out int f_TextureLayerIdx;
flat out uint f_NoFilter;

// 纹理组尺寸
flat out vec2 f_GroupSize;

void main() {}
