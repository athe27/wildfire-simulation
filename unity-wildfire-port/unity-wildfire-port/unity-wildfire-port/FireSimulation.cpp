//#include <glad/glad.h>
//#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

#include <vector>
#include "FireSimulation.h"

// Define the 3D texture sizes
const int TEX_WIDTH = 128;
const int TEX_HEIGHT = 128;
const int TEX_DEPTH = 128;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

void FireSimulation::RunFireSimulation(float deltaTime, GLuint textures[3])
{
    dt = deltaTime;
    double newUpdateTime = glfwGetTime();

    // Initialize 3D textures
    //InitializeTexture(velocityDensityTexture);
    //InitializeTexture(pressureTempPhiReactionTexture);
    //InitializeTexture(curlObstaclesTexture);

    velocityDensityTexture = textures[0];
    pressureTempPhiReactionTexture = textures[1];
    curlObstaclesTexture = textures[2];

    std::cout << "Simulation Time is " << (newUpdateTime - lastUpdateTime) << std::endl;
    lastUpdateTime = newUpdateTime;

    //velocityDensityTexture = 0;
    //pressureTempPhiReactionTexture = 1;
    //curlObstaclesTexture = 2;

    // Create CPU-side buffers for simulation
    std::vector<glm::vec4> velocityDensityData(TEX_WIDTH * TEX_HEIGHT * TEX_DEPTH, glm::vec4(0.0f));
    std::vector<glm::vec4> pressureTempData(TEX_WIDTH * TEX_HEIGHT * TEX_DEPTH, glm::vec4(0.0f));
    std::vector<glm::vec4> curlObstaclesData(TEX_WIDTH * TEX_HEIGHT * TEX_DEPTH, glm::vec4(0.0f));

    RunAllComputeSimulation(velocityDensityData, pressureTempData, curlObstaclesData);

    // Upload data to GPU textures for use in a fragment shader
    UpdateTexture(velocityDensityTexture, velocityDensityData);
    UpdateTexture(pressureTempPhiReactionTexture, pressureTempData);
    UpdateTexture(curlObstaclesTexture, curlObstaclesData);
}

void FireSimulation::RunAllComputeSimulation(std::vector<glm::vec4>& velocityDensityData, std::vector<glm::vec4>& pressureTempData, std::vector<glm::vec4>& curlObstaclesData)
{
    std::cout << "Preparing " << (TEX_DEPTH * TEX_HEIGHT * TEX_WIDTH) << " CPU Invocatrions." << std::endl;

    for (int z = 0; z < TEX_DEPTH; ++z) {
        for (int y = 0; y < TEX_HEIGHT; ++y) {
            for (int x = 0; x < TEX_WIDTH; ++x) {
                int voxel_index = x + y * TEX_WIDTH + z * TEX_WIDTH * TEX_HEIGHT;

                glm::vec4 velocityDensity = velocityDensityData[voxel_index];
                glm::vec4 pressureTemp = pressureTempData[voxel_index];
                glm::vec3 voxel_coord = glm::vec3(x, y, z);

                RunThisComputeSimulation(velocityDensityData[voxel_index], pressureTempData[voxel_index], curlObstaclesData[voxel_index], voxel_coord);
            }
        }
     }
}

void FireSimulation::RunThisComputeSimulation(glm::vec4& velocityDensityData, glm::vec4& pressureTempData, glm::vec4& curlObstaclesData, glm::vec3& voxelCoord)
{
    iTime = glfwGetTime();
    if (iTime <= 1.0) {
        ComputeObstacles(voxelCoord, curlObstaclesData);
    }

    //AdvectDensityVelocity(voxelCoord, curlObstaclesData, velocityDensityData);
    //AdvectReactionTemperature(voxelCoord, pressureTempData, curlObstaclesData, velocityDensityData);
    ApplyBuoyancy(voxelCoord, velocityDensityData, pressureTempData);
    ApplyImpulse(voxelCoord, m_reactionAmount, 3, pressureTempData);
    ApplyImpulse(voxelCoord, m_temperatureAmount, 1, pressureTempData);
    ApplyExtinguishmentImpulse(voxelCoord, pressureTempData, velocityDensityData);
    //ComputeVorticityConfinement(voxelCoord, curlObstaclesData, velocityDensityData);
    //ComputeDivergence(voxelCoord, curlObstaclesData);
    for (int i = 0; i < m_iterations; i++)
    {
        ComputePressure(voxelCoord, curlObstaclesData, pressureTempData);
    }
    //ComputeProjection(voxelCoord, curlObstaclesData, pressureTempData, velocityDensityData);
}

