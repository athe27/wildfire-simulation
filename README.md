Example of writing to a 3d texture in a compute shader and reading from fragment shader. The compute shader writes to a 128x128x128 texture the rgb corresponding to the coordinates and the fragment shader renders every XY layer and goes through the Z axis when time passes.

Clone the repo and open the .sln file and press the play button and I think it should work.

Useful Things to Read:
https://learnopengl.com/Guest-Articles/2022/Compute-Shaders/Introduction
https://learnopengl.com/Getting-started/Textures

What it looks like:
https://gyazo.com/a6b5f4d639566e2fe31df08d99ca35d1

## Wildfire Compute Shader

2 2D textures are used to store simulation data (one for reading, one for writing so there are no race conditions).

float CalculateTemperature(): Increases spread of fire if time of day is hot

bool IsImpactedByMouse(vec2 coord, inout vec4 cellData): Checks if user clicked an area and spreads fire

float GetWindSpreadProb(vec2 coord): Performs dot product with direction of fire and wind direction to get fire spread probability

void processCell(vec2 coord, inout vec4 cellData): Puts everything together and makes sure the right probability coefficients are used for cells catching on fire.
