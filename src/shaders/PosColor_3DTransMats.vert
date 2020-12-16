#version 450

// out variables
// NOTE: Out variables are linearly interpolated between vertices
layout(location = 0) out vec3 fragColor;

// vertex attributes
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

// uniform buffer objects
layout (binding = 0) uniform TransMats {
  mat4 model;
  mat4 view;
  mat4 proj;
} transMats;

void main() {
  // Note that each of the output values will be linearly
  // interpolated when they reach the fragment shader
  gl_Position = transMats.proj * transMats.view * transMats.model * vec4(inPos, 1.0);
  fragColor = inColor;
}