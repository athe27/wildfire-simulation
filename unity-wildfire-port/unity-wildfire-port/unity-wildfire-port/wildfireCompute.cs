#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// ----------------------------------------------------------------------------
//
// Macros for materials
//
// ----------------------------------------------------------------------------
#define MATERIAL_GRASS 0
#define MATERIAL_WATER 1
#define MATERIAL_BEDROCK 2
#define MATERIAL_TREE 3

// ----------------------------------------------------------------------------
//
// Macros for states
//
// ----------------------------------------------------------------------------
#define STATE_NOT_ON_FIRE 0
#define STATE_ON_FIRE 1
#define STATE_DESTROYED 2

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------
layout(rgba32f, binding = 0) uniform image2D materialStateHeightTexture_READ;
layout(rgba32f, binding = 1) uniform image2D materialStateHeightTexture_WRITE;

layout(location = 0) uniform float iTime;
uniform int frameCounter = 0;
uniform bool mouseDown;
uniform vec2 mousePos;

uniform int windDirectionIndex = 6;

uniform float FIRE_PROB = 0.0f;
uniform float FLAMMABLE_PROBABILITY_FOR_GRASS = 0.0f;
uniform float FLAMMABLE_PROBABILITY_FOR_WATER = 0.0f;
uniform float FLAMMABLE_PROBABILITY_FOR_BEDROCK = 0.1f;
uniform float FLAMMABLE_PROBABILITY_FOR_TREE = 0.75f;

uniform float GRASS_REGROW_PROBABILITY = 0.0f;
uniform float TREE_REGROW_PROBABILITY = 0.0f;

uniform bool USE_TEMP = true;
uniform bool USE_WIND = true;

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

