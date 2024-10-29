#version 430 core

#define NUM_THREADS 8

layout(location = 0) uniform vec4 _Size; // Size of the grid

layout(rw) buffer WriteBuffer
{
    float _Write []; // RWStructuredBuffer equivalent
};

layout(binding = 0) buffer PressureBuffer
{
    float _Pressure []; // StructuredBuffer equivalent
};

layout(binding = 1) buffer ObstaclesBuffer
{
    float _Obstacles []; // StructuredBuffer equivalent
};

layout(binding = 2) buffer DivergenceBuffer
{
    vec3 _Divergence []; // StructuredBuffer equivalent
};

void CSMain()
{
    // Get the global invocation ID
    ivec3 id = ivec3(gl_GlobalInvocationID);

    // Calculate neighbor indices
    int idxL = max(0, id.x - 1) + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);
    int idxR = min(int(_Size.x) - 1, id.x + 1) + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    int idxB = id.x + max(0, id.y - 1) * int(_Size.x) + id.z * int(_Size.x * _Size.y);
    int idxT = id.x + min(int(_Size.y) - 1, id.y + 1) * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    int idxD = id.x + id.y * int(_Size.x) + max(0, id.z - 1) * int(_Size.x * _Size.y);
    int idxU = id.x + id.y * int(_Size.x) + min(int(_Size.z) - 1, id.z + 1) * int(_Size.x * _Size.y);

    // Retrieve pressure values of neighboring cells
    float L = _Pressure[idxL];
    float R = _Pressure[idxR];

    float B = _Pressure[idxB];
    float T = _Pressure[idxT];

    float D = _Pressure[idxD];
    float U = _Pressure[idxU];

    // Calculate the index for the current cell
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    float C = _Pressure[idx];

    // Retrieve divergence for the current cell
    float divergence = _Divergence[idx].r;

    // Check for obstacles and adjust pressure values accordingly
    if (_Obstacles[idxL] > 0.1) L = C;
    if (_Obstacles[idxR] > 0.1) R = C;

    if (_Obstacles[idxB] > 0.1) B = C;
    if (_Obstacles[idxT] > 0.1) T = C;

    if (_Obstacles[idxD] > 0.1) D = C;
    if (_Obstacles[idxU] > 0.1) U = C;

    // Write the new pressure value to the output buffer
    _Write[idx] = (L + R + B + T + U + D - divergence) / 6.0;
}
