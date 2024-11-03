#type vertex
#version 420 core
#line 3

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;

layout(location = 0) out vec2 Uv;

void main()
{
    Uv = aUv;
    gl_Position = vec4(aPos, 1.0);
}

#type fragment
#version 420 core
#line 18

layout(location = 0) in vec2 Uv;

layout(binding = 0) uniform sampler2D u_tex;

layout(location = 0) out vec4 outColor;

void main()
{
    const vec4 scene_color = texture(u_tex, Uv);
    outColor = scene_color;
}
