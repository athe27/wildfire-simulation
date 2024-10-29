#version 430 core

#define NUM_THREADS 8

layout(location = 0) uniform float _Radius;
layout(location = 1) uniform float _Amount;
layout(location = 2) uniform float _DeltaTime;
layout(location = 3) uniform float _Extinguishment;
layout(location = 4) uniform vec4 _Pos;    // Position vector
layout(location = 5) uniform vec4 _Size;   // Size vector

layout(rw) buffer WriteBuffer
{
    float _Write [];  // RWStructuredBuffer equivalent
};

layout(binding = 0) buffer ReadBuffer
{
    float _Read []; // StructuredBuffer equivalent
};

layout(binding = 1) buffer ReactionBuffer
{
    float _Reaction []; // StructuredBuffer equivalent
};

void GaussImpulse()
{
    // Get the global invocation ID
    ivec3 id = gl_GlobalInvocationID.xyz;

    // Calculate position relative to _Pos
    vec3 pos = vec3(id) / (_Size.xyz - 1.0) - _Pos.xyz;
    float mag = dot(pos, pos);
    float rad2 = _Radius * _Radius;

    float amount = exp(-mag / rad2) * _Amount * _DeltaTime;

    // Calculate the index
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    _Write[idx] = _Read[idx] + amount;
}

void ExtinguishmentImpluse()
{
    // Get the global invocation ID
    ivec3 id = gl_GlobalInvocationID.xyz;

    // Calculate the index
    int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

    float amount = 0.0;
    float reaction = _Reaction[idx];

    if (reaction > 0.0 && reaction < _Extinguishment)
    {
        amount = _Amount * reaction;
    }

    _Write[idx] = _Read[idx] + amount;
}