// Function to initialize a 3D texture
void FireSimulation::InitializeTexture(GLuint& texture)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

// Function to update texture data from CPU computations
void FireSimulation::UpdateTexture(GLuint& texture, std::vector<glm::vec4>& data)
{
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH, GL_RGBA, GL_FLOAT, data.data());
}

void FireSimulation::ComputeObstacles(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData)
{
    float obstacle = 0;

    if (voxelCoord.x - 1 < 0)
    {
        obstacle = 1;
    }
    if (voxelCoord.x + 1 > m_size.x - 1)
    {
        obstacle = 1;
    }

    if (voxelCoord.y - 1 < 0)
    {
        obstacle = 1;
    }
    if (voxelCoord.y + 1 > m_size.y - 1)
    {
        obstacle = 1;
    }

    if (voxelCoord.z - 1 < 0)
    {
        obstacle = 1;
    }
    if (voxelCoord.z + 1 > m_size.z - 1)
    {
        obstacle = 1;
    }

    glm::vec3 curl = glm::vec3(curlObstaclesData.x, curlObstaclesData.y, curlObstaclesData.z);
    curlObstaclesData = glm::vec4(curl, obstacle);
}

glm::vec3 FireSimulation::GetAdvectedPosCoords(glm::vec3 voxelCoord, glm::vec4& velocityDensityData)
{
    voxelCoord -= dt * 1.0f * glm::vec3(velocityDensityData.x, velocityDensityData.y, velocityDensityData.z);
    return voxelCoord;
}

glm::vec3 FireSimulation::SampleVelocityBilinear(glm::vec3 voxelCoord, glm::vec3 size)
{
    glm::ivec3 floorVoxelCoord = glm::ivec3(voxelCoord);
    int x = floorVoxelCoord.x;
    int y = floorVoxelCoord.y;
    int z = floorVoxelCoord.z;

    float fx = voxelCoord.x - x;
    float fy = voxelCoord.y - y;
    float fz = voxelCoord.z - z;

    int xp1 = int(std::min(size.x - 1.0, x + 1.0));
    int yp1 = int(std::min(size.y - 1.0, y + 1.0));
    int zp1 = int(std::min(size.z - 1.0, z + 1.0));

    glm::vec3 x0 = GetVelocityDensityData(glm::vec3(x, y, z)) * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, y, z)) * fx;
    glm::vec3 x1 = GetVelocityDensityData(glm::vec3(x, y, zp1)) * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, y, zp1)) * fx;

    glm::vec3 x2 = GetVelocityDensityData(glm::vec3(x, yp1, z)) * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, yp1, z)) * fx;
    glm::vec3 x3 = GetVelocityDensityData(glm::vec3(x, yp1, zp1)) * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, yp1, zp1)) * fx;

    glm::vec3 z0 = x0 * (1.0f - fz) + x1 * fz;
    glm::vec3 z1 = x2 * (1.0f - fz) + x3 * fz;

    return z0 * (1.0f - fy) + z1 * fy;
}

