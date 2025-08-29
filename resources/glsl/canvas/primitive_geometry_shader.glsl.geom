#version 410 core

// Uniform 投影矩阵
uniform mat4 projection;

layout(points) in;
layout(triangle_strip, max_vertices = 36) out;

// --- 从顶点着色器接收 ---
// --- 输入：从顶点着色器接收的接口块 ---
// GS的输入总是数组
in VS_OUT {
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
} gs_in[]; // 实例名 gs_in，并且它是一个数组

// --- C++端 enum 值的常量定义 ---
const uint PRIMITIVE_QUAD = 1u;
const uint PRIMITIVE_OVAL = 2u;

const float PI = 3.1415926535;

// --- 传递给片元着色器 ---
// --- 输出：打包到接口块中，传递给片元着色器 ---
out GS_OUT {
    // 需要插值的变量
    vec2 v_TexCoord;
    vec2 v_WorldPos;

    // 需要转发的 flat 变量
    flat vec2 f_QuadSize;
    flat int f_TextureLayerIdx;
    flat uint f_NoFilter;
    flat vec2 f_UVScale;
    flat vec2 f_GroupSize;
    flat vec4 f_DefColor;
    flat vec2 f_Radius;
    flat float f_RadiusEffectParam;
    flat uint f_RadiusEffect;
    flat uint f_TexScaleStratergy;
    flat uint f_TexAlignStratergy;
} gs_out; // 实例名 gs_out

void generateQuad() {
    vec2 center = gl_in[0].gl_Position.xy;
    vec2 size = gs_in[0].size;
    float rotation = gs_in[0].rotation; // 获取旋转角度
    vec2 half_size = size / 2.0;

    vec2 positions_offset[4] = vec2[](
            vec2(-half_size.x, -half_size.y), // 左下
            vec2(half_size.x, -half_size.y), // 右下
            vec2(-half_size.x, half_size.y), // 左上
            vec2(half_size.x, half_size.y) // 右上
        );

    vec2 uvs[4] = vec2[](
            vec2(0.0, 0.0), vec2(1.0, 0.0),
            vec2(0.0, 1.0), vec2(1.0, 1.0)
        );

    // *** 核心修正: 创建旋转矩阵 ***
    float c = cos(rotation);
    float s = sin(rotation);
    mat2 rot_matrix = mat2(c, s, -s, c); // 注意Y轴方向，可能需要 c,s,-s,c 或 c,-s,s,c

    // 顺序: 左上 -> 左下 -> 右上 -> 右下
    int order[4] = int[](2, 0, 3, 1);
    for (int i = 0; i < 4; ++i) {
        int idx = order[i];

        // *** 核心修正: 先旋转偏移量，再加到中心点上 ***
        vec2 rotated_offset = rot_matrix * positions_offset[idx];
        gs_out.v_WorldPos = center + rotated_offset;

        gs_out.v_TexCoord = uvs[idx];
        gl_Position = projection * vec4(gs_out.v_WorldPos, 0.0, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

void generateOval() {
    vec2 center = gl_in[0].gl_Position.xy;
    vec2 size = gs_in[0].size;
    float rotation = gs_in[0].rotation; // 获取旋转角度
    vec2 radii = size / 2.0;

    uint segments = 16;

    // *** 核心修正: 创建旋转矩阵 ***
    float c = cos(rotation);
    float s = sin(rotation);
    mat2 rot_matrix = mat2(c, s, -s, c);

    for (uint i = 0; i <= segments; ++i) {
        float angle = 2.0 * PI * float(i) / float(segments);
        vec2 dir = vec2(cos(angle), sin(angle));

        // *** 核心修正: 对本地坐标应用旋转和平移 ***

        // 1. 在本地坐标计算椭圆上的点 (相对于原点)
        vec2 local_point_on_ellipse = dir * radii;
        // 2. 旋转这个本地点
        vec2 rotated_point = rot_matrix * local_point_on_ellipse;
        // 3. 平移到世界坐标中心
        vec2 world_point_on_ellipse = center + rotated_point;

        // 发射圆周点
        gs_out.v_WorldPos = world_point_on_ellipse;
        gs_out.v_TexCoord = dir * 0.5 + 0.5;
        gl_Position = projection * vec4(gs_out.v_WorldPos, 0.0, 1.0);
        EmitVertex();

        // 发射中心点 (中心点不受旋转影响，但为了保持strip结构，仍然发射)
        gs_out.v_WorldPos = center;
        gs_out.v_TexCoord = vec2(0.5, 0.5);
        gl_Position = projection * vec4(gs_out.v_WorldPos, 0.0, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

void main() {
    // --- 步骤 1: 转发所有 flat 变量 ---
    // 从输入块 gs_in 复制到输出块 gs_out
    gs_out.f_QuadSize = gs_in[0].size;
    gs_out.f_TextureLayerIdx = gs_in[0].textureLayerIdx;
    gs_out.f_NoFilter = gs_in[0].noFilter;
    gs_out.f_UVScale = gs_in[0].uvScale;
    gs_out.f_GroupSize = gs_in[0].groupSize;
    gs_out.f_DefColor = gs_in[0].color;
    gs_out.f_Radius = gs_in[0].radius;
    gs_out.f_RadiusEffectParam = gs_in[0].radiusEffectParam;
    gs_out.f_RadiusEffect = gs_in[0].radiusEffect;
    gs_out.f_TexScaleStratergy = gs_in[0].texScaleStratergy;
    gs_out.f_TexAlignStratergy = gs_in[0].texAlignStratergy;

    // 着色图元
    switch (gs_in[0].primitiveType) {
        case PRIMITIVE_QUAD:
        {
            generateQuad();
            break;
        }
        case PRIMITIVE_OVAL:
        {
            generateOval();
            break;
        }
    }
}
