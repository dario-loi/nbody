#type vertex

#version 420 core

layout(location = 0) in vec2 aPos;

uniform float uScale;

void main()
{
    float x = aPos.x / uScale;
    float y = aPos.y / uScale;

    gl_Position = vec4(x, y, 0.0, 1.0);
}

#type fragment

#version 420 core

layout(location = 0) out vec4 FragColor;


void main()
{
    const vec4 c = vec4(1.0f, 1.0f, 0.0f, 1.0f);

    FragColor = c;
}