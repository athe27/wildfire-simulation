#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec3 TexCoords;

void main()
{
    TexCoords = vec3(aTexCoords, 0.0);
    gl_Position = vec4(aPos, 1.0);
}