#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler3D tex;
uniform float iTime;
uniform vec3 iResolution; // x: width, y: height, z: aspect ratio

void main()
{             
    float cycleTime = 2;
    vec3 coords = vec3(TexCoords, iTime * 1 / cycleTime);
    vec3 texCol = texture(tex, coords).rgb;      
    FragColor = vec4(texCol, 1.0);
}