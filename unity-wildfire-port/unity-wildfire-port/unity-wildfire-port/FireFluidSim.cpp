#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

#include <iostream>

#include "FireFluidSim.h"

FireFluidSimulator::FireFluidSimulator() {}
FireFluidSimulator::~FireFluidSimulator() {}

void FireFluidSimulator::InitializeSSBO(GLuint& ssbo, size_t size, void* data, GLuint bindingPoint) {
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssbo); // Binding to the specified point
}

void FireFluidSimulator::InitAllBuffers() {
	int NUM_ELEMENTS = m_width * m_height * m_depth;
	//InitializeSSBO(m_density, sizeof(glm::vec4) * NUM_ELEMENTS, initialDensityData, 0); // Binding Point 0
	//InitializeSSBO(m_velocity, sizeof(glm::vec4) * NUM_ELEMENTS, initialVelocityData, 1); // Binding Point 1
	//InitializeSSBO(m_pressure, sizeof(float) * NUM_ELEMENTS, initialPressureData, 2); // Binding Point 2
	//InitializeSSBO(m_temperature, sizeof(float) * NUM_ELEMENTS, initialTemperatureData, 3); // Binding Point 3
	//InitializeSSBO(m_phi, sizeof(glm::vec4) * NUM_ELEMENTS, initialPhiData, 4); // Binding Point 4
	//InitializeSSBO(m_reaction, sizeof(float) * NUM_ELEMENTS, initialReactionData, 5); // Binding Point 5

	//// For single-purpose buffers
	//InitializeSSBO(m_temp3f, sizeof(glm::vec4) * NUM_ELEMENTS, initialTemp3fData, 6); // Binding Point 6


	//InitializeSSBO(m_obstacles, sizeof(float) * NUM_ELEMENTS, initialObstaclesData, 7); // Binding Point 7
	float initialObstacleData = 0.f;
	InitializeSSBO(m_obstacles, sizeof(float), &initialObstacleData, 1); // Binding Point 7
}

void FireFluidSimulator::Start() {
	m_width = MathUtilities::ClosestPowerOfTwo(m_width);
	m_height = MathUtilities::ClosestPowerOfTwo(m_height);
	m_depth = MathUtilities::ClosestPowerOfTwo(m_depth);

	// Put all dimension sizes in a vector for easy parsing.
	m_size = glm::vec4(m_width, m_height, m_depth, 0.f);

	InitAllBuffers();
	ComputeObstacles();
}

void FireFluidSimulator::Update(float DeltaTime)
{
	float dt = TIME_STEP;

	// First off advect any buffers that contain physical quantities like density or temperature by the 
	// velocity field. Advection is what moves values around.
	ApplyAdvection(dt, m_temperatureDissipation, 0.f);

	switch (densityAdvectionType) {
	case EAdvectionType::BFECC:
		//ApplyAdvection(dt, 1.0f, 0.0f, m_density, 1.0f); //advect forward into write buffer
		//ApplyAdvection(dt, 1.0f, 0.0f, m_density[READ], m_phi[PHI_N_HAT], -1.0f); //advect back into phi_n_hat buffer
		//ApplyAdvectionBFECC(dt, m_densityDissipation, 0.0f, m_density); //advect using BFECC
		break;
	case EAdvectionType::MACCORMACK:
		break;
	case EAdvectionType::NORMAL:
		ApplyAdvection(dt, m_densityDissipation, 0.f, 1.f);
		break;
	}

	// Apply the advection velocity.
	ApplyAdvectionVelocity(dt);

	// Apply the effect the sinking colder smoke has on the velocity field.
	ApplyBuoyancy(dt);

	// Add a certain amount of reaction (fire) and temperature
	ApplyImpulse(dt, m_reactionAmount);
	ApplyImpulse(dt, m_temperatureAmount);

	// The smoke is formed when the reaction is extinguished. When the reaction amount
	// falls below the extinguishment factor smoke is added
	ApplyExtinguishmentImpulse(dt, m_densityAmount);

	// The fuild sim math tends to remove the swirling movement of fluids.
	// This step will try and add it back in
	ComputeVorticityConfinement(dt);

	// Compute the divergence of the velocity field. In fluid simulation the
	// fluid is modelled as being incompressible meaning that the volume of the fluid
	// does not change over time. The divergence is the amount the field has deviated from being divergence free
	ComputeDivergence();

	// This computes the pressure need return the fluid to a divergence free condition
	ComputePressure();

	// Subtract the pressure field from the velogcity field enforcing the divergence free conditions
	ComputeProjection();
}

void FireFluidSimulator::ComputeObstacles()
{
	// Original Unity Implementation:
	//m_computeObstacles.SetVector("_Size", m_size);
	//m_computeObstacles.SetBuffer(0, "_Write", m_obstacles);
	//m_computeObstacles.Dispatch(0, (int)m_size.x / NUM_THREADS, (int)m_size.y / NUM_THREADS, (int)m_size.z / NUM_THREADS);

	ComputeShader computeObstaclesShader("computeObstacles.cs");
	computeObstaclesShader.use();

	// Actually set the parameters for the computeObstacles shader object.
	computeObstaclesShader.setVec4("_Size", glm::vec4(m_width, m_height, m_depth, 0.0f));

	// ToDo: Set the actual obstacles buffer data so we can write to it.
	glDispatchCompute((int)m_size.x / NUM_THREADS, (int)m_size.y / NUM_THREADS, (int)m_size.z / NUM_THREADS);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void FireFluidSimulator::ApplyImpulse(float deltaTime, float amount)
{

}

void FireFluidSimulator::ApplyExtinguishmentImpulse(float deltaTime, float amount)
{
}

void FireFluidSimulator::ApplyBuoyancy(float deltaTime)
{
}

void FireFluidSimulator::ApplyAdvection(float deltaTime, float dissipation, float decay, float forward)
{
}

void FireFluidSimulator::ApplyAdvectionBFECC(float deltaTime, float dissipation, float decay)
{
}

void FireFluidSimulator::ApplyAdvectionMacCormack(float deltaTime, float dissipation, float decay)
{
}

void FireFluidSimulator::ApplyAdvectionVelocity(float deltaTime)
{
}

void FireFluidSimulator::ComputeVorticityConfinement(float deltaTime)
{
}

void FireFluidSimulator::ComputeDivergence()
{
}

void FireFluidSimulator::ComputePressure()
{
}

void FireFluidSimulator::ComputeProjection()
{
}
