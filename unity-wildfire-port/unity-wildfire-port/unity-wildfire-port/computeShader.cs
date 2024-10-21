#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout(rgba32f, binding = 0) uniform image3D imgOutput;

layout(location = 0) uniform int BOX_N;
layout(location = 1) uniform float iTime;

// Constants
const float DENSITY_SMOOTHING = 0.2;
const float VISCOSITY = 0.15;
const float TIME_STEP = 0.1;
const float MAX_VELOCITY = 10.0;
const float K_CONST = 0.3;
const float VORTICITY_COEFF = 1.0;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

vec4 getDataNearest(vec3 voxelCoord)
{
    ivec3 floorVoxelCoord = ivec3(voxelCoord);
    return imageLoad(imgOutput, floorVoxelCoord);
}

vec3 getCurlNearest(vec3 voxelCoord)
{
    ivec3 floorVoxelCoord = ivec3(voxelCoord);
    return imageLoad(imgOutput, floorVoxelCoord + ivec3(0, 0, BOX_N + 1)).xyz;
}

vec4 getDataInterp(vec3 lmn)
{
    vec3 flmn = floor(lmn);

    vec4 d000 = getDataNearest(flmn);
    vec4 d001 = getDataNearest(flmn + vec3(0.0, 0.0, 1.0));
    vec4 d010 = getDataNearest(flmn + vec3(0.0, 1.0, 0.0));
    vec4 d011 = getDataNearest(flmn + vec3(0.0, 1.0, 1.0));
    vec4 d100 = getDataNearest(flmn + vec3(1.0, 0.0, 0.0));
    vec4 d101 = getDataNearest(flmn + vec3(1.0, 0.0, 1.0));
    vec4 d110 = getDataNearest(flmn + vec3(1.0, 1.0, 0.0));
    vec4 d111 = getDataNearest(flmn + vec3(1.0, 1.0, 1.0));

    vec3 t = lmn - flmn;
    vec4 dY0Z0 = mix(d000, d100, t.x);
    vec4 dY1Z0 = mix(d010, d110, t.x);
    vec4 dY0Z1 = mix(d001, d101, t.x);
    vec4 dY1Z1 = mix(d011, d111, t.x);
    vec4 dZ0 = mix(dY0Z0, dY1Z0, t.y);
    vec4 dZ1 = mix(dY0Z1, dY1Z1, t.y);
    return mix(dZ0, dZ1, t.z);
}

void addSource(
    in vec3 uvw,
    in vec3 uvwSource, in vec3 dirSource,
    inout float nextDensity, inout vec3 nextVelocity
)
{
    float t = 1.0 - smoothstep(0.0, 0.4, distance(uvw, uvwSource));
    nextDensity += t * 0.1;
    nextVelocity += t * MAX_VELOCITY * normalize(dirSource);
}

