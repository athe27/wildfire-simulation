#version 430 core

#define NUM_THREADS 8

layout(std140) uniform;

layout(binding = 0) buffer VelocityBuffer
{
    vec3 _Velocity [];
};

layout(binding = 1) buffer ObstaclesBuffer
{
    float _Obstacles [];
};

layout(binding = 2) buffer Write1fBuffer
{
    writeonly float _Write1f [];
};

layout(binding = 3) buffer Read1fBuffer
{
    float _Read1f [];
};

layout(binding = 4) buffer Write3fBuffer
{
    writeonly vec3 _Write3f [];
};

layout(binding = 5) buffer Read3fBuffer
{
    vec3 _Read3f [];
};

layout(binding = 6) buffer Phi_n_1_hatBuffer
{
    float _Phi_n_1_hat [];
};

layout(binding = 7) buffer Phi_n_hatBuffer
{
    float _Phi_n_hat [];
};

uniform vec4 _Size;
uniform float _DeltaTime;
uniform float _Dissipate;
uniform float _Decay;
uniform float _Forward;

vec3 GetAdvectedPosTexCoords(vec3 pos, int idx)
{
    pos -= _DeltaTime * _Forward * _Velocity[idx];
    return pos;
}

float SampleBilinear(const in float[] buffer, vec3 uv, vec3 size)
{
    int x = int(uv.x);
    int y = int(uv.y);
    int z = int(uv.z);

    int X = int(size.x);
    int XY = int(size.x * size.y);

    float fx = uv.x - float(x);
    float fy = uv.y - float(y);
    float fz = uv.z - float(z);

    int xp1 = min(X - 1, x + 1);
    int yp1 = min(int(size.y) - 1, y + 1);
    int zp1 = min(int(size.z) - 1, z + 1);

    float x0 = buffer[x + y * X + z * XY] * (1.0 - fx) + buffer[xp1 + y * X + z * XY] * fx;
    float x1 = buffer[x + y * X + zp1 * XY] * (1.0 - fx) + buffer[xp1 + y * X + zp1 * XY] * fx;

    float x2 = buffer[x + yp1 * X + z * XY] * (1.0 - fx) + buffer[xp1 + yp1 * X + z * XY] * fx;
    float x3 = buffer[x + yp1 * X + zp1 * XY] * (1.0 - fx) + buffer[xp1 + yp1 * X + zp1 * XY] * fx;

    float z0 = x0 * (1.0 - fz) + x1 * fz;
    float z1 = x2 * (1.0 - fz) + x3 * fz;

    return z0 * (1.0 - fy) + z1 * fy;
}

vec3 SampleBilinear(const in vec3[] buffer, vec3 uv, vec3 size)
{
    int x = int(uv.x);
    int y = int(uv.y);
    int z = int(uv.z);

    int X = int(size.x);
    int XY = int(size.x * size.y);

    float fx = uv.x - float(x);
    float fy = uv.y - float(y);
    float fz = uv.z - float(z);

    int xp1 = min(X - 1, x + 1);
    int yp1 = min(int(size.y) - 1, y + 1);
    int zp1 = min(int(size.z) - 1, z + 1);

    vec3 x0 = buffer[x + y * X + z * XY] * (1.0 - fx) + buffer[xp1 + y * X + z * XY] * fx;
    vec3 x1 = buffer[x + y * X + zp1 * XY] * (1.0 - fx) + buffer[xp1 + y * X + zp1 * XY] * fx;

    vec3 x2 = buffer[x + yp1 * X + z * XY] * (1.0 - fx) + buffer[xp1 + yp1 * X + z * XY] * fx;
    vec3 x3 = buffer[x + yp1 * X + zp1 * XY] * (1.0 - fx) + buffer[xp1 + yp1 * X + zp1 * XY] * fx;

    vec3 z0 = x0 * (1.0 - fz) + x1 * fz;
    vec3 z1 = x2 * (1.0 - fz) + x3 * fz;

    return z0 * (1.0 - fy) + z1 * fy;
}

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = NUM_THREADS) in;

void AdvectVelocity()
{
    uint3 id = gl_GlobalInvocationID.xyz;
    int idx = int(id.x) + int(id.y) * int(_Size.x) + int(id.z) * int(_Size.x * _Size.y);

    if (_Obstacles[idx] > 0.1)
    {
        _Write3f[idx] = vec3(0.0);
        return;
    }

    vec3 uv = GetAdvectedPosTexCoords(vec3(id), idx);
    _Write3f[idx] = SampleBilinear(_Read3f, uv, _Size.xyz) * _Dissipate;
}

