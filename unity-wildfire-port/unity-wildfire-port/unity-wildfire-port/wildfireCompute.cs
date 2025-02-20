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

uniform float FIRE_PROB = 0.01f;
uniform float FLAMMABLE_PROBABILITY_FOR_GRASS = 0.0f;
uniform float FLAMMABLE_PROBABILITY_FOR_WATER = 0.0f;
uniform float FLAMMABLE_PROBABILITY_FOR_BEDROCK = 0.1f;
uniform float FLAMMABLE_PROBABILITY_FOR_TREE = 0.75f;

uniform float GRASS_REGROW_PROBABILITY = 0.1f;
uniform float TREE_REGROW_PROBABILITY = 0.2f;

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

        bool neighborOnFire = false;

        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                if (i == 0 && j == 0)
                {
                    continue;
                }

                vec4 otherCellData = GetCellData(coord + vec2(i, j));
                int otherCellState = GetState(otherCellData);

                if (otherCellState == STATE_ON_FIRE)
                {
                    neighborOnFire = true;
                }
            }
        }

        float fireCatchProb = random(coord);

        if (neighborOnFire || FIRE_PROB > fireCatchProb)
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
            if (flammableProb > randomProb)
            {
                SetState(cellData, STATE_ON_FIRE);
            }
        }
    } 
    else if (cellState == STATE_ON_FIRE)
    {
        SetState(cellData, STATE_DESTROYED);
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
