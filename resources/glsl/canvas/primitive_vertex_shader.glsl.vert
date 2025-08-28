#version 410 core
// 接收每个图元描述的数据
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aScale;
layout(location = 2) in float aRotation;
layout(location = 3) in vec4 aColor;

// 圆角
layout(location = 4) in vec2 aRadius;
layout(location = 5) in float aRadiusEffectParam;
layout(location = 6) in uint aRadiusEffect;

// 纹理未必占满采样器数组的一个layer
layout(location = 7) in vec2 aUVScale;
// 但是一定是在0,0开始填充的,所以UVoffset恒为0,0
// 贴图所在纹理组的组尺寸
layout(location = 8) in vec2 aGroupSize;

// 纹理ID
layout(location = 9) in int aTextureLayerIdx;
// 是否禁止蒙版效果
layout(location = 10) in uint aNoFilter;

// 贴图策略
layout(location = 11) in uint aTexAlignStratergy;
layout(location = 12) in uint aTexScaleStratergy;

// Uniform 投影矩阵
uniform mat4 projection;

// 输出到几何着色器
// flat 表示不进行插值
flat out vec2 f_QuadSize;
flat out int f_TextureLayerIdx;
flat out uint f_NoFilter;
flat out vec2 f_UVScale;
flat out vec2 f_GroupSize;
flat out vec4 f_DefColor;
flat out vec2 f_Radius;
flat out float f_RadiusEffectParam;
flat out uint f_RadiusEffect;
flat out uint f_TexScaleStratergy;
flat out uint f_TexAlignStratergy;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);

    // 将实例信息传递给几何着色器(非插值)
    f_QuadSize = aScale;
    f_TextureLayerIdx = aTextureLayerIdx;
    f_NoFilter = aNoFilter;
    f_UVScale = aUVScale;
    f_GroupSize = aGroupSize;
    f_DefColor = aColor;
    f_Radius = aRadius;
    f_RadiusEffectParam = aRadiusEffectParam;
    f_RadiusEffect = aRadiusEffect;
    f_TexScaleStratergy = aTexScaleStratergy;
    f_TexAlignStratergy = aTexAlignStratergy;
}