float FireSimulation::SampleBilinear(bool otherData, glm::vec3 voxelCoord, glm::vec3 size, int index)
{
    glm::ivec3 floorVoxelCoord = glm::ivec3(voxelCoord);
    int x = floorVoxelCoord.x;
    int y = floorVoxelCoord.y;
    int z = floorVoxelCoord.z;

    float fx = voxelCoord.x - x;
    float fy = voxelCoord.y - y;
    float fz = voxelCoord.z - z;

    int xp1 = int(std::min(size.x - 1.f, x + 1.f));
    int yp1 = int(std::min(size.y - 1.f, y + 1.f));
    int zp1 = int(std::min(size.z - 1.f, z + 1.f));

    float x0, x1, x2, x3;

    if (otherData == true)
    {
        x0 = GetPressureTempPhiReactionData(glm::vec3(x, y, z))[index] * (1.0f - fx) + GetPressureTempPhiReactionData(glm::vec3(xp1, y, z))[index] * fx;
        x1 = GetPressureTempPhiReactionData(glm::vec3(x, y, zp1))[index] * (1.0f - fx) + GetPressureTempPhiReactionData(glm::vec3(xp1, y, zp1))[index] * fx;

        x2 = GetPressureTempPhiReactionData(glm::vec3(x, yp1, z))[index] * (1.0f - fx) + GetPressureTempPhiReactionData(glm::vec3(xp1, yp1, z))[index] * fx;
        x3 = GetPressureTempPhiReactionData(glm::vec3(x, yp1, zp1))[index] * (1.0f - fx) + GetPressureTempPhiReactionData(glm::vec3(xp1, yp1, zp1))[index] * fx;
    }
    else
    {
        x0 = GetVelocityDensityData(glm::vec3(x, y, z))[index] * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, y, z))[index] * fx;
        x1 = GetVelocityDensityData(glm::vec3(x, y, zp1))[index] * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, y, zp1))[index] * fx;

        x2 = GetVelocityDensityData(glm::vec3(x, yp1, z))[index] * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, yp1, z))[index] * fx;
        x3 = GetVelocityDensityData(glm::vec3(x, yp1, zp1))[index] * (1.0f - fx) + GetVelocityDensityData(glm::vec3(xp1, yp1, zp1))[index] * fx;
    }

    float z0 = x0 * (1.0f - fz) + x1 * fz;
    float z1 = x2 * (1.0f - fz) + x3 * fz;

    return z0 * (1.0f - fy) + z1 * fy;
}

void FireSimulation::AdvectDensityVelocity(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& velocityDensityData)
{
    float obstacle = curlObstaclesData.w;
    if (obstacle > 0.1)
    {
        velocityDensityData = glm::vec4(0.0);
        return;
    }

    glm::vec3 advectedCoords = GetAdvectedPosCoords(voxelCoord, velocityDensityData);
    glm::vec3 v = SampleVelocityBilinear(advectedCoords, m_size) * m_velocityDissipation;
    float d = SampleBilinear(false, advectedCoords, m_size, 3) * m_densityDissipation;

    velocityDensityData = glm::vec4(v, d);
}

void FireSimulation::AdvectReactionTemperature(glm::vec3 voxelCoord, glm::vec4& pressureTempPhiReactionData, glm::vec4& curlObstaclesData, glm::vec4& velocityDensityData)
{
    float obstacle = curlObstaclesData.w;
    if (obstacle > 0.1)
    {
        pressureTempPhiReactionData = glm::vec4(pressureTempPhiReactionData.x, 0.0, pressureTempPhiReactionData.z, 0.0);
        return;
    }

    glm::vec3 advectedCoords = GetAdvectedPosCoords(voxelCoord, velocityDensityData);
    float temp = SampleBilinear(true, advectedCoords, m_size, 1) * m_temperatureDissipation;
    float reaction = std::max(0.f, SampleBilinear(true, advectedCoords, m_size, 3) - m_reactionDecay);

    pressureTempPhiReactionData = glm::vec4(pressureTempPhiReactionData.x, temp, pressureTempPhiReactionData.z, reaction);
}

void FireSimulation::ApplyBuoyancy(glm::vec3 voxelCoord, glm::vec4& velocityDensityData, glm::vec4& pressureTempPhiReactionData)
{
    float T = pressureTempPhiReactionData.y;
    float D = velocityDensityData.w;
    glm::vec3 V = glm::vec3(velocityDensityData.x, velocityDensityData.y, velocityDensityData.z);

    if (T > m_ambientTemperature)
    {
        V += (dt * (T - m_ambientTemperature) * m_densityBuoyancy - D * m_densityWeight) * glm::vec3(0, 1, 0);
    }

    velocityDensityData = glm::vec4(V, D);
}

int FireSimulation::permute(int x)
{
    int index = x % 255;
    return perm[index];
}

float FireSimulation::grad(int hash, float x, float y)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0);
    return (int((h & 1)) == 0 ? u : -u) + (int((h & 2)) == 0 ? v : -v);
}