void doPage1(out vec4 fragColor, in vec3 lmn)
{
    vec4 data0 = getDataNearest(lmn);
    vec4 dataLA = getDataNearest(lmn + vec3(-1.0, 0.0, 0.0));
    vec4 dataLB = getDataNearest(lmn + vec3(1.0, 0.0, 0.0));
    vec4 dataMA = getDataNearest(lmn + vec3(0.0, -1.0, 0.0));
    vec4 dataMB = getDataNearest(lmn + vec3(0.0, 1.0, 0.0));
    vec4 dataNA = getDataNearest(lmn + vec3(0.0, 0.0, -1.0));
    vec4 dataNB = getDataNearest(lmn + vec3(0.0, 0.0, 1.0));

    vec3 curl0 = getCurlNearest(lmn);
    vec3 curlLA = getCurlNearest(lmn + vec3(-1.0, 0.0, 0.0));
    vec3 curlLB = getCurlNearest(lmn + vec3(1.0, 0.0, 0.0));
    vec3 curlMA = getCurlNearest(lmn + vec3(0.0, -1.0, 0.0));
    vec3 curlMB = getCurlNearest(lmn + vec3(0.0, 1.0, 0.0));
    vec3 curlNA = getCurlNearest(lmn + vec3(0.0, 0.0, -1.0));
    vec3 curlNB = getCurlNearest(lmn + vec3(0.0, 0.0, 1.0));

    // Various derivative approximations
    vec4 dxData = 0.5 * (dataLB - dataLA);
    vec4 dyData = 0.5 * (dataMB - dataMA);
    vec4 dzData = 0.5 * (dataNB - dataNA);

    vec3 gradDensity = vec3(dxData.w, dyData.w, dzData.w);
    float divVelocity = dxData.x + dyData.y + dzData.z;
    vec3 laplacian = dataLA.xyz + dataLB.xyz + dataMA.xyz + dataMB.xyz + dataNA.xyz + dataNB.xyz - 6.0 * data0.xyz;

    vec3 gradAbsCurl = vec3(
        length(curlLB) - length(curlLA),
        length(curlMB) - length(curlMA),
        length(curlNB) - length(curlNA)
    );
    vec3 fVorticityConfinement = vec3(0.0);
    if (length(gradAbsCurl) > 0.0)
    {
        fVorticityConfinement = VORTICITY_COEFF * cross(normalize(gradAbsCurl), curl0);
    }

    // Solve for density
    float nextDensity = mix(data0.w, (dataLA.w + dataLB.w + dataMA.w + dataMB.w + dataNA.w + dataNB.w) / 6.0, DENSITY_SMOOTHING);
    nextDensity -= TIME_STEP * (dot(gradDensity, data0.xyz) + divVelocity * data0.w);

    // Solve for velocity
    vec3 nextVelocity = getDataInterp(lmn - TIME_STEP * data0.xyz).xyz;
    vec3 fViscosity = VISCOSITY * laplacian;
    vec3 fCorrectivePressure = gradDensity * K_CONST;
    nextVelocity += TIME_STEP * (fViscosity + fVorticityConfinement) - fCorrectivePressure;

    // Add sources
    vec3 uvw = (2.0 * lmn - vec3(BOX_N)) / vec3(BOX_N);
    addSource(uvw, vec3(0.0, -0.5, 0.5), vec3(0.1 * sin(iTime), 1.0, 0.0), nextDensity, nextVelocity);
    addSource(uvw, vec3(0.0, 0.0, -0.5), vec3(cos(iTime), sin(iTime * 2.0), sin(iTime)), nextDensity, nextVelocity);

    // Clamp and set boundary values
    nextDensity = clamp(nextDensity, 0.5, 3.0);
    if (length(nextVelocity) > MAX_VELOCITY)
    {
        nextVelocity = normalize(nextVelocity) * MAX_VELOCITY;
    }
    nextVelocity *= step(0.5, lmn) * (1.0 - step(BOX_N - 1.5, lmn));

    // Done!
    fragColor = vec4(nextVelocity, nextDensity);
}

void doPage2(out vec4 fragColor, in vec3 lmn)
{
    vec4 data0 = getDataNearest(lmn);
    vec4 dataLA = getDataNearest(lmn + vec3(-1.0, 0.0, 0.0));
    vec4 dataLB = getDataNearest(lmn + vec3(1.0, 0.0, 0.0));
    vec4 dataMA = getDataNearest(lmn + vec3(0.0, -1.0, 0.0));
    vec4 dataMB = getDataNearest(lmn + vec3(0.0, 1.0, 0.0));
    vec4 dataNA = getDataNearest(lmn + vec3(0.0, 0.0, -1.0));
    vec4 dataNB = getDataNearest(lmn + vec3(0.0, 0.0, 1.0));

    // Various derivative approximations
    vec4 dxData = 0.5 * (dataLB - dataLA);
    vec4 dyData = 0.5 * (dataMB - dataMA);
    vec4 dzData = 0.5 * (dataNB - dataNA);

    vec3 curlVelocity = vec3(dyData.z - dzData.y, dzData.x - dxData.z, dxData.y - dyData.x);
    fragColor = vec4(curlVelocity, 1.0);
}

void main()
{
    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
    ivec3 voxelCoord = ivec3(gl_GlobalInvocationID);
    if (voxelCoord.z > BOX_N)
    {
        doPage2(value, voxelCoord - vec3(0.0, 0.0, BOX_N));
    } else if (voxelCoord.z < BOX_N)
    {
        doPage1(value, voxelCoord);
    }
    imageStore(imgOutput, voxelCoord, value);
}