void Advect()
{
    uint3 id = gl_GlobalInvocationID.xyz;
    int idx = int(id.x) + int(id.y) * int(_Size.x) + int(id.z) * int(_Size.x * _Size.y);

    if (_Obstacles[idx] > 0.1)
    {
        _Write1f[idx] = 0.0;
        return;
    }

    vec3 uv = GetAdvectedPosTexCoords(vec3(id), idx);
    _Write1f[idx] = max(0.0, SampleBilinear(_Read1f, uv, _Size.xyz) * _Dissipate - _Decay);
}



// Implement the other kernels (AdvectBFECC, AdvectMacCormack) similarly...

void AdvectBFECC()
{
    uint3 id = gl_GlobalInvocationID.xyz;
    int idx = int(id.x) + int(id.y) * int(_Size.x) + int(id.z) * int(_Size.x * _Size.y);

    if (_Obstacles[idx] > 0.1)
    {
        _Write1f[idx] = 0.0;
        reutrn;
    }

    vec3 uv = GetAdvectedPosTexCoords(vec3(id), idx);

    float r;
    vec4 halfVolumeDim = _Size / 2;

    vec3 diff = abs(halfVolumeDim.xyz - id);

    // Must use regular semi-Lagrangian advection instead of BFECC at the volume boundaries.
    if ((diff.x > (halfVolumeDim.x - 4)) || (diff.y > (halfVolumeDim.y - 4)) || (diff.z - (halfVolumeDim.z - 4))) {
        r = SampleBilinear(_Read1f, uv, _Size.xyz);
    } else {
        r = 1.5f * SampleBilinear(_Read1f, uv, _Size.xyz) - 0.5f * SampleBilinear(_Phi_n_hat, uv, _Size.xyz);
    }

    _Write1f[idx] = max(0, r * _Dissipate - _Decay);
}

void AdvectMacCormack()
{
    uint3 id = gl_GlobalInvocationID.xyz;
    int idx = int(id.x) + int(id.y) * int(_Size.x) + int(id.z) * int(_Size.x * _Size.y);

    if (_Obstacles[idx] > 0.1)
    {
        _Write1f[idx] = 0.0;
        reutrn;
    }

    vec3 uv = GetAdvectedPosTexCoords(vec3(id), idx);

    float r;
    vec4 halfVolumeDim = _Size / 2;
    vec3 diff = abs(halfVolumeDim.xyz - id);

    // Must use regular semi-Lagrangian advection instead of MacCormack at the volume boundaries
    if ((diff.x > (halfVolumeDim.x - 4)) || (diff.y > (halfVolumeDim.y - 4)) || (diff.z - (halfVolumeDim.z - 4)))
    {
        r = SampleBilinear(_Read1f, uv, _Size.xyz);
    }
    else
    {
        int idx0 = (id.x - 1) + (id.y - 1) * _Size.x + (id.z - 1) * _Size.x * _Size.y;
        int idx1 = (id.x - 1) + (id.y - 1) * _Size.x + (id.z + 1) * _Size.x * _Size.y;

        int idx2 = (id.x - 1) + (id.y + 1) * _Size.x + (id.z - 1) * _Size.x * _Size.y;
        int idx3 = (id.x - 1) + (id.y + 1) * _Size.x + (id.z + 1) * _Size.x * _Size.y;

        int idx4 = (id.x + 1) + (id.y - 1) * _Size.x + (id.z - 1) * _Size.x * _Size.y;
        int idx5 = (id.x + 1) + (id.y - 1) * _Size.x + (id.z + 1) * _Size.x * _Size.y;

        int idx6 = (id.x + 1) + (id.y + 1) * _Size.x + (id.z - 1) * _Size.x * _Size.y;
        int idx7 = (id.x + 1) + (id.y + 1) * _Size.x + (id.z + 1) * _Size.x * _Size.y;

        float nodes[8];
        nodes[0] = _Read1f[idx0];
        nodes[1] = _Read1f[idx1];

        nodes[2] = _Read1f[idx2];
        nodes[3] = _Read1f[idx3];

        nodes[4] = _Read1f[idx4];
        nodes[5] = _Read1f[idx5];

        nodes[6] = _Read1f[idx6];
        nodes[7] = _Read1f[idx7];

        float minPhi = min(min(min(min(min(min(min(nodes[0], nodes[1]), nodes[2]), nodes[3]), nodes[4]), nodes[5]), nodes[6]), nodes[7]));
        float maxPhi = max(max(max(max(max(max(max(nodes[0], nodes[1]), nodes[2]), nodes[3]), nodes[4]), nodes[5]), nodes[6]), nodes[7]));

        r = SampleBilinear(_Phi_n_1_hat, uv, _Size.xyz) + 0.5 * (_Read1f[idx] - _Phi_n_hat[idx]);

        r = clamp(r, minPhi, maxPhi);
    }

    _Write1f[idx] = max(0, r * _Dissipate - _Decay);
}

// You will need to dispatch these compute shaders with appropriate arguments in your OpenGL application.
