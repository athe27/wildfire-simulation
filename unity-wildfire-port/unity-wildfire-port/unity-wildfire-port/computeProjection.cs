#version 430 // Ensure you're using a suitable version for compute shaders

#define NUM_THREADS 8

layout(location = 0) uniform vec4 _Size; // Size of the grid

layout(rw) buffer WriteBuffer
{
    vec3 _Write []; // RWStructuredBuffer equivalent
};

layout(constant) buffer PressureBuffer
{
    float _Pressure []; // StructuredBuffer for Pressure
};

layout(constant) buffer ObstaclesBuffer
{
    float _Obstacles []; // StructuredBuffer for Obstacles
};

layout(constant) buffer VelocityBuffer
{
    vec3 _Velocity []; // StructuredBuffer for Velocity
};

void CSMain()
{
    // Get the global invocation ID
    ivec3 id = ivec3(gl_GlobalInvocationID);

    // Calculate the index for the current cell
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    // Check for obstacles
    if (_Obstacles[idx] > 0.1)
    {
        _Write[idx] = vec3(0.0, 0.0, 0.0);
        return;
    }

    // Calculate neighboring indices
    int idxL = max(0, id.x - 1) + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);
    int idxR = min(int(_Size.x) - 1, id.x + 1) + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    int idxB = id.x + max(0, id.y - 1) * int(_Size.x) + id.z * int(_Size.x * _Size.y);
    int idxT = id.x + min(int(_Size.y) - 1, id.y + 1) * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    int idxD = id.x + id.y * int(_Size.x) + max(0, id.z - 1) * int(_Size.x * _Size.y);
    int idxU = id.x + id.y * int(_Size.x) + min(int(_Size.z) - 1, id.z + 1) * int(_Size.x * _Size.y);

    float L = _Pressure[idxL];
    float R = _Pressure[idxR];
    float B = _Pressure[idxB];
    float T = _Pressure[idxT];
    float D = _Pressure[idxD];
    float U = _Pressure[idxU];

    float C = _Pressure[idx];

    vec3 mask = vec3(1.0, 1.0, 1.0);

    if (_Obstacles[idxL] > 0.1) { L = C; mask.x = 0.0; }
    if (_Obstacles[idxR] > 0.1) { R = C; mask.x = 0.0; }

    if (_Obstacles[idxB] > 0.1) { B = C; mask.y = 0.0; }
    if (_Obstacles[idxT] > 0.1) { T = C; mask.y = 0.0; }

    if (_Obstacles[idxD] > 0.1) { D = C; mask.z = 0.0; }
    if (_Obstacles[idxU] > 0.1) { U = C; mask.z = 0.0; }

    vec3 v = _Velocity[idx] - vec3(R - L, T - B, U - D) * 0.5;

    _Write[idx] = v * mask;
}
