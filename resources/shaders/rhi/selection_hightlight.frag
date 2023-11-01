#version 450

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 2) uniform qt3d_custom_uniforms {
  vec4 color;
};

// Circle drawing code from: https://rubendv.be/posts/fwidth/

void main()
{
    //fragColor = vec4(0.961, 0.737, 0.259, 1.0);
    //vec4 hard_color = vec4(0.961, 0.737, 0.259, 1.0);
    float dist = distance(texCoord, vec2(0.50, 0.50));
    float delta = fwidth(dist);
    float alpha = 1.0 - smoothstep(0.5-delta, 0.5, dist);
//    fragColor = vec4(color.rgb, color.a * alpha);
    fragColor = vec4(color.rgb, color.a * alpha);
}
