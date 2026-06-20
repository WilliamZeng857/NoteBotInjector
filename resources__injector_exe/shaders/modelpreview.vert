#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in float depthbias;

layout(location = 0) out vec2 v_texcoord;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{
    gl_Position = mvp * vec4(position, 1.0);
    gl_Position.z -= depthbias;
    v_texcoord = texcoord;
}
