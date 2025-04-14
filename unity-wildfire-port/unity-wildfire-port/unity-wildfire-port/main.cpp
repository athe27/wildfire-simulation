#define _CRT_SECURE_NO_WARNINGS

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
#include "Model.h"

#include <vector>

//#include "OpenGLMarkerObjects.h"
//#include "OpenGLCommon.h"
//#include "OpenGLWindow.h"
//#include "OpenGLViewer.h"
//#include "TinyObjLoader.h"


#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// wildfire macros

#define MATERIAL_GRASS 0.0f
#define MATERIAL_WATER 1.0f
#define MATERIAL_BEDROCK 2.0f
#define MATERIAL_TREE 3.0f

#define STATE_NOT_ON_FIRE 0.0f
#define STATE_ON_FIRE 1.0f
#define STATE_DESTROYED 2.0f

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifiers);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const unsigned int NUM_PATCH_PTS = 4;

// wildfire grid size
const unsigned int WILDFIRE_WIDTH = 2048;
const unsigned int WILDFIRE_HEIGHT = 2048;

int NUMBER_OF_TREES = 512 * 512;

// camera - give pretty starting point
Camera camera(glm::vec3(67.0f, 627.5f, 169.9f), glm::vec3(0.0f, 1.0f, 0.0f), -128.1f, -42.4f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int numberOfUpdatesUntilNextGridImageWrite = 0;
const unsigned int UPDATES_BETWEEN_GRID_IMAGE_WRITES = 30;

bool mouseDown = false;
glm::vec2 mousePos = glm::vec2(0.0f, 0.0f);

GLboolean generateWildfireTexture(GLsizei offset, GLuint* textures,
    GLsizei width, GLsizei height) {
    const char* heightmap_file_name = "HeightMaps/GreatLakeHeightmap.png";
    const char* landscape_file_name = "Landscapes/forested_landscape.png";

    // setup wildfire initial data
    const size_t pixelCount = width * height;
    float* data = new float[pixelCount * 4];

    // heightmap image
    int heightmap_img_width, heightmap_img_height, heightmap_channels;
    unsigned char* heightmap_image_data = stbi_load(heightmap_file_name, &heightmap_img_width, &heightmap_img_height, &heightmap_channels, STBI_grey);

    if (!heightmap_image_data) {
        std::cerr << "Failed to load heightmap image: " << heightmap_file_name << std::endl;
        return false;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Calculate the corresponding pixel coordinates in the image
            int img_x = (x * heightmap_img_width) / width;
            int img_y = (y * heightmap_img_height) / height;

            int index = (img_y * heightmap_img_width + img_x) * heightmap_channels;
            float heightValue = heightmap_image_data[img_y * width + img_x] / 255.0f;

            int data_index = (y * width + x) * 4;
            data[data_index + 2] = heightValue;
        }
    }

    stbi_image_free(heightmap_image_data);

    // landscape image
    int landscape_img_width, landscape_img_height, landscape_channels;
    unsigned char* landscape_image_data = stbi_load(landscape_file_name, &landscape_img_width, &landscape_img_height, &landscape_channels, STBI_rgb);

    if (!landscape_image_data) {
        std::cerr << "Failed to load landscape image: " << landscape_file_name << std::endl;
        return false;
    }


    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Calculate the corresponding pixel coordinates in the image
            int img_x = (x * landscape_img_width) / width;
            int img_y = (y * landscape_img_height) / height;

            int index = (img_y * landscape_img_width + img_x) * landscape_channels;

            int r = (int)landscape_image_data[index];     // Red channel
            int g = (int)landscape_image_data[index + 1]; // Green channel
            int b = (int)landscape_image_data[index + 2]; // Blue channel

            int data_index = (y * width + x) * 4;
            if (r == 181 && g == 219 && b == 235) {
                data[data_index + 0] = MATERIAL_WATER;
            }
            else if (r == 207 && g == 198 && b == 180) {
                data[data_index + 0] = MATERIAL_BEDROCK;
            }
            else if (r == 121 && g == 150 && b == 114) {
                data[data_index + 0] = MATERIAL_GRASS;
            }
            else if (r == 34 && g == 87 && b == 22) {
                data[data_index + 0] = MATERIAL_TREE;
            }

            data[data_index + 1] = STATE_NOT_ON_FIRE;

            /*if (y > 1000 && y < 1100 && x > 1000 && x < 1100) {
                data[data_index + 1] = STATE_ON_FIRE;
            }*/
        }
    }

    stbi_image_free(landscape_image_data);

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

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
            width, height,
            0, GL_RGBA, GL_FLOAT, data);

        if (i == 0) {
            glBindImageTexture(finalOffset, textures[i], 0, GL_FALSE, 0,
                GL_READ_ONLY, GL_RGBA32F);
        }
        else {
            glBindImageTexture(finalOffset, textures[i], 0, GL_FALSE, 0,
                GL_WRITE_ONLY, GL_RGBA32F);
        }

    }

    // Check for any errors during the process
    GLenum err = glGetError();
    return (err == GL_NO_ERROR);
}

