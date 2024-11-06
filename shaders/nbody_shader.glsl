#type vertex

#version 420 core

layout(location = 0) in vec2 aPos;

uniform float uScale;

void main()
{
    const vec2 pos_trans = aPos / uScale;
    gl_Position = vec4(pos_trans.x, pos_trans.y, 0.0, 1.0);
}

#type fragment

#version 420 core

layout(location = 0) out vec4 FragColor;



void main()
{
    const vec4 c = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    FragColor = c;
}