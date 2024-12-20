#type vertex

#version 420 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aCol;

uniform float uScale;

out vec4 vColor;

void main()
{
    const vec2 pos_trans = aPos / uScale;
    vColor = vec4(aCol, 0.99);
    gl_Position = vec4(pos_trans.x, pos_trans.y, 0.0, 1.0);
}

#type fragment

#version 420 core

layout(location = 0) out vec4 FragColor;

in vec4 vColor;

void main()
{
    vec4 c = vColor;
    c.rgb = pow(c.rgb, vec3(1.0 / 2.2));
    FragColor = c;
}