int main()
{

#pragma region Initialize

    // GLFW: Initialize and Configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a new window using GLFW
    // --------------------
    GLFWwindow* mainGLWindow = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wildfire Simulation", NULL, NULL);

    // If we failed to create a GL window, then just return.
    if (mainGLWindow == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Ensure that this new window is our main window and can consume input correctly.
    glfwMakeContextCurrent(mainGLWindow);
    glfwSetFramebufferSizeCallback(mainGLWindow, framebuffer_size_callback);
    glfwSetMouseButtonCallback(mainGLWindow, mouse_button_callback);

    // Set up our input call backs.
    glfwSetKeyCallback(mainGLWindow, key_callback);
    glfwSetCursorPosCallback(mainGLWindow, mouse_callback);
    glfwSetScrollCallback(mainGLWindow, scroll_callback);

    // Tell GLFW to capture our mouse.
    glfwSetInputMode(mainGLWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#pragma endregion Initialize

    // Build and compile our shader program using our vertex, fragment, tessellation control, and tessellation evaluatioin shaders.
    // ----------------------------------------------------------------------------------------------------------------------------
    Shader heightMapShader("gpuheight.vs", "gpuheight.frag", nullptr, "gpuheight.tcs", "gpuheight.tes");
    Shader treeModelShader("model_loading.vs", "model_loading.frag");

    Model treeModel("Meshes/tree.obj");

#pragma region LoadingHeightMapTexture

    // Load and create the heightmap texture.
    // --------------------------------------
    GLuint heightMapTexture;
    glGenTextures(1, &heightMapTexture);
    glActiveTexture(GL_TEXTURE6);

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
    unsigned char* heightMapData = stbi_load("HeightMaps/GreatLakeHeightmap.png", &heightmapImageWidth, &heightmapImageHeight, &heightmapImageChannelCount, STBI_rgb_alpha);

    // If we failed to read from the heightmap data, then just return.
    if (heightMapData == nullptr) {
        std::cout << "Failed to load heightmap texture" << std::endl;
        return -1;
    }

    // Generate the OpenGL texture from the heightmap image.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, heightmapImageWidth, heightmapImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, heightMapData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set the heightmap shader (which is shader 0) as a parameter for the shader program.
    heightMapShader.setInt("heightMap", 6);

    // Output the size of the heightmap.
    std::cout << "Loaded heightmap of size " << heightmapImageHeight << " x " << heightmapImageWidth << std::endl;

    stbi_image_free(heightMapData);

#pragma endregion LoadingHeightMapTexture

#pragma region LoadingLandscapeTexture

    // Load and create the landscape texture for the fragment shader.
    // --------------------------------------------------------------
    GLuint landscapeTexture;
    glGenTextures(1, &landscapeTexture);
    glActiveTexture(GL_TEXTURE1);

    // All upcoming GL_TEXTURE_2D operations now have effect on the landscapeTexture object.
    glBindTexture(GL_TEXTURE_2D, landscapeTexture);

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the PNG file (use stb_image or another image loader)
    int landscapeImageWidth, landscapeImageHeight, landscapeImageChannelCount;

    // Note that we do not read from RGBA in this load.
    unsigned char* landscapeData = stbi_load("Landscapes/forested_landscape.png", &landscapeImageWidth, &landscapeImageHeight, &landscapeImageChannelCount, 0);

    // If we failed to read from the landscape data, then just return.
    if (landscapeData == nullptr) {
        std::cout << "Failed to load landscape texture" << std::endl;
        return -1;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, landscapeImageWidth, landscapeImageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, landscapeData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Output the size of the heightmap.
    std::cout << "Loaded landscape texture of size " << landscapeImageHeight << " x " << landscapeImageWidth << std::endl;

    // Make sure to free the data when all is said and done.
    stbi_image_free(landscapeData);

#pragma endregion LoadingLandscapeTexture

    // COMPUTE SHADER STUFF (Anthony)
    ComputeShader wildfireCompute("wildfireCompute.cs");

    const GLsizei numWildfireTextures = 2;
    GLuint wildfireTextures[numWildfireTextures];
    glGenTextures(numWildfireTextures, wildfireTextures);

    generateWildfireTexture(2, wildfireTextures, WILDFIRE_WIDTH, WILDFIRE_HEIGHT);

    const double fpsLimit = 1.0 / 60.0;
    double lastFrameTime = 0;   // number of seconds since the last frame
    int frameCounter = 0;
    double lastFPSCheckTime = 0;

#pragma region GenerateVertices

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> vertices;

    // This affects the number of vertices that will be created.
    unsigned int resolutionFactor = 20;

    std::cout << "image width " << heightmapImageWidth << std::endl;
    for (unsigned i = 0; i <= resolutionFactor - 1; i++)
    {
        for (unsigned j = 0; j <= resolutionFactor - 1; j++)
        {
            vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * i / (float)resolutionFactor); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * j / (float)resolutionFactor); // v.z
            vertices.push_back(i / (float)resolutionFactor); // u
            vertices.push_back(j / (float)resolutionFactor); // v

            vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * (i + 1) / (float)resolutionFactor); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * j / (float)resolutionFactor); // v.z
            vertices.push_back((i + 1) / (float)resolutionFactor); // u
            vertices.push_back(j / (float)resolutionFactor); // v

            vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * i / (float)resolutionFactor); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * (j + 1) / (float)resolutionFactor); // v.z
            vertices.push_back(i / (float)resolutionFactor); // u
            vertices.push_back((j + 1) / (float)resolutionFactor); // v

            vertices.push_back(-heightmapImageWidth / 2.0f + heightmapImageWidth * (i + 1) / (float)resolutionFactor); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-heightmapImageHeight / 2.0f + heightmapImageHeight * (j + 1) / (float)resolutionFactor); // v.z
            vertices.push_back((i + 1) / (float)resolutionFactor); // u
            vertices.push_back((j + 1) / (float)resolutionFactor); // v
        }
    }

    std::cout << "Loaded " << resolutionFactor * resolutionFactor << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << resolutionFactor * resolutionFactor * 4 << " vertices in vertex shader" << std::endl;