float FireSimulation::fade(float t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float FireSimulation::lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float FireSimulation::perlinNoise(glm::vec2 p)
{
    glm::vec2 pi = floor(p);
    glm::vec2 pf = fract(p);

    int X = int(glm::mod(pi.x, 256.f));
    int Y = int(glm::mod(pi.y, 256.f));

    int A = perm[X] + Y;
    int B = perm[X + 1] + Y;

    float res = lerp(
        lerp(grad(perm[A], pf.x, pf.y), grad(perm[B], pf.x - 1.0, pf.y), fade(pf.x)),
        lerp(grad(perm[A + 1], pf.x, pf.y - 1.0), grad(perm[B + 1], pf.x - 1.0, pf.y - 1.0), fade(pf.x)),
        fade(pf.y)
    );
    return res;
}

void FireSimulation::ApplyWind(glm::vec3 voxelCoord, glm::vec4& velocityDensityData)
{
    float D = velocityDensityData.w;
    glm::vec3 V = glm::vec3(velocityDensityData.x, velocityDensityData.y, velocityDensityData.z);

    glm::vec2 noiseInput = (glm::vec2(voxelCoord.x, voxelCoord.y) * m_windNoiseFrequency + glm::vec2(iTime));

    float windX = perlinNoise(glm::vec2(noiseInput.x, 0.f));
    float windY = perlinNoise(glm::vec2(0.f, noiseInput.y));
    glm::vec2 wind = glm::normalize(glm::vec2(windX, windY)) * m_windSpeedFrequency;

    V.z += wind.x;
    V.y += wind.y;

    velocityDensityData = glm::vec4(V, D);
}

void FireSimulation::ApplyImpulse(glm::vec3 voxelCoord, float _Amount, int index, glm::vec4& pressureTempPhiReactionData)
{
    glm::vec3 pos = voxelCoord / m_size - m_inputPos;

    float mag = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z;
    float rad2 = m_inputRadius * m_inputRadius;

    float amount = exp(-mag / rad2) * _Amount * dt;

    pressureTempPhiReactionData[index] += amount;
}

void FireSimulation::ApplyExtinguishmentImpulse(glm::vec3 voxelCoord, glm::vec4& pressureTempPhiReactionData, glm::vec4& velocityDensityData)
{
    float amount = 0.0;
    float reaction = pressureTempPhiReactionData.w;

    if (reaction > 0.0 && reaction < m_reactionExtinguishment)
    {
        amount = m_densityAmount * reaction;
    }

    velocityDensityData.w += amount;
}

void FireSimulation::ComputeVorticityConfinement(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& velocityDensityData)
{
    glm::vec3 idxL = glm::vec3(std::max(0.f, voxelCoord.x - 1), voxelCoord.y, voxelCoord.z);
    glm::vec3 idxR = glm::vec3(std::min(m_size.x - 1, voxelCoord.x + 1), voxelCoord.y, voxelCoord.z);

    glm::vec3 idxB = glm::vec3(voxelCoord.x, std::max(0.f, voxelCoord.y - 1), voxelCoord.z);
    glm::vec3 idxT = glm::vec3(voxelCoord.x, std::min(m_size.y - 1, voxelCoord.y + 1), voxelCoord.z);

    glm::vec3 idxD = glm::vec3(voxelCoord.x, voxelCoord.y, std::max(0.f, voxelCoord.z - 1));
    glm::vec3 idxU = glm::vec3(voxelCoord.x, voxelCoord.y, std::min(m_size.z - 1, voxelCoord.z + 1));

    glm::vec4 L_4 = GetVelocityDensityData(idxL);
    glm::vec3 L = glm::vec3(L_4.x, L_4.y, L_4.z);

    glm::vec4 R_4 = GetVelocityDensityData(idxR);
    glm::vec3 R = glm::vec3(R_4.x, R_4.y, R_4.z);

    glm::vec4 B_4 = GetVelocityDensityData(idxB);
    glm::vec3 B = glm::vec3(B_4.x, B_4.y, B_4.z);

    glm::vec4 T_4 = GetVelocityDensityData(idxT);
    glm::vec3 T = glm::vec3(T_4.x, T_4.y, T_4.z);

    glm::vec4 D_4 = GetVelocityDensityData(idxD);
    glm::vec3 D = glm::vec3(D_4.x, D_4.y, D_4.z);

    glm::vec4 U_4 = GetVelocityDensityData(idxU);
    glm::vec3 U = glm::vec3(U_4.x, U_4.y, U_4.z);;

    glm::vec3 vorticity = glm::vec3(((T.z - B.z) - (U.y - D.y)), ((U.x - D.x) - (R.z - L.z)), ((R.y - L.y) - (T.x - B.x))); \
    vorticity.x *= 0.5f;
    vorticity.y *= 0.5f;
    vorticity.z *= 0.5f;

    float obstacle = curlObstaclesData.w;
    curlObstaclesData = glm::vec4(vorticity, obstacle);

    // confinement
    float omegaL = glm::length(GetCurlObstaclesData(idxL));
    float omegaR = glm::length(GetCurlObstaclesData(idxR));

    float omegaB = glm::length(GetCurlObstaclesData(idxB));
    float omegaT = glm::length(GetCurlObstaclesData(idxT));

    float omegaD = glm::length(GetCurlObstaclesData(idxD));
    float omegaU = glm::length(GetCurlObstaclesData(idxU));

    glm::vec3 omega = glm::vec3(curlObstaclesData.x, curlObstaclesData.y, curlObstaclesData.z);

    glm::vec3 eta = glm::vec3(omegaR - omegaL, omegaT - omegaB, omegaU - omegaD);
    eta.x *= 0.5f;
    eta.y *= 0.5f;
    eta.z *= 0.5f;

    eta = glm::normalize(eta + glm::vec3(0.001, 0.001, 0.001));

    glm::vec3 force = dt * m_vorticityStrength * glm::vec3(eta.y * omega.z - eta.z * omega.y, eta.z * omega.x - eta.x * omega.z, eta.x * omega.y - eta.y * omega.x);

    velocityDensityData += glm::vec4(force, 0.0);
}

void FireSimulation::ComputeDivergence(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData)
{
    glm::vec3 idxL = glm::vec3(std::max(0.f, voxelCoord.x - 1), voxelCoord.y, voxelCoord.z);
    glm::vec3 idxR = glm::vec3(std::min(m_size.x - 1, voxelCoord.x + 1), voxelCoord.y, voxelCoord.z);

    glm::vec3 idxB = glm::vec3(voxelCoord.x, std::max(0.f, voxelCoord.y - 1), voxelCoord.z);
    glm::vec3 idxT = glm::vec3(voxelCoord.x, std::min(m_size.y - 1, voxelCoord.y + 1), voxelCoord.z);

    glm::vec3 idxD = glm::vec3(voxelCoord.x, voxelCoord.y, std::max(0.f, voxelCoord.z - 1));
    glm::vec3 idxU = glm::vec3(voxelCoord.x, voxelCoord.y, std::min(m_size.z - 1, voxelCoord.z + 1));

    glm::vec4 L_4 = GetVelocityDensityData(idxL);
    glm::vec3 L = glm::vec3(L_4.x, L_4.y, L_4.z);

    glm::vec4 R_4 = GetVelocityDensityData(idxR);
    glm::vec3 R = glm::vec3(R_4.x, R_4.y, R_4.z);

    glm::vec4 B_4 = GetVelocityDensityData(idxB);
    glm::vec3 B = glm::vec3(B_4.x, B_4.y, B_4.z);

    glm::vec4 T_4 = GetVelocityDensityData(idxT);
    glm::vec3 T = glm::vec3(T_4.x, T_4.y, T_4.z);

    glm::vec4 D_4 = GetVelocityDensityData(idxD);
    glm::vec3 D = glm::vec3(D_4.x, D_4.y, D_4.z);

    glm::vec4 U_4 = GetVelocityDensityData(idxU);
    glm::vec3 U = glm::vec3(U_4.x, U_4.y, U_4.z);;

    glm::vec3 obstacleVelocity = glm::vec3(0, 0, 0);

    if (GetCurlObstaclesData(idxL).w > 0.1)
    {
        L = obstacleVelocity;
    }
    if (GetCurlObstaclesData(idxR).w > 0.1)
    {
        R = obstacleVelocity;
    }

    if (GetCurlObstaclesData(idxB).w > 0.1)
    {
        B = obstacleVelocity;
    }
    if (GetCurlObstaclesData(idxT).w > 0.1)
    {
        T = obstacleVelocity;
    }

    if (GetCurlObstaclesData(idxD).w > 0.1)
    {
        D = obstacleVelocity;
    }
    if (GetCurlObstaclesData(idxU).w > 0.1)
    {
        U = obstacleVelocity;
    }

    float divergence = 0.5 * ((R.x - L.x) + (T.y - B.y) + (U.z - D.z));

    float obstacle = curlObstaclesData.w;
    curlObstaclesData = glm::vec4(divergence, 0, 0, obstacle);
}

void FireSimulation::ComputePressure(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& pressureTempPhiReactionData)
{
    glm::vec3 idxL = glm::vec3(std::max(0.f, voxelCoord.x - 1), voxelCoord.y, voxelCoord.z);
    glm::vec3 idxR = glm::vec3(std::min(m_size.x - 1, voxelCoord.x + 1), voxelCoord.y, voxelCoord.z);

    glm::vec3 idxB = glm::vec3(voxelCoord.x, std::max(0.f, voxelCoord.y - 1), voxelCoord.z);
    glm::vec3 idxT = glm::vec3(voxelCoord.x, std::min(m_size.y - 1, voxelCoord.y + 1), voxelCoord.z);

    glm::vec3 idxD = glm::vec3(voxelCoord.x, voxelCoord.y, std::max(0.f, voxelCoord.z - 1));
    glm::vec3 idxU = glm::vec3(voxelCoord.x, voxelCoord.y, std::min(m_size.z - 1, voxelCoord.z + 1));

    float L = GetPressureTempPhiReactionData(idxL).x;
    float R = GetPressureTempPhiReactionData(idxR).x;

    float B = GetPressureTempPhiReactionData(idxB).x;
    float T = GetPressureTempPhiReactionData(idxT).x;

    float D = GetPressureTempPhiReactionData(idxD).x;
    float U = GetPressureTempPhiReactionData(idxU).x;

    float C = pressureTempPhiReactionData.x;

    float divergence = curlObstaclesData.x;

    if (GetCurlObstaclesData(idxL).w > 0.1)
    {
        L = C;
    }
    if (GetCurlObstaclesData(idxR).w > 0.1)
    {
        R = C;
    }

    if (GetCurlObstaclesData(idxB).w > 0.1)
    {
        B = C;
    }
    if (GetCurlObstaclesData(idxT).w > 0.1)
    {
        T = C;
    }

    if (GetCurlObstaclesData(idxD).w > 0.1)
    {
        D = C;
    }
    if (GetCurlObstaclesData(idxU).w > 0.1)
    {
        U = C;
    }

    pressureTempPhiReactionData.x = (L + R + B + T + U + D - divergence) / 6.0;
}

void FireSimulation::ComputeProjection(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& pressureTempPhiReactionData, glm::vec4& velocityDensityData)
{
    if (curlObstaclesData.w > 0.1)
    {
        velocityDensityData = glm::vec4(glm::vec3(0.0), velocityDensityData.w);
        return;
    }

    glm::vec3 idxL = glm::vec3(std::max(0.f, voxelCoord.x - 1), voxelCoord.y, voxelCoord.z);
    glm::vec3 idxR = glm::vec3(std::min(m_size.x - 1.0, voxelCoord.x + 1.0), voxelCoord.y, voxelCoord.z);

    glm::vec3 idxB = glm::vec3(voxelCoord.x, std::max(0.f, voxelCoord.y - 1), voxelCoord.z);
    glm::vec3 idxT = glm::vec3(voxelCoord.x, std::min(m_size.y - 1, voxelCoord.y + 1), voxelCoord.z);

    glm::vec3 idxD = glm::vec3(voxelCoord.x, voxelCoord.y, std::max(0.f, voxelCoord.z - 1));
    glm::vec3 idxU = glm::vec3(voxelCoord.x, voxelCoord.y, std::min(m_size.z - 1, voxelCoord.z + 1));

    float L = GetPressureTempPhiReactionData(idxL).x;
    float R = GetPressureTempPhiReactionData(idxR).x;

    float B = GetPressureTempPhiReactionData(idxB).x;
    float T = GetPressureTempPhiReactionData(idxT).x;

    float D = GetPressureTempPhiReactionData(idxD).x;
    float U = GetPressureTempPhiReactionData(idxU).x;

    float C = pressureTempPhiReactionData.x;

    glm::vec3 mask = glm::vec3(1.0);

    if (GetCurlObstaclesData(idxL).w > 0.1)
    {
        L = C;
        mask.x = 0;
    }
    if (GetCurlObstaclesData(idxR).w > 0.1)
    {
        R = C;
        mask.x = 0;
    }

    if (GetCurlObstaclesData(idxB).w > 0.1)
    {
        B = C;
        mask.y = 0;
    }
    if (GetCurlObstaclesData(idxT).w > 0.1)
    {
        T = C;
        mask.y = 0;
    }

    if (GetCurlObstaclesData(idxD).w > 0.1)
    {
        D = C;
        mask.z = 0;
    }
    if (GetCurlObstaclesData(idxU).w > 0.1)
    {
        U = C;
        mask.z = 0;
    }

    glm::vec3 v = glm::vec3(velocityDensityData.x, velocityDensityData.y, velocityDensityData.z) - glm::vec3((R - L) * 0.5f, (T - B) * 0.5f, (U - D) * 0.5f);
    velocityDensityData = glm::vec4(v * mask, velocityDensityData.w);
}

glm::vec4 FireSimulation::GetVelocityDensityData(glm::vec3 voxelCoord)
{
    glm::ivec3 floorVoxelCoord = glm::ivec3(voxelCoord); // Convert float to integer

    // Ensure coordinates are within bounds
    if (floorVoxelCoord.x < 0 || floorVoxelCoord.x >= TEX_WIDTH ||
        floorVoxelCoord.y < 0 || floorVoxelCoord.y >= TEX_HEIGHT ||
        floorVoxelCoord.z < 0 || floorVoxelCoord.z >= TEX_DEPTH) {
        return glm::vec4(0.0f); // Default return for out-of-bounds
    }

    glm::vec4 pixelData;
    glBindTexture(GL_TEXTURE_3D, velocityDensityTexture);

    // Read pixel data from the texture (this is slow and should be used with caution)
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, &pixelData);

    glBindTexture(GL_TEXTURE_3D, 0);
    return pixelData;
}

