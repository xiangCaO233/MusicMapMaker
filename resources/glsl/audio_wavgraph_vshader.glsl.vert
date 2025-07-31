#version 410 core

// uniform int formwidth;
// uniform int formheight;
// uniform uint samples;

uniform mat4 projection;
uniform mat4 view;

layout(location = 0) in float sample_value;

void main() {
    gl_Position = projection * view * vec4(float(gl_VertexID), sample_value, 0.0, 1.0);
}
