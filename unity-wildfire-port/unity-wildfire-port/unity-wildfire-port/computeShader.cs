#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout(rgba32f, binding = 0) uniform image3D imgOutput;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

void main()
{
    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
    ivec3 voxelCoord = ivec3(gl_GlobalInvocationID);
    value = vec4(vec3(voxelCoord) / vec3(gl_NumWorkGroups), 1.0);
    imageStore(imgOutput, voxelCoord, value);
}