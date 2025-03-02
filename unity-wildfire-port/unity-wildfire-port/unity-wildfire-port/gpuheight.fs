#version 330 core

out vec4 FragColor;

in float Height;

#define HEIGHT_SHIFT 48
#define HEIGHT_SCALAR (1/256.0f)

void main()
{
    // shift and scale the height into a grayscale value
    float h = (Height + HEIGHT_SHIFT) * HEIGHT_SCALAR;
    FragColor = vec4(h, h, h, 1.0);
}