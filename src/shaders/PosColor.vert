#version 450

// out variable
layout(location = 0) out vec3 fragColor;

// vertex attributes
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

void main() {
    gl_Position = vec4(inPos, 1.0);
    fragColor = inColor;
}