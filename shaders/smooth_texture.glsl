#type vertex

#version 420 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;

layout(location = 0) out vec2 Uv;

void main()
{

    Uv = aUv;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

#type fragment
#version 420 core

#line 20

layout(location = 0) in vec2 Uv;

layout(binding = 0) uniform sampler2D u_last_frame;
uniform float u_time_delta;

layout(location = 0) out vec4 FragColor;

const float tau = 1.5;

void main()
{
    vec4 last_color = texture(u_last_frame, Uv);
    vec3 c = last_color.rgb - vec3(u_time_delta / tau);

    FragColor = vec4(max(c, vec3(0.0)), last_color.a);
}
