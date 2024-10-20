#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler3D tex;
uniform float t;

void main()
{             
    float cycleTime = 2;
    vec3 coords = vec3(TexCoords, t * 1 / cycleTime);
    vec3 texCol = texture(tex, coords).rgb;      
    FragColor = vec4(texCol, 1.0);
}