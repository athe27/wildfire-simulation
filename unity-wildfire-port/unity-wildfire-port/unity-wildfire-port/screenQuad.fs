#version 430 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler3D tex;
uniform float t;

void main()
{             
    float speed = 0.2;
    vec3 coords = vec3(TexCoords.xy, t * speed);
    vec3 texCol = texture(tex, coords).rgb;      
    FragColor = vec4(texCol, 1.0);
}