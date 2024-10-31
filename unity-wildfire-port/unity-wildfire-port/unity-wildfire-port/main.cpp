#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

#include <iostream>

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

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

bool mouseDown = false;
glm::vec2 mousePos = glm::vec2(0.0f, 0.0f);

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

		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
			width, height, depth,
			0, GL_RGBA, GL_FLOAT, NULL);

		glBindImageTexture(i, textures[i], 0, GL_TRUE, 0,
			GL_READ_WRITE, GL_RGBA32F);
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

	computeShader.use();
	computeShader.setVec3("m_size", glm::vec3(WIDTH, HEIGHT, DEPTH));
	computeShader.setFloat("dt", 0.1f);
	computeShader.setInt("m_iterations", 10);
	computeShader.setFloat("m_vorticityStrength", 5.0f);
	computeShader.setFloat("m_densityAmount", 1.0f);
	computeShader.setFloat("m_densityDissipation", 0.99f);
	computeShader.setFloat("m_densityBuoyancy", 1.0f);
	computeShader.setFloat("m_densityWeight", 0.0125f);
	computeShader.setFloat("m_temperatureAmount", 10.0f);
	computeShader.setFloat("m_temperatureDissipation", 0.995f);
	computeShader.setFloat("m_reactionAmount", 1.0f);
	computeShader.setFloat("m_reactionDecay", 0.003f);
	computeShader.setFloat("m_reactionExtinguishment", 0.01f);
	computeShader.setFloat("m_velocityDissipation", 0.995f);
	computeShader.setFloat("m_inputRadius", 0.04f);
	computeShader.setFloat("m_ambientTemperature", 0.0f);
	computeShader.setVec3("m_inputPos", glm::vec3(0.5f, 0.1f, 0.5f));

	// Create texture for opengl operation
	// -----------------------------------
	GLuint textures[3];

	generateMultipleTextures(3, textures, WIDTH, HEIGHT, DEPTH);

	const double fpsLimit = 1.0 / 60.0;
	double lastUpdateTime = 0;  // number of seconds since the last loop
	double lastFrameTime = 0;   // number of seconds since the last frame
	int frameCounter = 0;
	double lastFPSCheckTime = 0;

	// This while loop repeats as fast as possible
	while (!glfwWindowShouldClose(window))
	{
		double now = glfwGetTime();
		double deltaTime = now - lastUpdateTime;

		glfwPollEvents();

		screenQuad.use();
		screenQuad.setBool("mouseDown", mouseDown);
		if (mouseDown == true) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			mousePos.x = xpos;
			mousePos.y = ypos;
			screenQuad.setVec2("mousePos", mousePos);
		}
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
			// draw your frame here
			computeShader.use();
			computeShader.setFloat("iTime", now);

			glDispatchCompute(WIDTH, HEIGHT, DEPTH);

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

			renderQuad();

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

	glfwTerminate();

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
	}
	else {
		mouseDown = false;
	}
}