glm::vec4 FireSimulation::GetPressureTempPhiReactionData(glm::vec3 voxelCoord)
{
    glm::ivec3 floorVoxelCoord = glm::ivec3(voxelCoord); // Convert float to integer

    // Ensure coordinates are within bounds
    if (floorVoxelCoord.x < 0 || floorVoxelCoord.x >= TEX_WIDTH ||
        floorVoxelCoord.y < 0 || floorVoxelCoord.y >= TEX_HEIGHT ||
        floorVoxelCoord.z < 0 || floorVoxelCoord.z >= TEX_DEPTH) {
        return glm::vec4(0.0f); // Default return for out-of-bounds
    }

    glm::vec4 pixelData;
    glBindTexture(GL_TEXTURE_3D, pressureTempPhiReactionTexture);

    // Read pixel data from the texture (this is slow and should be used with caution)
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, &pixelData);

    glBindTexture(GL_TEXTURE_3D, 0);
    return pixelData;
}

glm::vec4 FireSimulation::GetCurlObstaclesData(glm::vec3 voxelCoord)
{
    glm::ivec3 floorVoxelCoord = glm::ivec3(voxelCoord); // Convert float to integer

    // Ensure coordinates are within bounds
    if (floorVoxelCoord.x < 0 || floorVoxelCoord.x >= TEX_WIDTH ||
        floorVoxelCoord.y < 0 || floorVoxelCoord.y >= TEX_HEIGHT ||
        floorVoxelCoord.z < 0 || floorVoxelCoord.z >= TEX_DEPTH) {
        return glm::vec4(0.0f); // Default return for out-of-bounds
    }

    glm::vec4 pixelData;
    glBindTexture(GL_TEXTURE_3D, curlObstaclesTexture);

    // Read pixel data from the texture (this is slow and should be used with caution)
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, &pixelData);

    glBindTexture(GL_TEXTURE_3D, 0);
    return pixelData;
}