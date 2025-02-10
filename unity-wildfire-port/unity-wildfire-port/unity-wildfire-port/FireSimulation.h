#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct Vector3D
{
	float X = 0.f;
	float Y = 0.f;
	float Z = 0.f;
};

struct Vector4D
{
	float X = 0.f;
	float Y = 0.f;
	float Z = 0.f;
	float W = 0.f;
};

class FireSimulation {

public:
	void RunFireSimulation(float deltaTime, GLuint textures[3]);

	void RunAllComputeSimulation(std::vector<glm::vec4>& velocityDensityData, std::vector<glm::vec4>& pressureTempData, std::vector<glm::vec4>& curlObstaclesData);

	void RunThisComputeSimulation(glm::vec4& velocityDensityData, glm::vec4& pressureTempData, glm::vec4& curlObstaclesData, glm::vec3& voxelCoord);

	// OpenGL texture handles
	GLuint velocityDensityTexture = 0;
	GLuint pressureTempPhiReactionTexture = 0;
	GLuint curlObstaclesTexture = 0;

private:

	static constexpr int M_SIZE_X = 0;
	static constexpr int M_SIZE_Y = 0;
	static constexpr int M_SIZE_Z = 0;

	// Permutation table for random gradient vectors
	int perm[256] = {
		151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
		140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
		247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
		57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
		74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
		60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
		65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 195,
		30, 42, 102, 198, 152, 98, 113, 219, 122, 138, 224, 14, 184, 197, 5, 83
	};

	void InitializeTexture(GLuint& texture);
	void UpdateTexture(GLuint& texture, std::vector<glm::vec4>& data);

	void ComputeObstacles(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData);
	glm::vec3 GetAdvectedPosCoords(glm::vec3 voxelCoord, glm::vec4& velocityDensityData);
	glm::vec3 SampleVelocityBilinear(glm::vec3 voxelCoord, glm::vec3 size);

	float SampleBilinear(bool otherData, glm::vec3 voxelCoord, glm::vec3 size, int index);

	void AdvectDensityVelocity(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& velocityDensityData);

	void AdvectReactionTemperature(glm::vec3 voxelCoord, glm::vec4& pressureTempPhiReactionData, glm::vec4& curlObstaclesData, glm::vec4& velocityDensityData);

	void ApplyBuoyancy(glm::vec3 voxelCoord, glm::vec4& velocityDensityData, glm::vec4& pressureTempPhiReactionData);

	int permute(int x);

	float grad(int hash, float x, float y);

	float fade(float t);

	float lerp(float a, float b, float t);

	float perlinNoise(glm::vec2 p);

	void ApplyWind(glm::vec3 voxelCoord, glm::vec4& velocityDensityData);

	void ApplyImpulse(glm::vec3 voxelCoord, float _Amount, int index, glm::vec4& pressureTempPhiReactionData);

	void ApplyExtinguishmentImpulse(glm::vec3 voxelCoord, glm::vec4& pressureTempPhiReactionData, glm::vec4& velocityDensityData);

	void ComputeVorticityConfinement(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& velocityDensityData);

	void ComputeDivergence(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData);

	void ComputePressure(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& pressureTempPhiReactionData);

	void ComputeProjection(glm::vec3 voxelCoord, glm::vec4& curlObstaclesData, glm::vec4& pressureTempPhiReactionData, glm::vec4& velocityDensityData);

	glm::vec4 GetVelocityDensityData(glm::vec3 voxelCoord);
	glm::vec4 GetPressureTempPhiReactionData(glm::vec3 voxelCoord);
	glm::vec4 GetCurlObstaclesData(glm::vec3 voxelCoord);

	float dt = 0.f;
	double iTime = 0.0;

	glm::vec3 m_size;

	int m_iterations;
	float m_vorticityStrength;
	float m_densityAmount;
	float m_densityDissipation;
	float m_densityBuoyancy;
	float m_densityWeight;
	float m_temperatureAmount;
	float m_temperatureDissipation;
	float m_reactionAmount;
	float m_reactionDecay;
	float m_reactionExtinguishment;
	float m_velocityDissipation;
	float m_inputRadius;
	float m_ambientTemperature;
	glm::vec3 m_inputPos;

	// Wind Gust Based on Mouse Input
	bool gustActive;
	glm::vec2 gustPosition;
	
	float gustStrength;

	float m_windNoiseFrequency;
	float m_windSpeedFrequency;

	double lastUpdateTime;
};