#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

#include <iostream>

#include "WildFireSimulation.h"
#include "FireSimulation.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// texture size
const unsigned int WIDTH = 64;
const unsigned int DEPTH = 64;
const unsigned int HEIGHT = 128;

static constexpr bool USE_CPU_IMPLEMENTATION = true;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

int numberOfUpdatesUntilNextGridImageWrite = 0;
const unsigned int UPDATES_BETWEEN_GRID_IMAGE_WRITES = 30;

bool mouseDown = false;
glm::vec2 mousePos = glm::vec2(0.0f, 0.0f);
glm::vec2 gustPosition = glm::vec2(0.f, 0.f);

GLboolean generateMultipleTextures(GLsizei count, GLuint* textures,
	GLsizei width, GLsizei height, GLsizei depth) {
	// Generate all textures at once
	glGenTextures(count, textures);

	// Set up each texture
	for (GLsizei i = 0; i < count; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_3D, textures[i]);

		// Set common parameters
		const GLint wrap = GL_CLAMP_TO_BORDER;
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, NULL);

		glBindImageTexture(i, textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	}

	// Check for any errors during the process
	GLenum err = glGetError();
	return (err == GL_NO_ERROR);
}

int main(int argc, char* argv[])
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSwapInterval(0);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// query limitations
	// -----------------
	int max_compute_work_group_count[3];
	int max_compute_work_group_size[3];
	int max_compute_work_group_invocations;

	for (int idx = 0; idx < 3; idx++) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
	}
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);

	std::cout << "OpenGL Limitations: " << std::endl;
	std::cout << "maximum number of work groups in X dimension " << max_compute_work_group_count[0] << std::endl;
	std::cout << "maximum number of work groups in Y dimension " << max_compute_work_group_count[1] << std::endl;
	std::cout << "maximum number of work groups in Z dimension " << max_compute_work_group_count[2] << std::endl;

	std::cout << "maximum size of a work group in X dimension " << max_compute_work_group_size[0] << std::endl;
	std::cout << "maximum size of a work group in Y dimension " << max_compute_work_group_size[1] << std::endl;
	std::cout << "maximum size of a work group in Z dimension " << max_compute_work_group_size[2] << std::endl;

	std::cout << "Number of invocations in a single local work group that may be dispatched to a compute shader " << max_compute_work_group_invocations << std::endl;

	// build and compile shaders
	// -------------------------
	Shader screenQuad("screenQuad.vs", "screenQuad.fs");
	ComputeShader computeShader("computeShader.cs");

	screenQuad.use();
	screenQuad.setInt("velocityDensityTexture", 0);
	screenQuad.setInt("pressureTempPhiReactionTexture", 1);
	screenQuad.setInt("curlObstaclesTexture", 2);
	screenQuad.setVec3("m_size", glm::vec3(WIDTH, HEIGHT, DEPTH));
	screenQuad.setBool("mouseDown", mouseDown);
	screenQuad.setVec2("mousePos", mousePos);

	float dt = 0.1f;
	int m_iterations = 10;
	float m_vorticityStrength = 5.0f;
	float m_densityAmount = 1.0f;
	float m_densityDissipation = 0.99f;
	float m_densityBuoyancy = 1.0f;
	float m_densityWeight = 0.0125f;
	float m_temperatureAmount = 10.0f;
	float m_temperatureDissipation = 0.995f;
	float m_reactionAmount = 1.0f;
	float m_reactionDecay = 0.003f;
	float m_reactionExtinguishment = 0.01f;
	float m_velocityDissipation = 0.995f;
	float m_inputRadius = 0.04f;
	float m_ambientTemperature = 0.0f;
	float m_inputPos[3];
	m_inputPos[0] = 0.5;
	m_inputPos[1] = 0.1;
	m_inputPos[2] = 0.5;

	float m_windNoiseFrequency = 0.1;
	float m_windSpeedFrequency = 0.2f;

	computeShader.use();
	computeShader.setVec3("m_size", glm::vec3(WIDTH, HEIGHT, DEPTH));
	computeShader.setFloat("dt", dt);
	computeShader.setInt("m_iterations", m_iterations);
	computeShader.setFloat("m_vorticityStrength", m_vorticityStrength);
	computeShader.setFloat("m_densityAmount", m_densityAmount);
	computeShader.setFloat("m_densityDissipation", m_densityDissipation);
	computeShader.setFloat("m_densityBuoyancy", m_densityBuoyancy);
	computeShader.setFloat("m_densityWeight", m_densityWeight);
	computeShader.setFloat("m_temperatureAmount", m_temperatureAmount);
	computeShader.setFloat("m_temperatureDissipation", m_temperatureDissipation);
	computeShader.setFloat("m_reactionAmount", m_reactionAmount);
	computeShader.setFloat("m_reactionDecay", m_reactionDecay);
	computeShader.setFloat("m_reactionExtinguishment", m_reactionExtinguishment);
	computeShader.setFloat("m_velocityDissipation", m_velocityDissipation);
	computeShader.setFloat("m_inputRadius", m_inputRadius);
	computeShader.setFloat("m_ambientTemperature", m_ambientTemperature);
	computeShader.setVec3("m_inputPos", glm::vec3(m_inputPos[0], m_inputPos[1], m_inputPos[2]));
	computeShader.setFloat("m_windNoiseFrequency", m_windNoiseFrequency);
	computeShader.setFloat("m_windSpeedFrequency", m_windSpeedFrequency);

	// Create texture for opengl operation
	// -----------------------------------
	GLuint textures[3];
	generateMultipleTextures(3, textures, WIDTH, HEIGHT, DEPTH);

	const double fpsLimit = 1.0 / 60.0;
	double lastUpdateTime = 0;  // number of seconds since the last loop
	double lastFrameTime = 0;   // number of seconds since the last frame
	int frameCounter = 0;
	double lastFPSCheckTime = 0;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	WildFireSimulation* wildFireSimulation = new WildFireSimulation();
	wildFireSimulation->InitializeWildFireFimulation();

	FireSimulation* fireSimulation = new FireSimulation();

	// This while loop repeats as fast as possible
	while (!glfwWindowShouldClose(window))
	{
		double now = glfwGetTime();
		double deltaTime = now - lastUpdateTime;

		glfwPollEvents();

		// update your application logic here,
		// using deltaTime if necessary (for physics, tweening, etc.)

		// This if-statement only executes once every 60th of a second
		if ((now - lastFrameTime) >= fpsLimit)
		{
			if (frameCounter >= 60) {
				std::cout << "FPS: " << frameCounter / (now - lastFPSCheckTime) << std::endl;
				frameCounter = 0;
				lastFPSCheckTime = now;
			}
			else {
				++frameCounter;
			}


			if ((++numberOfUpdatesUntilNextGridImageWrite) >= UPDATES_BETWEEN_GRID_IMAGE_WRITES) {
				wildFireSimulation->UpdateWildFireSimulation(1.f / 60.f);
				wildFireSimulation->WriteGridResultsToImage();
				numberOfUpdatesUntilNextGridImageWrite = 0;
			}

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Config");
			ImGui::InputFloat("dt", &dt);
			ImGui::InputInt("iterations", &m_iterations);
			ImGui::InputFloat("vorticity strength", &m_vorticityStrength);
			ImGui::InputFloat("density amount", &m_densityAmount);
			ImGui::InputFloat("density dissipation", &m_densityDissipation);
			ImGui::InputFloat("density buoyancy", &m_densityBuoyancy);
			ImGui::InputFloat("density weight", &m_densityWeight);
			ImGui::InputFloat("temperature amount", &m_temperatureAmount);
			ImGui::InputFloat("temperature dissipation", &m_temperatureDissipation);
			ImGui::InputFloat("reaction amount", &m_reactionAmount);
			ImGui::InputFloat("reaction decay", &m_reactionDecay);
			ImGui::InputFloat("reaction extinguishment", &m_reactionExtinguishment);
			ImGui::InputFloat("velocity dissipation", &m_velocityDissipation);
			ImGui::InputFloat("input radius", &m_inputRadius);
			ImGui::InputFloat("ambient temperature", &m_ambientTemperature);
			ImGui::InputFloat3("input position", m_inputPos);
			ImGui::InputFloat("wind noise frequency", &m_windNoiseFrequency);
			ImGui::InputFloat("wind speed frequency", &m_windSpeedFrequency);
			ImGui::End();

			if (USE_CPU_IMPLEMENTATION) {
				fireSimulation->RunFireSimulation(dt, textures);
			} else {
				// draw your frame here
				computeShader.use();
				computeShader.setFloat("iTime", now);
				// set configs
				computeShader.setFloat("dt", dt);
				computeShader.setInt("m_iterations", m_iterations);
				computeShader.setFloat("m_vorticityStrength", m_vorticityStrength);
				computeShader.setFloat("m_densityAmount", m_densityAmount);
				computeShader.setFloat("m_densityDissipation", m_densityDissipation);
				computeShader.setFloat("m_densityBuoyancy", m_densityBuoyancy);
				computeShader.setFloat("m_densityWeight", m_densityWeight);
				computeShader.setFloat("m_temperatureAmount", m_temperatureAmount);
				computeShader.setFloat("m_temperatureDissipation", m_temperatureDissipation);
				computeShader.setFloat("m_reactionAmount", m_reactionAmount);
				computeShader.setFloat("m_reactionDecay", m_reactionDecay);
				computeShader.setFloat("m_reactionExtinguishment", m_reactionExtinguishment);
				computeShader.setFloat("m_velocityDissipation", m_velocityDissipation);
				computeShader.setFloat("m_inputRadius", m_inputRadius);
				computeShader.setFloat("m_ambientTemperature", m_ambientTemperature);
				computeShader.setVec3("m_inputPos", glm::vec3(m_inputPos[0], m_inputPos[1], m_inputPos[2]));
				computeShader.setBool("gustActive", mouseDown);
				computeShader.setVec2("gustPosition", gustPosition);
				computeShader.setFloat("gustStrength", m_inputPos[0]);
				computeShader.setFloat("m_windNoiseFrequency", m_windNoiseFrequency);
				computeShader.setFloat("m_windSpeedFrequency", m_windSpeedFrequency);

				glDispatchCompute(WIDTH, HEIGHT, DEPTH);

				
			}

			// make sure writing to image has finished before read
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			// render image to quad
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			screenQuad.use();

			//set shadertoy params
			screenQuad.setFloat("iTime", now);

			int width, height;
			glfwGetWindowSize(window, &width, &height);
			screenQuad.setVec3("iResolution", glm::vec3(width, height, width / (float)height));

			// set mouse stuff
			ImGui::Begin("Config");
			bool isConfigFocused = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
			ImGui::End();
			bool mouseDownReal = mouseDown && isConfigFocused == false;
			screenQuad.setBool("mouseDown", mouseDownReal);
			if (mouseDownReal == true) {
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				mousePos.x = xpos;
				mousePos.y = ypos;
				screenQuad.setVec2("mousePos", mousePos);
			}

			renderQuad();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);

			// only set lastFrameTime when you actually draw something
			lastFrameTime = now;
		}

		// set lastUpdateTime every iteration
		lastUpdateTime = now;
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteTextures(2, textures);
	glDeleteProgram(screenQuad.ID);
	glDeleteProgram(computeShader.ID);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();

	delete fireSimulation;
	delete wildFireSimulation;

	return EXIT_SUCCESS;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouseDown = true;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Convert screen coordinates to simulation coordinates
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		gustPosition = glm::vec2(
			(xpos / width) * 2.0f - 1.0f, // Normalize to [-1, 1] for X
			1.0f - (ypos / height) * 2.0f  // Normalize to [-1, 1] for Y
		);
	}
	else {
		mouseDown = false;
	}
}

