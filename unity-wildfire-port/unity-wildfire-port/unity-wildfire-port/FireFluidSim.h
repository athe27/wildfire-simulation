#include "Utilities.h"

class FireFluidSimulator
{
	static constexpr int READ = 0;
	static constexpr int WRITE = 1;
	static constexpr int PHI_N_HAT = 0;
	static constexpr int PHI_N_1_HAT = 1;

	static constexpr int NUM_THREADS = 8;

	// We can make this be set to the DeltaTime, but this was the implementation mentioned in the Unity project.
	static constexpr float TIME_STEP = 0.1f;

public:
	FireFluidSimulator();
	~FireFluidSimulator();

	EAdvectionType densityAdvectionType = EAdvectionType::NORMAL;
	EAdvectionType reactionAdvectionType = EAdvectionType::NORMAL;

	int m_width = 128;
	int m_height = 128;
	int m_depth = 128;
	int m_iterations = 10;

	float m_vorticityStrength = 1.f;
	float m_densityAmount = 1.f;
	float m_densityDissipation = 0.999f;
	float m_densityBuoyancy = 1.0f;
	float m_densityWeight = 0.0125f;
	float m_temperatureAmount = 10.f;
	float m_temperatureDissipation = 0.995f;
	float m_reactionAmount = 1.f;
	float m_reactionDecay = 0.001f;
	float m_reactionExtinguishment = 0.01f;
	float m_velocityDissipation = 0.995f;
	float m_inputRadius = 0.04f;

	glm::vec4 m_size;

	float m_ambientTemperature = 0.f;

	//public ComputeShader m_applyImpulse, m_applyAdvect, m_computeVorticity;
	//public ComputeShader m_computeDivergence, m_computeJacobi, m_computeProjection;
	//public ComputeShader m_computeConfinement, m_computeObstacles, m_applyBuoyancy;

	//Vector4 m_size;
	//ComputeBuffer[] m_density, m_velocity, m_pressure, m_temperature, m_phi, m_reaction;
	//ComputeBuffer m_temp3f, m_obstacles;

	void Start();
	void Update(float DeltaTime);

private:

	void ComputeObstacles();
	void ApplyImpulse(float deltaTime, float amount);
	void ApplyExtinguishmentImpulse(float deltaTime, float amount);
	void ApplyBuoyancy(float deltaTime);
	void ApplyAdvection(float deltaTime, float dissipation, float decay, float forward = 1.f);
	void ApplyAdvectionBFECC(float deltaTime, float dissipation, float decay);
	void ApplyAdvectionMacCormack(float deltaTime, float dissipation, float decay);
	void ApplyAdvectionVelocity(float deltaTime);
	void ComputeVorticityConfinement(float deltaTime);
	void ComputeDivergence();
	void ComputePressure();
	void ComputeProjection();

};