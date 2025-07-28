#version 410 core

// uniform int formwidth;
// uniform int formheight;
// uniform uint samples;

uniform mat4 projection;
uniform mat4 view;

layout(location = 0) in float sample_value;

void main() {
    // 1. 获取原始采样的正负号。
    // sign() 函数会返回 -1.0, 0.0, 或 1.0。
    // 我们先把它存起来，因为 abs() 会丢失这个信息。
    float original_sign = sign(sample_value);

    // 2. 获取原始振幅的绝对值（[0.0, 1.0]范围内的正数）。
    float amplitude = abs(sample_value);

    // 3. 定义一个指数来调整响度曲线。
    // 0.5 是一个很好的起点 (相当于开根号)。
    // 值越小，对安静部分的提升越大。可以尝试 0.3 到 0.7 之间的值。
    const float loudness_exponent = 0.5;

    // 4. 使用幂函数，根据振幅计算出新的“视觉高度”。
    // 安静的部分(amplitude小)会被不成比例地放大，响亮的部分(amplitude大)则变化不大。
    float visual_height = pow(amplitude, loudness_exponent);

    // 5. 【关键步骤】将计算出的新高度与原始的正负号重新组合。
    // 这样，波形的形状和正负关系就完全保留了，只是高度被非线性地拉伸了。
    float y_pos = visual_height * original_sign;

    gl_Position = projection * view * vec4(float(gl_VertexID), y_pos, 0.0, 1.0);
}
