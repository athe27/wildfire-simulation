#include "FireFluidSim.h"
#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

//----CONSTANTS----
const int READ = 0;
const int WRITE = 0;
const int PHI_N_HAT = 0;
const int PHI_N_1_HAT = 1;
//-----------------

enum ADVECTION { NORMAL = 1, BFECC = 2, MACCORMACK = 3 };
const int NUM_THREADS = 8;

//You can change this or even use Time.DeltaTime but large time steps can cause numerical errors
const float TIME_STEP = 0.1f;

//----SIM. CONSTANTS----
enum ADVECTION m_denstyAdvectionType = NORMAL;
enum ADVECTION m_reactionAdvectionType = NORMAL;
int m_width = 128;
int m_height = 128;
int m_depth = 128;
int m_iterations = 10;
float m_vorticityStrength = 1.0f;
float m_densityAmount = 1.0f;
float m_densityDissipation = 0.999f;
float m_densityBuoyancy = 1.0f;
float m_densityWeight = 0.0125f;
float m_temperatureAmount = 10.0f;
float m_temperatureDissipation = 0.995f;
float m_reactionAmount = 1.0f;
float m_reactionDecay = 0.001f;
float m_reactionExtinguishment = 0.01f;
float m_velocityDissipation = 0.995f;
float m_inputRadius = 0.04f;
Vector4 m_inputPos = Vector4(0.5f, 0.1f, 0.5f, 0.0f);
//----------------------

float m_ambientTemperature = 0.0f;
ComputeShader m_applyImpulse, m_applyAdvect, m_computeVorticity;
ComputeShader m_computeDivergence, m_computeJacobi, m_computeProjection;
ComputeShader m_computeConfinement, m_computeObstacles, m_applyBuoyancy;

Vector4 m_size;

//SSBOs (replaces ComputeBuffer in Unity)
GLuint m_density, m_velocity, m_pressure, m_temperature, m_phi, m_reaction;
GLuint m_temp3f, m_obstacles;

void InitializeSSBO(GLuint& ssbo, size_t size, void* data, GLuint bindingPoint) {
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssbo); // Binding to the specified point
}

void InitAllBuffers() {
	int NUM_ELEMENTS = m_width * m_height * m_depth;
	InitializeSSBO(m_density, sizeof(Vector4) * NUM_ELEMENTS, initialDensityData, 0); // Binding Point 0
	InitializeSSBO(m_velocity, sizeof(Vector4) * NUM_ELEMENTS, initialVelocityData, 1); // Binding Point 1
	InitializeSSBO(m_pressure, sizeof(float) * NUM_ELEMENTS, initialPressureData, 2); // Binding Point 2
	InitializeSSBO(m_temperature, sizeof(float) * NUM_ELEMENTS, initialTemperatureData, 3); // Binding Point 3
	InitializeSSBO(m_phi, sizeof(Vector4) * NUM_ELEMENTS, initialPhiData, 4); // Binding Point 4
	InitializeSSBO(m_reaction, sizeof(float) * NUM_ELEMENTS, initialReactionData, 5); // Binding Point 5

	// For single-purpose buffers
	InitializeSSBO(m_temp3f, sizeof(Vector4) * NUM_ELEMENTS, initialTemp3fData, 6); // Binding Point 6
	InitializeSSBO(m_obstacles, sizeof(Vector4) * NUM_ELEMENTS, initialObstaclesData, 7); // Binding Point 7
}


void FireFluidSimulator::Start() {
	m_width = MathUtilities::ClosestPowerOfTwo(m_width);
	m_height = MathUtilities::ClosestPowerOfTwo(m_height);
	m_depth = MathUtilities::ClosestPowerOfTwo(m_depth);

	// Put all dimension sizes in a vector for easy parsing to shader and also prevents user changing
	// dimension sizes during play
	//m_size = new Vector4(m_width, m_height, m_depth, 0.0f);

	int SIZE = m_width * m_height * m_depth;

	//m_density = new ComputeBuffer[2];
	//m_density[READ] = new ComputeBuffer(SIZE, sizeof(float));
	//m_density[WRITE] = new ComputeBuffer(SIZE, sizeof(float));

	//m_temperature = new ComputeBuffer[2];
	//m_temperature[READ] = new ComputeBuffer(SIZE, sizeof(float));
	//m_temperature[WRITE] = new ComputeBuffer(SIZE, sizeof(float));

	//m_reaction = new ComputeBuffer[2];
	//m_reaction[READ] = new ComputeBuffer(SIZE, sizeof(float));
	//m_reaction[WRITE] = new ComputeBuffer(SIZE, sizeof(float));

	//m_phi = new ComputeBuffer[2];
	//m_phi[READ] = new ComputeBuffer(SIZE, sizeof(float));
	//m_phi[WRITE] = new ComputeBuffer(SIZE, sizeof(float));

	//m_velocity = new ComputeBuffer[2];
	//m_velocity[READ] = new ComputeBuffer(SIZE, sizeof(float) * 3);
	//m_velocity[WRITE] = new ComputeBuffer(SIZE, sizeof(float) * 3);

	//m_pressure = new ComputeBuffer[2];
	//m_pressure[READ] = new ComputeBuffer(SIZE, sizeof(float));
	//m_pressure[WRITE] = new ComputeBuffer(SIZE, sizeof(float));

	//m_obstacles = new ComputeBuffer(SIZE, sizeof(float));

	//m_temp3f = new ComputeBuffer(SIZE, sizeof(float) * 3);
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

	// Subtract the pressure field from the velocity field enforcing the divergence free conditions
	ComputeProjection();
}

void FireFluidSimulator::ComputeObstacles()
{
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
