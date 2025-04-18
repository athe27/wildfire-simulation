// vertex shader
#version 410 core

// vertex position
layout (location = 0) in vec3 aPos;
// texture coordinate
layout (location = 1) in vec2 aTex;

out vec2 TexCoord;

void main()
{
    // convert XYZ vertex to XYZW homogeneous coordinate
    gl_Position = vec4(aPos, 1.0);
    // pass the texture coordinate through which will be used in the Tessellation Control Shader and then the Tessellation Evaluation Shader
    TexCoord = aTex;
}