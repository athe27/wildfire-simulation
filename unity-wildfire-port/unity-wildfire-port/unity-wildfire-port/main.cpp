#define _CRT_SECURE_NO_WARNINGS

#pragma region Includes

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_t.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <iomanip>
#include <random>
#include "Model.h"

#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma endregion Includes

////////////////////////////////////////////////////////////////////
/// MACROS AND CONSTANT VALUES
////////////////////////////////////////////////////////////////////

#define MATERIAL_GRASS 0.0f
#define MATERIAL_WATER 1.0f
#define MATERIAL_BEDROCK 2.0f
#define MATERIAL_TREE_1 3.0f
#define MATERIAL_TREE_2 4.0f
#define MATERIAL_TREE_3 5.0f

#define STATE_NOT_ON_FIRE 0.0f
#define STATE_ON_FIRE 1.0f
#define STATE_DESTROYED 2.0f

constexpr unsigned int SCREEN_WIDTH = 800;
constexpr unsigned int SCREEN_HEIGHT = 600;
constexpr unsigned int WILDFIRE_WIDTH = 2048;
constexpr unsigned int WILDFIRE_HEIGHT = 2048;
constexpr unsigned int NUM_PATCH_PTS = 4;
constexpr unsigned int VERTICES_RESOLUTION_FACTOR = 20;

constexpr unsigned int LANDSCAPE_TEXTURE_INDEX = 1;
constexpr unsigned int WILDFIRE_TEXTURE_INDEX = 2;
constexpr unsigned int HEIGHTMAP_TEXTURE_INDEX = 6;

// Define the file path for the heightmap image.
const char* HEIGHTMAP_FILE_NAME = "HeightMaps/GreatLakeHeightmap.png";

// Define the file path for the landscape image.
const char* LANDSCAPE_FILE_NAME = "Landscapes/species_forest_landscape.png";

const char* TERRAIN_MESH_VERTEX_SHADER = "Shaders/terrainMesh.vs";
const char* TERRAIN_MESH_FRAGMENT_SHADER = "Shaders/terrainMesh.frag";
const char* TERRAIN_MESH_TESSELLATION_CONTROL_SHADER = "Shaders/terrainMesh.tcs";
const char* TERRAIN_MESH_TESSELLATION_EVALUATION_SHADER = "Shaders/terrainMesh.tes";

const char* TREE_FOLIAGE_VERTEX_SHADER = "Shaders/treeModel.vs";
const char* TREE_FOLIAGE_FRAGMENT_SHADER = "Shaders/treeModel.frag";

const char* WILDFIRE_COMPUTE_SHADER = "Shaders/wildfireCompute.cs";

constexpr int TREE_GRID_DIMENSION_X = 4;
constexpr int TREE_GRID_DIMENSION_Y = 4;
constexpr int NUMBER_OF_PIXELS_IN_TREE_GRID = TREE_GRID_DIMENSION_X * TREE_GRID_DIMENSION_Y;
constexpr int NUMBER_OF_TREES_IN_GRID_THRESHOLD = 9;

// Offsets for the 2x2 block
const int TREE_GRID_PIXEL_OFFSETS[NUMBER_OF_PIXELS_IN_TREE_GRID][2] = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0},
    {0, 1}, {1, 1}, {2, 1}, {3, 1},
    {0, 2}, {1, 2}, {2, 2}, {3, 2},
    {0, 3}, {1, 3}, {2, 3}, {3, 3},
};

const int NUMBER_OF_TREE_GRIDS_X = WILDFIRE_HEIGHT / NUMBER_OF_PIXELS_IN_TREE_GRID;
const int NUMBER_OF_TREE_GRIDS_Y = WILDFIRE_WIDTH / NUMBER_OF_PIXELS_IN_TREE_GRID;

// Change this speed to affect how fast you want the camera to zip around the terrain.
constexpr float CAMERA_SPEED = 1000.f;

////////////////////////////////////////////////////////////////////
/// METHODS AND VARIABLES
////////////////////////////////////////////////////////////////////

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Initialize camera at starting location.
Camera camera(glm::vec3(67.0f, 627.5f, 169.9f), glm::vec3(0.0f, 1.0f, 0.0f), -128.1f, -42.4f);
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;

