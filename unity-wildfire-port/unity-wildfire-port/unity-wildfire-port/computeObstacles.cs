#version 430 core

#define NUM_THREADS 8

layout(location = 0) uniform vec4 _Size; // Size of the grid

layout(rw) buffer WriteBuffer
{
    float _Write []; // RWStructuredBuffer equivalent
};

void CSMain()
{
    // Get the global invocation ID
    ivec3 id = ivec3(gl_GlobalInvocationID);

    // Calculate the index for the current cell
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    float obstacle = 0.0;

    // Check for boundaries and set obstacle value
    if (id.x - 1 < 0) obstacle = 1.0;
    if (id.x + 1 > int(_Size.x) - 1) obstacle = 1.0;

    if (id.y - 1 < 0) obstacle = 1.0;
    if (id.y + 1 > int(_Size.y) - 1) obstacle = 1.0;

    if (id.z - 1 < 0) obstacle = 1.0;
    if (id.z + 1 > int(_Size.z) - 1) obstacle = 1.0;

    // Write the obstacle value to the output buffer
    _Write[idx] = obstacle;
}
