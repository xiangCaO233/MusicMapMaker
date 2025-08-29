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
// 图元类型
layout(location = 13) in uint aPrimitive;

// 输出到几何着色器
// --- 输出：打包到接口块中，传递给几何着色器 ---
out VS_OUT {
    // 只有 gl_Position 是内置变量，不能放进块里

    // 把所有 flat 变量都放进这个块
    flat vec2 size;
    flat float rotation;
    flat vec4 color;
    flat vec2 radius;
    flat float radiusEffectParam;
    flat uint radiusEffect;
    flat vec2 uvScale;
    flat vec2 groupSize;
    flat int textureLayerIdx;
    flat uint noFilter;
    flat uint texAlignStratergy;
    flat uint texScaleStratergy;
    flat uint primitiveType;
} vs_out; // 块的实例名为 vs_out

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);

    // 将实例信息传递给几何着色器(非插值)
    // 为接口块的成员赋值
    vs_out.size = aScale;
    vs_out.rotation = aRotation;
    vs_out.color = aColor;
    vs_out.radius = aRadius;
    vs_out.radiusEffectParam = aRadiusEffectParam;
    vs_out.radiusEffect = aRadiusEffect;
    vs_out.uvScale = aUVScale;
    vs_out.groupSize = aGroupSize;
    vs_out.textureLayerIdx = aTextureLayerIdx;
    vs_out.noFilter = aNoFilter;
    vs_out.texAlignStratergy = aTexAlignStratergy;
    vs_out.texScaleStratergy = aTexScaleStratergy;
    vs_out.primitiveType = aPrimitive;
}
