#version 430 core

layout(local_size_x = 128, local_size_y = 128, local_size_z = 128) in;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout(rgba32f, binding = 0) uniform image3D imgOutput;

layout(location = 0) uniform float deltaTime;
layout(location = 1) uniform float dissipate;
layout(location = 2) uniform float decay;
layout(location = 3) uniform float forward;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

vec3 GetAdvectedPosTexCoords(vec3 pos, int idx)
{
    pos = pos - deltaTime * forward * velocity[idx];

    return pos;
}

void main()
{
    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
    int idx = int(gl_LocalInvocationIndex); // int idx = id.x + id.y*_Size.x + id.z*_Size.x*_Size.y;
	float speed = 100;
    // the width of the texture
    float width = 1000;

    value.x = mod(float(texelCoord.x) + t * speed, width) / (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
    value.y = float(texelCoord.y) / (gl_NumWorkGroups.y * gl_WorkGroupSize.y);
    imageStore(imgOutput, texelCoord, value);
}