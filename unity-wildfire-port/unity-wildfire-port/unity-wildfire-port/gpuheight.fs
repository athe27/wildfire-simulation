#version 330 core

in vec2 TexCoord;      // Interpolated texture coordinates from the vertex shader
out vec4 FragColor;     // Output color of the fragment

uniform sampler2D landscapeTexture;  // The PNG texture

void main()
{
    FragColor = texture(landscapeTexture, TexCoord);  // Sample the texture at given coordinates
}