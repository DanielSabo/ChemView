#version 450

layout(location = 0) in vec3 vertexPosition;

layout(std140, binding = 1) uniform qt3d_command_uniforms {
  mat4 modelMatrix;
  mat4 inverseModelMatrix;
  mat4 modelViewMatrix;
  mat3 modelNormalMatrix;
  mat4 inverseModelViewMatrix;
  mat4 mvp;
  mat4 inverseModelViewProjectionMatrix;
};

void main()
{
    gl_Position = mvp * vec4( vertexPosition, 1.0 );
}
