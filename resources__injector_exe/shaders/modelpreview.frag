#version 440

layout(location = 0) in vec2 v_texcoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D skinTexture;

void main()
{
    vec4 color = texture(skinTexture, v_texcoord);
    if (color.a < 0.10) {
        discard;
    }
    fragColor = vec4(color.rgb, 1.0);
}
