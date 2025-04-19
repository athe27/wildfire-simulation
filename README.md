# Wildfire Simulation
## Introduction
Using compute shaders and OpenGL, we implemented a new 3D wildfire simulation program. Using source landscape and heightmap textures, we read from the textures to generate and render the terrain mesh using tessellation.

Mesh instancing is then used to render the thousands of tree instances.

With the compute shader, we simulate the movement of wild fire via wind and different tree species.

## Wildfire Compute Shader

2 2D textures are used to store simulation data (one for reading, one for writing so there are no race conditions).

float CalculateTemperature(): Increases spread of fire if time of day is hot

bool IsImpactedByMouse(vec2 coord, inout vec4 cellData): Checks if user clicked an area and spreads fire

float GetWindSpreadProb(vec2 coord): Performs dot product with direction of fire and wind direction to get fire spread probability

void processCell(vec2 coord, inout vec4 cellData): Puts everything together and makes sure the right probability coefficients are used for cells catching on fire.

## Demo Video
https://www.youtube.com/watch?v=e9ExoZol7Qo