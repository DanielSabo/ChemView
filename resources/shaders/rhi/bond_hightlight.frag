#version 450

//layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 2) uniform qt3d_custom_uniforms {
  vec4 color;
};

void main()
{
    fragColor = color;
}
