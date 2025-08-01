#version 410 core

// uniform int formwidth;
// uniform int formheight;
// uniform uint samples;

uniform mat4 projection;
uniform mat4 view;
uniform int channel;

layout(location = 0) in float sample_value;

void main() {
    if (channel == 0) {
        vec4 pos = projection * view * vec4(float(gl_VertexID), sample_value / 2.0 + 0.5, 0.0, 1.0);
        gl_Position = vec4(pos.xyzw);
    } else {
        vec4 pos = projection * view * vec4(float(gl_VertexID), sample_value / 2.0 - 0.5, 0.0, 1.0);
        gl_Position = vec4(pos.xyzw);
    }
}
