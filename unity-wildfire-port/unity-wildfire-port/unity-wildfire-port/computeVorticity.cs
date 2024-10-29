# version 430 core

#define NUM_THREADS 8

layout(location = 0) uniform vec4 _Size; // Size of the grid

layout(rw) buffer WriteBuffer
{
	vec3 _Write []; // RWStructuredBuffer equivalent
};

layout(constant) buffer VelocityBuffer
{
	vec3 _Velocity []; // StructuredBuffer for Velocity
};

void CSMain()
{
	// Get the global invocation ID
	ivec3 id = ivec3(gl_GlobalInvocationID);

	// Calculate the index for the current cell
	int idx = id.x + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

	// Calculate neighboring indices
	int idxL = max(0, id.x - 1) + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);
	int idxR = min(int(_Size.x) - 1, id.x + 1) + id.y * int(_Size.x) + id.z * int(_Size.x * _Size.y);

	int idxB = id.x + max(0, id.y - 1) * int(_Size.x) + id.z * int(_Size.x * _Size.y);
	int idxT = id.x + min(int(_Size.y) - 1, id.y + 1) * int(_Size.x) + id.z * int(_Size.x * _Size.y);

	int idxD = id.x + id.y * int(_Size.x) + max(0, id.z - 1) * int(_Size.x * _Size.y);
	int idxU = id.x + id.y * int(_Size.x) + min(int(_Size.z) - 1, id.z + 1) * int(_Size.x * _Size.y);

	// Fetch velocities from neighboring indices
	vec3 L = _Velocity[idxL];
	vec3 R = _Velocity[idxR];

	vec3 B = _Velocity[idxB];
	vec3 T = _Velocity[idxT];

	vec3 D = _Velocity[idxD];
	vec3 U = _Velocity[idxU];

	// Calculate vorticity
	vec3 vorticity = 0.5 * vec3(
		(T.z - B.z) - (U.y - D.y),
		(U.x - D.x) - (R.z - L.z),
		(R.y - L.y) - (T.x - B.x)
	);

	// Write the calculated vorticity to the output
	_Write[idx] = vorticity;
}
