#version 410 core

// Texture coordinates from the Tessellation Evaluation Shader
in vec2 TexCoord;

// Output color of the fragment
out vec4 FragColor;

// The PNG texture we are reading from.
uniform sampler2D landscapeTexture;

void main()
{
    FragColor = texture(landscapeTexture, TexCoord);  // Sample the texture at given coordinates
}