// Measure the current DeltaTime between frames.
float deltaTime = 0.0f;
constexpr float MIN_FRAME_TIME_LIMIT = 1.0 / 60.0;
constexpr float FRAME_RATE_LIMIT = 1.0 / MIN_FRAME_TIME_LIMIT;

bool bIsFirstMouseClick = true;
bool bIsMouseDown = false;
glm::vec2 mousePos = glm::vec2(0.0f, 0.0f);

int TreeModelInstanceCount = 0;

GLFWwindow* mainWindow = nullptr;

////////////////////////////////////////////////////////////////////
/// GENERATE WILDFIRE TEXTURE
////////////////////////////////////////////////////////////////////

GLboolean generateWildfireTexture(GLsizei offset, GLuint* textures, GLsizei width, GLsizei height) {
    
    const size_t pixelCount = width * height;

    // We give each pixel 4 array cells to work with.
    // data[0] - Material Type
    // data[1] - Current State (Not On Fire to Start With)
    // data[2] - World Height Value (Normalized)
    float* texture_data = new float[pixelCount * 4];

    ////////////////////////////////////////////////////////////////////
    /// READ FROM HEIGHTMAP TEXTURE
    ////////////////////////////////////////////////////////////////////

    int heightmap_img_width, heightmap_img_height, heightmap_channels;
    unsigned char* heightmap_image_data = stbi_load(HEIGHTMAP_FILE_NAME, &heightmap_img_width, &heightmap_img_height, &heightmap_channels, STBI_grey);

    if (!heightmap_image_data) {
        std::cerr << "Failed to load heightmap image: " << HEIGHTMAP_FILE_NAME << std::endl;
        return false;
    }

    ////////////////////////////////////////////////////////////////////
    /// READ FROM LANDSCAPE TEXTURE
    ////////////////////////////////////////////////////////////////////

    // landscape image
    int landscape_img_width, landscape_img_height, landscape_channels;
    unsigned char* landscape_image_data = stbi_load(LANDSCAPE_FILE_NAME, &landscape_img_width, &landscape_img_height, &landscape_channels, STBI_rgb);

    if (!landscape_image_data) {
        std::cerr << "Failed to load landscape image: " << LANDSCAPE_FILE_NAME << std::endl;
        return false;
    }

    ////////////////////////////////////////////////////////////////////
    /// CREATE TEXTURE DATA FROM LANDSCAPE AND HEIGHTMAP IMAGES
    ////////////////////////////////////////////////////////////////////

    for (int pixel_y = 0; pixel_y < height; pixel_y++) {
        for (int pixel_x = 0; pixel_x < width; pixel_x++) {
            int pixel_index = pixel_y * width + pixel_x;

            // Determine what the current pixel's data index is in the array.
            int pixel_data_index = pixel_index * 4;

            // Determine the color value of the corresponding landscape image pixel.
            int landscape_image_index = pixel_index * landscape_channels;

            int r = (int)landscape_image_data[landscape_image_index];     // Red channel
            int g = (int)landscape_image_data[landscape_image_index + 1]; // Green channel
            int b = (int)landscape_image_data[landscape_image_index + 2]; // Blue channel

            // Based off the pixel color, 
            if (r == 181 && g == 219 && b == 235) {
                texture_data[pixel_data_index + 0] = MATERIAL_WATER;
            }
            else if (r == 207 && g == 198 && b == 180) {
                texture_data[pixel_data_index + 0] = MATERIAL_BEDROCK;
            }
            else if (r == 121 && g == 150 && b == 114) {
                texture_data[pixel_data_index + 0] = MATERIAL_GRASS;
            }
            else if (r == 32 && g == 99 && b == 84) {
                texture_data[pixel_data_index + 0] = MATERIAL_TREE_1;
            }
            else if (r == 66 && g == 143 && b == 30) {
                texture_data[pixel_data_index + 0] = MATERIAL_TREE_2;
            }
            else if (r == 185 && g == 209 && b == 50) {
                texture_data[pixel_data_index + 0] = MATERIAL_TREE_3;
            }

            // Write the current state value.
            texture_data[pixel_data_index + 1] = STATE_NOT_ON_FIRE;

            // Write the heightmap value.
            texture_data[pixel_data_index + 2] = heightmap_image_data[pixel_index] / 255.0f;
        }
    }
    
    stbi_image_free(heightmap_image_data);
    stbi_image_free(landscape_image_data);

    // Actually bind the OpenGL texture and set up relevant parameters.
    for (GLsizei i = 0; i < 2; i++) {
        GLsizei finalOffset = offset + i;

        glActiveTexture(GL_TEXTURE0 + finalOffset);
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        // Set common parameters
        const GLint wrap = GL_CLAMP_TO_BORDER;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        // Set border to nonsense values
        float borderColor[] = { -1.0f, -1.0f, 0.0f, 1.0f }; // RGBA values
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, texture_data);

        // Alternate between reading and writing.
        if (i == 0) {
            glBindImageTexture(finalOffset, textures[i], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        }
        else {
            glBindImageTexture(finalOffset, textures[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
    }

    // Check for any errors during the process
    GLenum err = glGetError();
    return (err == GL_NO_ERROR);
}

int main()
{

#pragma region Initialize

    ////////////////////////////////////////////////////////////////////
    /// INITIALIZE GLFW
    ////////////////////////////////////////////////////////////////////
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    ////////////////////////////////////////////////////////////////////
    /// CREATE NEW OPENGL WINDOW
    ////////////////////////////////////////////////////////////////////

    mainWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wildfire Simulation", NULL, NULL);

    // If we failed to create a GL window, then just return.
    if (mainWindow == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Ensure that this new window is our main window and can consume input correctly.
    glfwMakeContextCurrent(mainWindow);
    glfwSetFramebufferSizeCallback(mainWindow, framebuffer_size_callback);
    glfwSetMouseButtonCallback(mainWindow, mouse_button_callback);

    // Set up our input call backs.
    glfwSetKeyCallback(mainWindow, key_callback);
    glfwSetCursorPosCallback(mainWindow, mouse_callback);
    glfwSetScrollCallback(mainWindow, scroll_callback);

    // Tell GLFW to capture our mouse.
    glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    ////////////////////////////////////////////////////////////////////
    /// INITIALIZE GLAD
    ////////////////////////////////////////////////////////////////////
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    ////////////////////////////////////////////////////////////////////
    /// CONFIGURE OPENGL GLOABL SETTINGS
    ////////////////////////////////////////////////////////////////////
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ////////////////////////////////////////////////////////////////////
    /// BUILD AND COMPILE ALL SHADERS
    ////////////////////////////////////////////////////////////////////

    Shader terrainMeshShader(TERRAIN_MESH_VERTEX_SHADER, TERRAIN_MESH_FRAGMENT_SHADER, nullptr, TERRAIN_MESH_TESSELLATION_CONTROL_SHADER, TERRAIN_MESH_TESSELLATION_EVALUATION_SHADER);
    
    Model treeModel("Meshes/tree.obj");
    Shader treeModelShader(TREE_FOLIAGE_VERTEX_SHADER, TREE_FOLIAGE_FRAGMENT_SHADER);

    ComputeShader wildfireCompute(WILDFIRE_COMPUTE_SHADER);

#pragma region LoadingHeightMapTexture

    ////////////////////////////////////////////////////////////////////
    /// LOAD HEIGHTMAP TEXTURE FOR TERRAIN SHADER
    ////////////////////////////////////////////////////////////////////

    GLuint heightMapTexture;
    glGenTextures(1, &heightMapTexture);

    // Set the active texture index based on the heightmap texture index.
    glActiveTexture(GL_TEXTURE0 + HEIGHTMAP_TEXTURE_INDEX);

    // All upcoming GL_TEXTURE_2D operations now have effect on the heightMapTexture object.
    glBindTexture(GL_TEXTURE_2D, heightMapTexture);

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Maybe this can be removed?
    stbi_set_flip_vertically_on_load(true);

    // Load the heightmap image, create the texture, and generate mipmaps.
    int heightmapImageWidth, heightmapImageHeight, heightmapImageChannelCount;

    // It is essential that we read the image with RGBA. Otherwise, we will face a crash when trying to generate a texture from it.
    // By default, the heightmap image only has 2 textures.
    unsigned char* heightMapData = stbi_load(HEIGHTMAP_FILE_NAME, &heightmapImageWidth, &heightmapImageHeight, &heightmapImageChannelCount, STBI_rgb_alpha);

    // If we failed to read from the heightmap data, then just return.
    if (heightMapData == nullptr) {
        std::cout << "Failed to load heightmap texture" << std::endl;
        return -1;
    }

    // Generate the OpenGL texture from the heightmap image.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, heightmapImageWidth, heightmapImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, heightMapData);
    glGenerateMipmap(GL_TEXTURE_2D);

    terrainMeshShader.setInt("heightMap", HEIGHTMAP_TEXTURE_INDEX);

    stbi_image_free(heightMapData);

#pragma endregion

#pragma region LoadingComputeShader

    ////////////////////////////////////////////////////////////////////
    /// INITIALIZE COMPUTE SHADER
    ////////////////////////////////////////////////////////////////////

    const GLsizei numWildfireTextures = 2;
    GLuint wildfireTextures[numWildfireTextures];
    glGenTextures(numWildfireTextures, wildfireTextures);

    generateWildfireTexture(2, wildfireTextures, WILDFIRE_WIDTH, WILDFIRE_HEIGHT);

#pragma endregion

#pragma region GenerateVertices

    ////////////////////////////////////////////////////////////////////
    /// CREATE TERRAIN VERTICES
    ////////////////////////////////////////////////////////////////////

    std::vector<float> terrain_vertices;

    // This affects the number of vertices that will be created.
    unsigned int resolutionFactor = VERTICES_RESOLUTION_FACTOR;

    for (unsigned i = 0; i <= resolutionFactor - 1; i++)
    {
        for (unsigned j = 0; j <= resolutionFactor - 1; j++)
        {
            terrain_vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * i / (float)resolutionFactor); // v.x
            terrain_vertices.push_back(0.0f); // v.y
            terrain_vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * j / (float)resolutionFactor); // v.z
            terrain_vertices.push_back(i / (float)resolutionFactor); // u
            terrain_vertices.push_back(j / (float)resolutionFactor); // v

            terrain_vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * (i + 1) / (float)resolutionFactor); // v.x
            terrain_vertices.push_back(0.0f); // v.y
            terrain_vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * j / (float)resolutionFactor); // v.z
            terrain_vertices.push_back((i + 1) / (float)resolutionFactor); // u
            terrain_vertices.push_back(j / (float)resolutionFactor); // v

            terrain_vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * i / (float)resolutionFactor); // v.x
            terrain_vertices.push_back(0.0f); // v.y
            terrain_vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * (j + 1) / (float)resolutionFactor); // v.z
            terrain_vertices.push_back(i / (float)resolutionFactor); // u
            terrain_vertices.push_back((j + 1) / (float)resolutionFactor); // v

            terrain_vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * (i + 1) / (float)resolutionFactor); // v.x
            terrain_vertices.push_back(0.0f); // v.y
            terrain_vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * (j + 1) / (float)resolutionFactor); // v.z
            terrain_vertices.push_back((i + 1) / (float)resolutionFactor); // u
            terrain_vertices.push_back((j + 1) / (float)resolutionFactor); // v
        }
    }

    ////////////////////////////////////////////////////////////////////
    /// CONFIGURE TERRAIN'S OPENGL ATTRIBUTES
    ////////////////////////////////////////////////////////////////////

    unsigned int terrainVAO, terrainVBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);

    glBindVertexArray(terrainVAO);

    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * terrain_vertices.size(), &terrain_vertices[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

    // Make sure to clear the buffer once we are done.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

#pragma endregion TerrainSetUp

#pragma region FoliageSetUp

    ////////////////////////////////////////////////////////////////////
    /// INITIALIZE TREE FOLIAGE INSTANCE RENDERING
    ////////////////////////////////////////////////////////////////////

    const size_t pixelCount = WILDFIRE_WIDTH * WILDFIRE_HEIGHT;

    ////////////////////////////////////////////////////////////////////
    /// LOAD LANDSCAPE IMAGE FROM DIRECTORY
    ////////////////////////////////////////////////////////////////////

    int landscape_img_width, landscape_img_height, landscape_channels;
    unsigned char* landscape_image_data = stbi_load(LANDSCAPE_FILE_NAME, &landscape_img_width, &landscape_img_height, &landscape_channels, STBI_rgb);

    if (!landscape_image_data) {
        std::cerr << "Failed to load landscape image: " << LANDSCAPE_FILE_NAME << std::endl;
        return false;
    }

    ////////////////////////////////////////////////////////////////////
    /// LOAD HEIGHTMAP IMAGE FROM DIRECTORY
    ////////////////////////////////////////////////////////////////////

    int heightmap_img_width, heightmap_img_height, heightmap_channels;
    unsigned char* heightmap_image_data = stbi_load(HEIGHTMAP_FILE_NAME, &heightmap_img_width, &heightmap_img_height, &heightmap_channels, STBI_grey);

    if (!heightmap_image_data) {
        std::cerr << "Failed to load heightmap image: " << HEIGHTMAP_FILE_NAME << std::endl;
        return false;
    }

    ////////////////////////////////////////////////////////////////////
    /// COUNT NUMBER OF TREE PIXELS IN IMAGE
    ////////////////////////////////////////////////////////////////////

    int totalNumberOfTrees = 0;

    for (int y = 0; y < WILDFIRE_HEIGHT; y++) {
        for (int x = 0; x < WILDFIRE_HEIGHT; x++) {
            int index = (y * WILDFIRE_WIDTH + x) * landscape_channels;

            int r = (int)landscape_image_data[index];
            int g = (int)landscape_image_data[index + 1];
            int b = (int)landscape_image_data[index + 2];

            const bool bIsTree1 = r == 32 && g == 99 && b == 84;
            const bool bIsTree2 = r == 66 && g == 143 && b == 30;
            const bool bIsTree3 = r == 185 && g == 209 && b == 50;

            if (bIsTree1 || bIsTree2 || bIsTree3) {
                ++totalNumberOfTrees;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    /// SET UP TREE MODEL TRANSFORM MATRICES
    ////////////////////////////////////////////////////////////////////

    int current_tree_instance_index = 0;

    // Determine the maximum number of trees possible based from the maximum number of grids in each direction.
    TreeModelInstanceCount = NUMBER_OF_TREE_GRIDS_X * NUMBER_OF_TREE_GRIDS_Y;

    glm::mat4* treeModelMatrices = new glm::mat4[TreeModelInstanceCount];

    // Initialize our array of tree model instances.
    for (int index_tree = 0; index_tree < TreeModelInstanceCount; index_tree++) {
        glm::mat4 tree_model = glm::mat4(1.0f);
        tree_model = glm::scale(tree_model, glm::vec3(0.f));
        treeModelMatrices[index_tree] = tree_model;
    }

    int totalTreeMeshCount = 0;

    for (int y = 0; y < NUMBER_OF_TREE_GRIDS_Y; y++) {
        for (int x = 0; x < NUMBER_OF_TREE_GRIDS_X; x++) {

            int numberOfTreesInGrid = 0;
            const int PIXEL_X = x * NUMBER_OF_PIXELS_IN_TREE_GRID;
            const int PIXEL_Y = y * NUMBER_OF_PIXELS_IN_TREE_GRID;

            // Go through all pixels in the grid, and determine the number of trees in the grid.
            for (int index_pixel_grid = 0; index_pixel_grid < NUMBER_OF_PIXELS_IN_TREE_GRID; ++index_pixel_grid) {
                int dx = PIXEL_X + TREE_GRID_PIXEL_OFFSETS[index_pixel_grid][0];
                int dy = PIXEL_Y + TREE_GRID_PIXEL_OFFSETS[index_pixel_grid][1];

                int index = (dy * WILDFIRE_WIDTH + dx) * landscape_channels;

                int r = (int)landscape_image_data[index];
                int g = (int)landscape_image_data[index + 1];
                int b = (int)landscape_image_data[index + 2];

                const bool bIsTree1 = r == 32 && g == 99 && b == 84;
                const bool bIsTree2 = r == 66 && g == 143 && b == 30;
                const bool bIsTree3 = r == 185 && g == 209 && b == 50;

                if (bIsTree1 || bIsTree2 || bIsTree3) {
                    ++numberOfTreesInGrid;
                }
            }

            // Only place a tree if at least 9 of the 16 pixels are green
            if (numberOfTreesInGrid >= NUMBER_OF_TREES_IN_GRID_THRESHOLD) {
                treeModelMatrices[current_tree_instance_index] = glm::mat4(1.0f);

                constexpr float HEIGHT_SCALE = 768.f;
                constexpr float HEIGHT_DISPLACEMENT = 0.f;

                float heightValue = (heightmap_image_data[PIXEL_Y * heightmap_img_width + PIXEL_X] / 255.0f) * HEIGHT_SCALE;
                heightValue += HEIGHT_DISPLACEMENT;

                // Set the location.
                std::random_device rd;                           // Seed source
                std::mt19937 gen(rd());                          // Mersenne Twister RNG
                std::uniform_real_distribution<float> dist(-5.0f, 5.0f);  // Range [0.0, 1.0)

                glm::vec3 newLocation = glm::vec3(PIXEL_X - (WILDFIRE_WIDTH / 2.f) + dist(gen), heightValue, PIXEL_Y - (WILDFIRE_WIDTH / 2.f) + dist(gen));

                treeModelMatrices[current_tree_instance_index] = glm::translate(treeModelMatrices[current_tree_instance_index], newLocation);

                // Set the scale.
                constexpr float MESH_SCALE = 0.01f;
                treeModelMatrices[current_tree_instance_index] = glm::scale(treeModelMatrices[current_tree_instance_index], glm::vec3(MESH_SCALE));

                current_tree_instance_index++;
                totalTreeMeshCount++;
            }
        }
    }

    stbi_image_free(landscape_image_data);
    stbi_image_free(heightmap_image_data);

    unsigned int TREE_VBO;
    glGenBuffers(1, &TREE_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, TREE_VBO);
    glBufferData(GL_ARRAY_BUFFER, TreeModelInstanceCount * sizeof(glm::mat4), &treeModelMatrices[0], GL_STATIC_DRAW);

    for (unsigned int i = 0; i < treeModel.meshes.size(); i++)
    {
        unsigned int VAO = treeModel.meshes[i].VAO;
        glBindVertexArray(VAO);
        // vertex attributes
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

#pragma endregion FoliageSetUp

#pragma region RenderingLoop

    // Number of seconds since the last frame
    double lastFrameTime = 0;
    int frameCounter = 0;
    double lastFPSCheckTime = 0;

    ////////////////////////////////////////////////////////////////////
    /// RENDERING LOOP
    ////////////////////////////////////////////////////////////////////

    while (!glfwWindowShouldClose(mainWindow))
    {
        float currentTime = glfwGetTime();
        deltaTime = currentTime - lastFrameTime;

        //if (deltaTime >= fpsLimit) {
        {
            // Output the current frame rate.
            if (frameCounter >= (1.0 / MIN_FRAME_TIME_LIMIT)) {
                std::cout << "FPS: " << frameCounter / (currentTime - lastFPSCheckTime) << std::endl;
                frameCounter = 0;
                lastFPSCheckTime = currentTime;
            }
            else {
                ++frameCounter;
            }

            ////////////////////////////////////////////////////////////////////
            /// PROCESS INPUT
            ////////////////////////////////////////////////////////////////////
            
            processInput(mainWindow);

            ////////////////////////////////////////////////////////////////////
            /// RUN COMPUTE SHADER
            ////////////////////////////////////////////////////////////////////

            wildfireCompute.use();
            wildfireCompute.setInt("frameCounter", frameCounter);
            wildfireCompute.setFloat("iTime", currentTime);
            wildfireCompute.setBool("mouseDown", bIsMouseDown);
            wildfireCompute.setVec2("mousePos", mousePos);

            wildfireCompute.setBool("mouseDown", bIsMouseDown);

            if (bIsMouseDown == true) {
                bIsMouseDown = false;
                std::cout << mousePos.x << " " << mousePos.y << std::endl;
                wildfireCompute.setVec2("mousePos", mousePos);
            }

            glDispatchCompute(WILDFIRE_WIDTH, WILDFIRE_HEIGHT, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glCopyImageSubData(wildfireTextures[1], GL_TEXTURE_2D, 0, 0, 0, 0,
                wildfireTextures[0], GL_TEXTURE_2D, 0, 0, 0, 0,
                WILDFIRE_WIDTH, WILDFIRE_HEIGHT, 1);

            ////////////////////////////////////////////////////////////////////
            /// RENDER TERRAIN
            ////////////////////////////////////////////////////////////////////

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Calculate the current projection and camera view matrix.
            glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100000.0f);
            glm::mat4 cameraViewMatrix = camera.GetViewMatrix();

            // be sure to activate shader when setting uniforms/drawing objects
            terrainMeshShader.use();
            
            // World transformation
            glm::mat4 model = glm::mat4(1.0f);
            terrainMeshShader.setMat4("model", model);
            terrainMeshShader.setMat4("projection", cameraProjection);
            terrainMeshShader.setMat4("view", cameraViewMatrix);
            terrainMeshShader.setInt("landscapeTexture", LANDSCAPE_TEXTURE_INDEX);
            terrainMeshShader.setInt("wildfireTexture", WILDFIRE_TEXTURE_INDEX);
            terrainMeshShader.setInt("heightMap", HEIGHTMAP_TEXTURE_INDEX);

            glBindVertexArray(terrainVAO);
            glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS); // Make sure to set number of vertices per patch
            glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS * resolutionFactor * resolutionFactor); // Count depends on your patch size
            glBindVertexArray(0);

            ////////////////////////////////////////////////////////////////////
            /// RENDER TREES
            ////////////////////////////////////////////////////////////////////

            treeModelShader.use();
            treeModelShader.setMat4("projection", cameraProjection);
            treeModelShader.setMat4("view", cameraViewMatrix);

            // Go through all the meshes in the tree model, and draw the elements as instanced.
            for (unsigned int Index_TreeMesh = 0; Index_TreeMesh < treeModel.meshes.size(); Index_TreeMesh++)
            {
                glBindVertexArray(treeModel.meshes[Index_TreeMesh].VAO);
                glDrawElementsInstanced(GL_TRIANGLES, treeModel.meshes[Index_TreeMesh].indices.size(), GL_UNSIGNED_INT, 0, TreeModelInstanceCount);
            }

            ////////////////////////////////////////////////////////////////////
            /// SWAP GLFW BUFFERS AND CHECK POLL EVENTS
            ////////////////////////////////////////////////////////////////////

            glfwSwapBuffers(mainWindow);
            glfwPollEvents();

            lastFrameTime = currentTime;
        }
    }

#pragma endregion RenderingLoop

    ////////////////////////////////////////////////////////////////////
    /// TERMINATE GLFW
    ////////////////////////////////////////////////////////////////////
    
    glfwTerminate();
   
    return 0;
}

// Query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime * CAMERA_SPEED);
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime * CAMERA_SPEED);
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime * CAMERA_SPEED);
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime * CAMERA_SPEED);
    }  
}

// Whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever a key event occurs, this callback is called
// ---------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers)
{
    // Used to close the window if the user presses ESC.
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        default:
            break;
        }
    }
}

// Whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (bIsFirstMouseClick)
    {
        lastX = xpos;
        lastY = ypos;
        bIsFirstMouseClick = false;
    }

    float xoffset = xpos - lastX;

    // Reversed since y-coordinates go from bottom to top
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        bIsMouseDown = true;

        // Camera's forward direction in world space
        glm::vec3 cameraForward = camera.Front;

        // Camera's current position
        glm::vec3 rayOrigin = camera.Position;

        // Plane parameters (assuming flat terrain at y=0)
        glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
        float planeDistance = 0.0f;

        // Ray-plane intersection
        float denom = glm::dot(cameraForward, planeNormal);

        if (std::abs(denom) > 0.0001f) {
            float t = -(glm::dot(rayOrigin, planeNormal) + planeDistance) / denom;

            if (t >= 0) {
                // Calculate intersection point
                glm::vec3 intersectionPoint = rayOrigin + cameraForward * t;

                // Normalize coordinates based on terrain dimensions
                // Assuming terrain spans from -heightmapImageWidth/2 to +heightmapImageWidth/2
                // and from -heightmapImageHeight/2 to +heightmapImageHeight/2
                mousePos.x = (intersectionPoint.x + WILDFIRE_WIDTH / 2.0f) / WILDFIRE_WIDTH;
                mousePos.y = (intersectionPoint.z + WILDFIRE_HEIGHT / 2.0f) / WILDFIRE_HEIGHT;

                // Clamp to [0, 1] range
                mousePos.x = glm::clamp(mousePos.x, 0.0f, 1.0f);
                mousePos.y = glm::clamp(mousePos.y, 0.0f, 1.0f);
            }
        }
    } else {
        bIsMouseDown = false;
    }
}

// Whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}