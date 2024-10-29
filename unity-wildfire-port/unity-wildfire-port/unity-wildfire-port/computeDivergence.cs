#version 430 core

#define NUM_THREADS 8

layout(location = 0) uniform vec4 _Size; // Size of the grid

layout(rw) buffer WriteBuffer
{
    vec3 _Write []; // RWStructuredBuffer equivalent
};

layout(binding = 0) buffer VelocityBuffer
{
    vec3 _Velocity []; // StructuredBuffer equivalent
};

layout(binding = 1) buffer ObstaclesBuffer
{
    float _Obstacles []; // StructuredBuffer equivalent
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

    // Retrieve velocities of neighboring cells
    vec3 L = _Velocity[idxL];
    vec3 R = _Velocity[idxR];

    vec3 B = _Velocity[idxB];
    vec3 T = _Velocity[idxT];

    vec3 D = _Velocity[idxD];
    vec3 U = _Velocity[idxU];

    vec3 obstacleVelocity = vec3(0.0, 0.0, 0.0);

    // Check for obstacles and set velocity accordingly
    if (_Obstacles[idxL] > 0.1) L = obstacleVelocity;
    if (_Obstacles[idxR] > 0.1) R = obstacleVelocity;

    if (_Obstacles[idxB] > 0.1) B = obstacleVelocity;
    if (_Obstacles[idxT] > 0.1) T = obstacleVelocity;

    if (_Obstacles[idxD] > 0.1) D = obstacleVelocity;
    if (_Obstacles[idxU] > 0.1) U = obstacleVelocity;

    // Calculate divergence
    float divergence = 0.5 * ((R.x - L.x) + (T.y - B.y) + (U.z - D.z));

    // Calculate the index for writing results
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    _Write[idx] = vec3(divergence, 0.0, 0.0);
}
