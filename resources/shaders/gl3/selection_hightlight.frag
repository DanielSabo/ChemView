#version 150 core

uniform vec4 color;
in vec2 texCoord;
out vec4 fragColor;

// Circle drawing code from: https://rubendv.be/posts/fwidth/

void main()
{
    //fragColor = vec4(0.961, 0.737, 0.259, 1.0);
    //vec4 hard_color = vec4(0.961, 0.737, 0.259, 1.0);
    float dist = distance(texCoord, vec2(0.50, 0.50));
    float delta = fwidth(dist);
    float alpha = 1.0 - smoothstep(0.5-delta, 0.5, dist);
    fragColor = vec4(color.rgb, color.a * alpha);
}
