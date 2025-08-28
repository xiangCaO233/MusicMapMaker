#version 410 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

// --- 从顶点着色器接收 ---
// GS的输入总是数组
flat in vec2 f_QuadSize[];
flat in int f_TextureLayerIdx[];
flat in uint f_NoFilter[];
flat in vec2 f_UVScale[];
flat in vec2 f_GroupSize[];
flat in vec4 f_DefColor[];
flat in vec2 f_Radius[];
flat in float f_RadiusEffectParam[];
flat in uint f_RadiusEffect[];
flat in uint f_TexScaleStratergy[];
flat in uint f_TexAlignStratergy[];

// --- 传递给片元着色器 ---
out vec2 v_TexCoord;
out vec2 v_WorldPos;
flat out int gs_out_TextureLayerIdx;
flat out uint gs_out_NoFilter;
flat out vec2 gs_out_UVScale;
flat out vec2 gs_out_GroupSize;
flat out vec4 gs_out_DefColor;
flat out vec2 gs_out_Radius;
flat out float gs_out_RadiusEffectParam;
flat out uint gs_out_RadiusEffect;
flat out uint gs_out_TexScaleStratergy;
flat out uint gs_out_TexAlignStratergy;

void main() {
    // 将所有 flat 变量的值“转发”到输出变量
    // 这些值对于由这个点生成的所有顶点都是相同的
    gs_out_TextureLayerIdx = f_TextureLayerIdx[0];
    gs_out_NoFilter = f_NoFilter[0];
    gs_out_UVScale = f_UVScale[0];
    gs_out_GroupSize = f_GroupSize[0];
    gs_out_DefColor = f_DefColor[0];
    gs_out_Radius = f_Radius[0];
    gs_out_RadiusEffectParam = f_RadiusEffectParam[0];
    gs_out_RadiusEffect = f_RadiusEffect[0];
    gs_out_TexScaleStratergy = f_TexScaleStratergy[0];
    gs_out_TexAlignStratergy = f_TexAlignStratergy[0];
}