// Pseudo-random number generator
vec3 hash33(vec3 p3)
{
    p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

float random(vec2 coord)
{
    // Create a 3D vector from our inputs
    vec3 p = vec3(coord.x, coord.y, iTime);

    // Add some large prime numbers to avoid patterns
    p *= vec3(61.0, 157.0, 367.0);

    // Use the first component of our hash
    return hash33(p).x;
}

// Get cell data from texture
vec4 GetCellData(vec2 coord)
{
    ivec2 icoord = ivec2(coord);
    return imageLoad(materialStateHeightTexture_READ, icoord);
}

// Get material type as an integer
int GetMaterial(vec4 cellData)
{
    return int(cellData.x);
}

// Set the material of a cell
void SetMaterial(inout vec4 cellData, int material)
{
    cellData.x = float(material);
}

// Get state as an integer
int GetState(vec4 cellData)
{
    return int(cellData.y);
}

// Set the state of a cell
void SetState(inout vec4 cellData, int state)
{
    cellData.y = float(state);
}

// Get height value from cell data
float GetHeight(vec4 cellData)
{
    return cellData.z;
}

float CalculateTemperature()
{
    if (!USE_TEMP)
    {
        return 20.0f; // Default temperature if system is disabled
    }

    int hourOfDay = frameCounter % 24;
    float average_temp = 20.0f;
    float amplitude = 5.0f;

    // Use sinusoidal pattern similar to C++ implementation
    float temp = average_temp + amplitude * sin(2.0 * 3.14159265 * float(hourOfDay) / 24.0 - 3.14159265 / 2.0);

    // Add some noise
    temp += (random(vec2(hourOfDay, frameCounter)) - 0.5) * 2.0;

    return temp;
}

// Check if neighbors in wind direction are on fire
bool CheckNeighborOnFire(vec2 coord)
{
    if (!USE_WIND)
    {
        // Check all neighbors if wind system is disabled
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                if (i == 0 && j == 0) continue;

                vec4 otherCellData = GetCellData(coord + vec2(i, j));
                int otherCellState = GetState(otherCellData);

                if (otherCellState == STATE_ON_FIRE)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // Define wind direction offsets based on windDirectionIndex
    // 0: calm, 1: S, 2: N, 3: W, 4: E, 5: SW, 6: SE, 7: NW, 8: NE
    bool neighborOnFire = false;

    switch (windDirectionIndex)
    {
        case 0: // Calm - check all directions
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (i == 0 && j == 0) continue;

                    vec4 otherCellData = GetCellData(coord + vec2(i, j));
                    int otherCellState = GetState(otherCellData);

                    if (otherCellState == STATE_ON_FIRE)
                    {
                        neighborOnFire = true;
                    }
                }
            }
            break;

        case 1: // South - check north, northeast, northwest, east, west
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 0; j++)
                {
                    if (i == 0 && j == 0) continue;

                    vec4 otherCellData = GetCellData(coord + vec2(i, j));
                    int otherCellState = GetState(otherCellData);

                    if (otherCellState == STATE_ON_FIRE)
                    {
                        neighborOnFire = true;
                    }
                }
            }
            break;

        case 2: // North - check south, southeast, southwest, east, west
            for (int i = -1; i <= 1; i++)
            {
                for (int j = 0; j <= 1; j++)
                {
                    if (i == 0 && j == 0) continue;

                    vec4 otherCellData = GetCellData(coord + vec2(i, j));
                    int otherCellState = GetState(otherCellData);

                    if (otherCellState == STATE_ON_FIRE)
                    {
                        neighborOnFire = true;
                    }
                }
            }
            break;

        case 3: // West - check east, northeast, southeast, north, south
            for (int i = -1; i <= 0; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (i == 0 && j == 0) continue;

                    vec4 otherCellData = GetCellData(coord + vec2(i, j));
                    int otherCellState = GetState(otherCellData);

                    if (otherCellState == STATE_ON_FIRE)
                    {
                        neighborOnFire = true;
                    }
                }
            }
            break;

        case 4: // East - check west, northwest, southwest, north, south
            for (int i = 0; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (i == 0 && j == 0) continue;

                    vec4 otherCellData = GetCellData(coord + vec2(i, j));
                    int otherCellState = GetState(otherCellData);

                    if (otherCellState == STATE_ON_FIRE)
                    {
                        neighborOnFire = true;
                    }
                }
            }
            break;

        case 5: // Southwest - check northeast, north, east
            if (GetState(GetCellData(coord + vec2(-1, -1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(0, -1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(-1, 0))) == STATE_ON_FIRE)
            {
                neighborOnFire = true;
            }
            break;

        case 6: // Southeast - check northwest, north, west
            if (GetState(GetCellData(coord + vec2(1, -1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(0, -1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(1, 0))) == STATE_ON_FIRE)
            {
                neighborOnFire = true;
            }
            break;

        case 7: // Northwest - check southeast, south, east
            if (GetState(GetCellData(coord + vec2(-1, 1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(0, 1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(-1, 0))) == STATE_ON_FIRE)
            {
                neighborOnFire = true;
            }
            break;

        case 8: // Northeast - check southwest, south, west
            if (GetState(GetCellData(coord + vec2(1, 1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(0, 1))) == STATE_ON_FIRE ||
                GetState(GetCellData(coord + vec2(1, 0))) == STATE_ON_FIRE)
            {
                neighborOnFire = true;
            }
            break;
    }

    return neighborOnFire;
}

// mouse fire spawn
bool IsImpactedByMouse(vec2 coord, inout vec4 cellData)
{
    if (mouseDown == true)
    {
        ivec2 computeSize = ivec2(gl_NumWorkGroups.xy);
        vec2 fireCenter = computeSize * mousePos;

        float distance = length(fireCenter - coord);

        if (distance < 10)
        {
            return true;
        }
    }

    return false;
}

// Process a single cell
void processCell(vec2 coord, inout vec4 cellData)
{
    int cellMaterial = GetMaterial(cellData);
    int cellState = GetState(cellData);

    // TODO: Replace with wind implementation and add temperature factors.
    if (cellState == STATE_NOT_ON_FIRE)
    {
        if (cellMaterial == MATERIAL_GRASS)
        {
            float regrowTreeProb = TREE_REGROW_PROBABILITY;
            float newTreeProb = random(coord);
            if (regrowTreeProb > newTreeProb)
            {
                SetMaterial(cellData, MATERIAL_TREE);
                return;
            }
        }

        bool neighborOnFire = CheckNeighborOnFire(coord);

        float fireCatchProb = random(coord);

        if (neighborOnFire || FIRE_PROB > fireCatchProb || IsImpactedByMouse(coord, cellData))
        {
            float flammableProb = 0.0f;
            if (cellMaterial == MATERIAL_GRASS)
            {
                flammableProb = FLAMMABLE_PROBABILITY_FOR_GRASS;
            }
            else if (cellMaterial == MATERIAL_WATER)
            {
                flammableProb = FLAMMABLE_PROBABILITY_FOR_WATER;
            }
            else if (cellMaterial == MATERIAL_BEDROCK)
            {
                flammableProb = FLAMMABLE_PROBABILITY_FOR_BEDROCK;
            }
            else if (cellMaterial == MATERIAL_TREE)
            {
                flammableProb = FLAMMABLE_PROBABILITY_FOR_TREE;
            }

            float randomProb = random(coord);

            float currentTemperature = CalculateTemperature();
            bool isHot = currentTemperature > 25.0f;
            if (isHot)
            {
                randomProb *= 2;
            }
            
            if (flammableProb > randomProb)
            {
                SetState(cellData, STATE_ON_FIRE);
            }
        }
    } 
    else if (cellState == STATE_ON_FIRE)
    {
        float randomProb = random(coord);
        if (randomProb < 0.01f)
        {
            SetState(cellData, STATE_DESTROYED);
        }
    }
    else if (cellState == STATE_DESTROYED)
    {
        float regrowGrassProb = GRASS_REGROW_PROBABILITY;
        float newGrassProb = random(coord);
        if (regrowGrassProb > newGrassProb)
        {
            SetState(cellData, STATE_NOT_ON_FIRE);
            SetMaterial(cellData, MATERIAL_GRASS);
        }
    }
    
}

// Main function
void main()
{
    ivec3 voxelCoord = ivec3(gl_GlobalInvocationID);

    vec2 coord = voxelCoord.xy;
    vec4 cellData = GetCellData(coord);

    processCell(coord, cellData);

    imageStore(materialStateHeightTexture_WRITE, voxelCoord.xy, cellData);
}
