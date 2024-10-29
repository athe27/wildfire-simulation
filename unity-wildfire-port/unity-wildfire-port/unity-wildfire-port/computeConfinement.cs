#version 430 core

#define NUM_THREADS 8

layout(location = 0) uniform float _DeltaTime;
layout(location = 1) uniform float _Epsilon;
layout(location = 2) uniform vec4 _Size; // Size of the grid

layout(rw) buffer WriteBuffer
{
    vec3 _Write []; // RWStructuredBuffer equivalent
};

layout(binding = 0) buffer VorticityBuffer
{
    vec3 _Vorticity []; // StructuredBuffer equivalent
};

layout(binding = 1) buffer ReadBuffer
{
    vec3 _Read []; // StructuredBuffer equivalent
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

    // Calculate the lengths of vorticity vectors
    float omegaL = length(_Vorticity[idxL]);
    float omegaR = length(_Vorticity[idxR]);

    float omegaB = length(_Vorticity[idxB]);
    float omegaT = length(_Vorticity[idxT]);

    float omegaD = length(_Vorticity[idxD]);
    float omegaU = length(_Vorticity[idxU]);

    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    vec3 omega = _Vorticity[idx];

    vec3 eta = 0.5 * vec3(omegaR - omegaL, omegaT - omegaB, omegaU - omegaD);
    eta = normalize(eta + vec3(0.001, 0.001, 0.001));

    vec3 force = _DeltaTime * _Epsilon * vec3(
        eta.y * omega.z - eta.z * omega.y,
        eta.z * omega.x - eta.x * omega.z,
        eta.x * omega.y - eta.y * omega.x
    );

    _Write[idx] = _Read[idx] + force;
}
