#version 430 core
#define NUM_THREADS 8

layout(location = 0) uniform vec4 _Size;          // Size of the grid
layout(location = 1) uniform vec4 _Up;            // Up vector
layout(location = 2) uniform float _AmbientTemperature;
layout(location = 3) uniform float _DeltaTime;
layout(location = 4) uniform float _Buoyancy;
layout(location = 5) uniform float _Weight;

layout(rw) buffer WriteBuffer
{
    vec3 _Write [];  // RWStructuredBuffer equivalent
};

layout(binding = 0) buffer VelocityBuffer
{
    vec3 _Velocity []; // StructuredBuffer equivalent
};

layout(binding = 1) buffer DensityBuffer
{
    float _Density []; // StructuredBuffer equivalent
};

layout(binding = 2) buffer TemperatureBuffer
{
    float _Temperature []; // StructuredBuffer equivalent
};

void main()
{
    // Get the global invocation ID
    ivec3 id = gl_GlobalInvocationID.xyz;

    // Calculate the index
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    float T = _Temperature[idx];
    float D = _Density[idx];
    vec3 V = _Velocity[idx];

    if (T > _AmbientTemperature)
    {
        V += (_DeltaTime * (T - _AmbientTemperature) * _Buoyancy - D * _Weight) * _Up.xyz;
    }

    _Write[idx] = V;
}
