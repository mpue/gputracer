#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Shader.h"
#include "Camera.h"
#include "ComputeShader.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void renderQuad();
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

using namespace glm;

Camera camera;
vec3 campos;

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1200;

float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool navigate_mouse = false;

// texture size
const unsigned int TEXTURE_WIDTH = 1920, TEXTURE_HEIGHT = 1200;


struct Sphere {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	glm::vec3 normal;
};

struct Light {
	glm::vec3 position;
	glm::vec3 color;
};

struct FragUBO
{
	mat4 invProjectionView;
	float near;
	float far;
}
ubo;

const double PI = 3.14159265358979323846;

float specStrength = 1.0f;
float exponent = 64.0;

double generateSineWave(double frequency, double amplitude, double time) {
	return amplitude * std::sin(2 * PI * frequency * time);
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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "gputracer", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSwapInterval(0);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);


	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");


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
	Shader screenQuad("vertex.vert", "fragment.frag");
	ComputeShader computeShader("compute.comp");

	screenQuad.use();
	screenQuad.setInt("tex", 0);

	// Create texture for opengl operation
	// -----------------------------------
	unsigned int texture;

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	Light* lights = new Light[1];
	lights[0] = Light();
	lights[0].color = glm::vec3(0.5, 0.5, 0.5);
	lights[0].position = glm::vec3(-10,10,-10);

	Sphere* spheres = new Sphere[100];

	int i = 0;

	for (int y = 0; y < 10; y++) {
		for (int x = 0; x < 10;x++) {
			spheres[i] = Sphere();
			spheres[i].color = glm::vec3(0.1*x, 0.1 * y, 0);
			spheres[i].radius = 1.0f;
			spheres[i].position = glm::vec3(2 * x, 0, 2 * y);
			i++;
		}
	}
	

	//GLuint lightBuffer;
	//glGenBuffers(1, &lightBuffer);
	//glBindBuffer(GL_UNIFORM_BUFFER, lightBuffer);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(Sphere) * 1, spheres, GL_DYNAMIC_DRAW);
	//glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightBuffer);

	GLuint _ubo;
	glGenBuffers(1, &_ubo);

	campos = vec3(0.0f, 0, 2);


	camera = Camera(campos, vec3(0, 1, 0));

	// render loop
	// -----------
	int fCounter = 0;

	float time = 0;

	while (!glfwWindowShouldClose(window))
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Set frame time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		if (fCounter > 500) {
			std::cout << "FPS: " << 1 / deltaTime << "\r";
			fCounter = 0;
		}
		else {
			fCounter++;
		}

		computeShader.use();
		vec3 firstPersonDirection = glm::vec3(0, 0, 1);
		
		vec3 firstPersonUp = glm::vec3(0, 1, 0);
		// mat4 view = lookAt(campos, campos + firstPersonDirection, firstPersonUp);

		mat4 view = camera.GetViewMatrix();

		// aprox. 103 degrees FoV horizontal like Overwatch
		float fovVertical = 1.24f;

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		float near = .1f, far = 300.0f;
		mat4 projection = perspective(fovVertical, aspectRatio, near, far);

		// Unproject camera for world space ray
		mat4 inversinvProjectionView = inverse(projection * view);
		// fill the buffer, you do this anytime any of the lights change

		ubo.far = far;
		ubo.near = near;
		ubo.invProjectionView = inversinvProjectionView;

		glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(FragUBO), (void*)&ubo, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, _ubo);
		computeShader.setInt("numLights", 1);
		computeShader.setInt("numSpheres", 100);
		computeShader.setFloat("specStrength", specStrength);
		computeShader.setFloat("exponent", exponent);
		

		// Upload sphere data
		for (int i = 0; i < 100; ++i) {
			float wave = generateSineWave(i * 0.01, 1, time);
			spheres[i].position.y = wave;
			// Construct uniform name, e.g., "spheres[0].position"
			std::string uniformName = "spheres[" + std::to_string(i) + "].position";
			glUniform3fv(glGetUniformLocation(computeShader.ID, uniformName.c_str()), 1, glm::value_ptr(spheres[i].position));
			uniformName = "spheres[" + std::to_string(i) + "].color";
			glUniform3fv(glGetUniformLocation(computeShader.ID, uniformName.c_str()), 1, glm::value_ptr(spheres[i].color));
			uniformName = "spheres[" + std::to_string(i) + "].radius";
			computeShader.setFloat(uniformName.c_str(),spheres[i].radius);

			// Similarly upload other properties like radius, color, etc.
		}

		time += 0.0001;
		// Upload light data
		for (int i = 0; i < 1; ++i) {
			std::string uniformName = "lights[" + std::to_string(i) + "].position";
			glUniform3fv(glGetUniformLocation(computeShader.ID, uniformName.c_str()), 1, glm::value_ptr(lights[i].position));
			uniformName = "lights[" + std::to_string(i) + "].color";
			glUniform3fv(glGetUniformLocation(computeShader.ID, uniformName.c_str()), 1, glm::value_ptr(lights[i].color));
		}

		glDispatchCompute((unsigned int)TEXTURE_WIDTH / 10, (unsigned int)TEXTURE_HEIGHT / 10, 1);
		

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// render image to quad
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		screenQuad.use();

		renderQuad();

		ImGui::Begin("Settings");

		if (ImGui::DragFloat("Specular strength", (float*)&specStrength)) {

		}
		if (ImGui::DragFloat("Specular exponent", (float*)&exponent)) {

		}


		if (ImGui::DragFloat3("Light position", (float*)&lights[0].position)) {
		}

		if (ImGui::DragFloat3("Camera position", (float*)&campos)) {

		}


		if (ImGui::DragFloat3("Sphere position 0", (float*)&spheres[0].position)) {

		}
		if (ImGui::ColorEdit3("Sphere color 0", (float*)&spheres[0].color)) {

		}


		if (ImGui::DragFloat3("Sphere position 1", (float*)&spheres[1].position)) {

		}



		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteTextures(1, &texture);
	glDeleteProgram(screenQuad.ID);
	glDeleteProgram(computeShader.ID);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


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

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

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

	if (navigate_mouse) {
		camera.ProcessMouseMovement(xoffset, yoffset);
	}
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		navigate_mouse = true;
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		navigate_mouse = false;

}