#pragma endregion GenerateVertices

#pragma region TerrainSetUp

    // First, configure the cube's VAO (and terrainVBO)
    unsigned int terrainVAO, terrainVBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);

    glBindVertexArray(terrainVAO);

    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

#pragma endregion TerrainSetUp

#pragma region FoliageSetUp

    const size_t pixelCount = WILDFIRE_WIDTH * WILDFIRE_HEIGHT;

    /////////////////////////////////////////////////////
    /// Load Landscape Image from File Name
    /////////////////////////////////////////////////////

    const char* landscape_file_name = "Landscapes/forested_landscape.png";
    int landscape_img_width, landscape_img_height, landscape_channels;
    unsigned char* landscape_image_data = stbi_load(landscape_file_name, &landscape_img_width, &landscape_img_height, &landscape_channels, STBI_rgb);

    if (!landscape_image_data) {
        std::cerr << "Failed to load landscape image: " << landscape_file_name << std::endl;
        return false;
    }

    /////////////////////////////////////////////////////
    /// Load Heightmap Image from File Name
    /////////////////////////////////////////////////////

    const char* heightmap_file_name = "HeightMaps/GreatLakeHeightmap.png";
    int heightmap_img_width, heightmap_img_height, heightmap_channels;
    unsigned char* heightmap_image_data = stbi_load(heightmap_file_name, &heightmap_img_width, &heightmap_img_height, &heightmap_channels, STBI_grey);

    if (!heightmap_image_data) {
        std::cerr << "Failed to load heightmap image: " << heightmap_file_name << std::endl;
        return false;
    }

    /////////////////////////////////////////////////////
    /// Count Total Number of Trees in Image
    /////////////////////////////////////////////////////

    int totalNumberOfTrees = 0;

    for (int y = 0; y < WILDFIRE_HEIGHT; y++) {
        for (int x = 0; x < WILDFIRE_HEIGHT; x++) {
            int index = (y * WILDFIRE_WIDTH + x) * landscape_channels;

            int r = (int)landscape_image_data[index];
            int g = (int)landscape_image_data[index + 1];
            int b = (int)landscape_image_data[index + 2];

            if (r == 34 && g == 87 && b == 22) {
                ++totalNumberOfTrees;
            }
        }
    }

    /////////////////////////////////////////////////////
    /// Generate Grids from Landscape Texture
    /////////////////////////////////////////////////////

    constexpr int GRID_DIMENSION_X = 4;
    constexpr int GRID_DIMENSION_Y = 4;
    constexpr int NUMBER_OF_CELLS_IN_GRID = GRID_DIMENSION_X * GRID_DIMENSION_Y;
    constexpr int NUMBER_OF_TREES_IN_GRID_THRESHOLD = 9;

    int tree_model_index = 0;

    // Offsets for the 2x2 block
    const int all_grid_pixel_offsets[NUMBER_OF_CELLS_IN_GRID][2] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0},
        {0, 1}, {1, 1}, {2, 1}, {3, 1},
        {0, 2}, {1, 2}, {2, 2}, {3, 2},
        {0, 3}, {1, 3}, {2, 3}, {3, 3},
    };

    const int NUMBER_OF_GRIDS_X = WILDFIRE_HEIGHT / NUMBER_OF_CELLS_IN_GRID;
    const int NUMBER_OF_GRIDS_Y = WILDFIRE_WIDTH / NUMBER_OF_CELLS_IN_GRID;

    /////////////////////////////////////////////////////
    /// Initialize Tree Model Matrices
    /////////////////////////////////////////////////////

    // Determine the maximum number of trees possible based from the maximum number of grids in each direction.
    NUMBER_OF_TREES = NUMBER_OF_GRIDS_X * NUMBER_OF_GRIDS_Y;

    glm::mat4* treeModelMatrices = new glm::mat4[NUMBER_OF_TREES];
    for (int index_tree = 0; index_tree < NUMBER_OF_TREES; index_tree++) {
        glm::mat4 tree_model = glm::mat4(1.0f);
        tree_model = glm::scale(tree_model, glm::vec3(0.f));
        treeModelMatrices[index_tree] = tree_model;
    }

    int totalTreeMeshCount = 0;

    for (int y = 0; y < NUMBER_OF_GRIDS_Y; y++) {
        for (int x = 0; x < NUMBER_OF_GRIDS_X; x++) {

            int numberOfTreesInGrid = 0;
            const int PIXEL_X = x * NUMBER_OF_CELLS_IN_GRID;
            const int PIXEL_Y = y * NUMBER_OF_CELLS_IN_GRID;

            // Go through all pixels in the grid, and determine the number of trees in the grid.
            for (int index_pixel_grid = 0; index_pixel_grid < NUMBER_OF_CELLS_IN_GRID; ++index_pixel_grid) {
                int dx = PIXEL_X + all_grid_pixel_offsets[index_pixel_grid][0];
                int dy = PIXEL_Y + all_grid_pixel_offsets[index_pixel_grid][1];

                int index = (dy * WILDFIRE_WIDTH + dx) * landscape_channels;

                int r = (int)landscape_image_data[index];
                int g = (int)landscape_image_data[index + 1];
                int b = (int)landscape_image_data[index + 2];

                if (r == 34 && g == 87 && b == 22) {
                    ++numberOfTreesInGrid;
                }
            }

            // Only place a tree if at least 9 of the 16 pixels are green
            if (numberOfTreesInGrid >= NUMBER_OF_TREES_IN_GRID_THRESHOLD) {
                treeModelMatrices[tree_model_index] = glm::mat4(1.0f);

                constexpr float HEIGHT_SCALE = 768.f;
                constexpr float HEIGHT_DISPLACEMENT = 0.f;

                float heightValue = (heightmap_image_data[PIXEL_Y * heightmap_img_width + PIXEL_X] / 255.0f) * HEIGHT_SCALE;
                heightValue += HEIGHT_DISPLACEMENT;

                // Set the location.
                glm::vec3 newLocation = glm::vec3(PIXEL_X - (WILDFIRE_WIDTH / 2.f), heightValue, PIXEL_Y - (WILDFIRE_WIDTH / 2.f));

                treeModelMatrices[tree_model_index] = glm::translate(treeModelMatrices[tree_model_index], newLocation);

                // Set the scale.
                constexpr float MESH_SCALE = 0.01f;
                treeModelMatrices[tree_model_index] = glm::scale(treeModelMatrices[tree_model_index], glm::vec3(MESH_SCALE));

                tree_model_index++;
                totalTreeMeshCount++;
            }
        }
    }

    stbi_image_free(landscape_image_data);

    std::cout << "Initial Tree Model Count is " << totalTreeMeshCount << std::endl;

    // vertex buffer object
    unsigned int TREE_VBO;
    glGenBuffers(1, &TREE_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, TREE_VBO);
    glBufferData(GL_ARRAY_BUFFER, NUMBER_OF_TREES * sizeof(glm::mat4), &treeModelMatrices[0], GL_STATIC_DRAW);

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

    // render loop
    // -----------
    while (!glfwWindowShouldClose(mainGLWindow))
    {
        // per-frame time logic
        // --------------------
        float now = glfwGetTime();
        deltaTime = now - lastFrameTime;

        //if (deltaTime >= fpsLimit) {
        {
            if (frameCounter >= 60) {
                std::cout << "FPS: " << frameCounter / (now - lastFPSCheckTime) << std::endl;
                frameCounter = 0;
                lastFPSCheckTime = now;
            }
            else {
                ++frameCounter;
            }

            // input
            // -----
            processInput(mainGLWindow);

            // compute
            // _____

            wildfireCompute.use();
            wildfireCompute.setInt("frameCounter", frameCounter);
            wildfireCompute.setFloat("iTime", now);
            wildfireCompute.setBool("mouseDown", mouseDown);
            wildfireCompute.setVec2("mousePos", mousePos);

            wildfireCompute.setBool("mouseDown", mouseDown);
            if (mouseDown == true) {
                mouseDown = false;
                std::cout << mousePos.x << " " << mousePos.y << std::endl;
                wildfireCompute.setVec2("mousePos", mousePos);
            }

            glDispatchCompute(WILDFIRE_WIDTH, WILDFIRE_HEIGHT, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glCopyImageSubData(wildfireTextures[1], GL_TEXTURE_2D, 0, 0, 0, 0,
                wildfireTextures[0], GL_TEXTURE_2D, 0, 0, 0, 0,
                WILDFIRE_WIDTH, WILDFIRE_HEIGHT, 1);

            // render
            // ------
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Calculate the current projection and camera view matrix.
            glm::mat4 cameraProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
            glm::mat4 cameraViewMatrix = camera.GetViewMatrix();

            // be sure to activate shader when setting uniforms/drawing objects
            heightMapShader.use();
            
            heightMapShader.setMat4("projection", cameraProjection);
            heightMapShader.setMat4("view", cameraViewMatrix);
            heightMapShader.setInt("heightMap", 6);
            heightMapShader.setInt("landscapeTexture", 1);
            heightMapShader.setInt("wildfireTexture", 2);

            // World transformation
            glm::mat4 model = glm::mat4(1.0f);
            heightMapShader.setMat4("model", model);

            // Render the terrain
            glBindVertexArray(terrainVAO);
            glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS); // Make sure to set number of vertices per patch
            glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS * resolutionFactor * resolutionFactor); // Count depends on your patch size
            glBindVertexArray(0);

            ////// RENDER THE TREES
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#pragma region DefaultRendering

            ////// RENDER THE TREES
            //treeModelShader.use();
            //treeModelShader.setMat4("projection", cameraProjection);
            //treeModelShader.setMat4("view", cameraViewMatrix);

            //for (int x = 0; x < 512; x++) {
            //    for (int y = 0; y < 512; y++) {
            //        // render the loaded model
            //        glm::mat4 tree_model = glm::mat4(1.0f);
            //        tree_model = glm::translate(tree_model, glm::vec3(x * 40.f, 75.f, y * 40.f)); // translate it down so it's at the center of the scene
            //        tree_model = glm::scale(tree_model, glm::vec3(0.02f));

            //        treeModelShader.setMat4("model", tree_model);

            //        treeModel.Draw(treeModelShader);
            //    }
            //}

#pragma endregion

#pragma region InstanceRendering

            treeModelShader.use();
            treeModelShader.setMat4("projection", cameraProjection);
            treeModelShader.setMat4("view", cameraViewMatrix);

            // Go through all the meshes in the tree model, and draw the elements as instanced.
            for (unsigned int Index_TreeMesh = 0; Index_TreeMesh < treeModel.meshes.size(); Index_TreeMesh++)
            {
                glBindVertexArray(treeModel.meshes[Index_TreeMesh].VAO);
                glDrawElementsInstanced(GL_TRIANGLES, treeModel.meshes[Index_TreeMesh].indices.size(), GL_UNSIGNED_INT, 0, NUMBER_OF_TREES);
            }

#pragma endregion InstanceRendering

            // GLFW: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(mainGLWindow);
            glfwPollEvents();

            lastFrameTime = now;
        }
    }

#pragma endregion RenderingLoop

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    // Change this speed to affect how fast you want the camera to zip around the terrain.
    constexpr float CAMERA_SPEED = 1000.f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * CAMERA_SPEED);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * CAMERA_SPEED);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * CAMERA_SPEED);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime * CAMERA_SPEED);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
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

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mouseDown = true;

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

                std::cout << "Normalized Mouse Pos: (" << mousePos.x << ", " << mousePos.y << ")" << std::endl;
            }
        }
    }
    else {
        mouseDown = false;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}