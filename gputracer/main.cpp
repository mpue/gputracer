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
#include <sstream>
#include <iostream>
#include <random>
#include <map>

#include "TextEditor.h"
#include "ImGuiFileDialog.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void renderQuad();
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void myKeyCallbackFunc(GLFWwindow* window, int key, int scancode, int action, int mods);
void character_callback(GLFWwindow* window, unsigned int codepoint);
void recompileShader();
void renderMenu();
int quitApplication();
void renderUI();
void addTexture(unsigned int id);

glm::vec2 calculateNewImageSize(const glm::vec2& imageSize, const glm::vec2& viewportSize);

Shader screenQuad;

void saveImage();

using namespace glm;

Camera camera;
vec3 campos;

TextEditor* editor = nullptr;

bool editor_open = true;
bool output_open = true;
bool settings_open = true;
bool fullscreen = false;
bool show_open_File  = false;

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

struct Sphere
{
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	glm::vec3 normal;
	uint type;
};

struct Light
{
	glm::vec3 position;
	glm::vec3 color;
};

struct FragUBO
{
	mat4 invProjectionView;
	float near;
	float far;
} ubo;


vec3 mouse;
const double PI = 3.14159265358979323846;

ImFont* editorFont;
ImFont* defaultFont;

float specStrength = 1.0f;
float exponent = 64.0;
float speed = 1.0;

float fps = 0;

std::map<unsigned int, ComputeShader*> computeShaders;

// ComputeShader computeShaders[8];
unsigned int textures[16];

uint numshaders = 0;
unsigned int currentShader = 0;

uint ToUInt(int r , int g , int b , int a)
{
	return (uint)r << 24 | (uint)g << 16 | (uint)b << 8 | (uint)a;
}

double generateSineWave(double frequency, double amplitude, double time)
{
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
	glfwSetCharCallback(window, character_callback);
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
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	editorFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", 14.0f);
	defaultFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\arial.ttf", 14.0f);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");	

	editor = new TextEditor();
	// query limitations
	// -----------------
	int max_compute_work_group_count[3];
	int max_compute_work_group_size[3];
	int max_compute_work_group_invocations;

	for (int idx = 0; idx < 3; idx++)
	{
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
	screenQuad = Shader("vertex.vert", "fragment.frag");

	computeShaders = std::map<unsigned int, ComputeShader*>();

	// computeShaders[0] = new ComputeShader("mandelbulb.comp");

	// editor->SetText(computeShaders[0]->computeCode);

	screenQuad.use();
	screenQuad.setInt("tex", 0);

	// Create texture for opengl operation
	// -----------------------------------

	// render loop
	// -----------
	int fCounter = 0;

	float time = 0;

	while (!glfwWindowShouldClose(window))
	{
		if (!fullscreen) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();			
			ImGui::NewFrame();
			ImGui::PushFont(defaultFont);;
			renderMenu();
			ImGui::DockSpaceOverViewport();	

		}
	
		// Set frame time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		if (fCounter > 500)
		{
			std::cout << "FPS: " << 1 / deltaTime << "\r";
			fCounter = 0;
			if (fps > 0) {
				fps = (fps + 1 / deltaTime) / 2;
			}
			fps = 1 / deltaTime;
		}
		else
		{
			fCounter++;
		}
		
		time += deltaTime;

		std::map<unsigned int, ComputeShader*>::iterator it;

		for (it = computeShaders.begin(); it != computeShaders.end(); it++)
		{
			it->second->use();
			it->second->setInt("numLights", 1);
			it->second->setFloat("specStrength", specStrength);
			it->second->setFloat("exponent", exponent);
			it->second->setFloat("time", time*speed);
			it->second->setFloat("iTime", time*speed);
			it->second->setFloat("speed", speed);
			it->second->setVec3("iMouse", mouse);

			glActiveTexture(GL_TEXTURE0 + it->first);
			glBindTexture(GL_TEXTURE_2D, textures[it->first]);
			glBindImageTexture(GL_TEXTURE0 + it->first, it->first, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glDispatchCompute((unsigned int)TEXTURE_WIDTH / 10, (unsigned int)TEXTURE_HEIGHT / 10, 1);
			// make sure writing to image has finished before read

		}
		for (it = computeShaders.begin(); it != computeShaders.end(); it++) 
		{
			std::stringstream title;
			title << "Output " << it->first;
			output_open = ImGui::Begin(title.str().c_str(), &output_open);

			ImVec2 windowPos = ImGui::GetItemRectMin();

			vec2 windowSize;
			windowSize.x = ImGui::GetWindowContentRegionMax().x - windowPos.x;
			windowSize.y = ImGui::GetWindowContentRegionMax().y - windowPos.y;

			vec2 imageSize = vec2(TEXTURE_WIDTH, TEXTURE_HEIGHT);

			ImGui::Image((void*)(intptr_t)it->first, ImVec2(imageSize.x, imageSize.y), { 0, 1 }, { 1, 0 });
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			windowPos.x += 20;
			windowPos.y += 50;
			std::stringstream ss;
			ss << "FPS : " << fps;
			ss.str();
			drawList->AddText(windowPos, ToUInt(255, 255, 255, 255), ss.str().c_str());
			ImGui::End();
			
		}
		// render image to quad
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if (fullscreen) {
			screenQuad.use();
			renderQuad();
		}
		else {
			renderUI();
		}
	
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	quitApplication();
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
			-1.0f,
			1.0f,
			0.0f,
			0.0f,
			1.0f,
			-1.0f,
			-1.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			0.0f,
			1.0f,
			1.0f,
			1.0f,
			-1.0f,
			0.0f,
			1.0f,
			0.0f,
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
		fullscreen = false;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS)
	{
		saveImage();
	}
	if ((glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS))
		recompileShader();
	if ((glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS)) {
		fullscreen = !fullscreen;
		std::cout << "Fullscreen " << fullscreen << std::endl;
	}

}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	mouse.x = xpos;
	mouse.y = ypos;

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

	if (navigate_mouse)
	{
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
void character_callback(GLFWwindow* window, unsigned int c)
{
	if (isprint(c) || isspace(c))
	{
		// editor->EnterCharacter(char(c));
	}
}

void myKeyCallbackFunc(GLFWwindow* window, int key, int scancode, int action, int mods) {
}

void recompileShader() {
	//computeShader.computeCode = editor->GetText().c_str();
	//computeShader.compile();
}

glm::vec2 calculateNewImageSize(const glm::vec2& imageSize, const glm::vec2& viewportSize) {
	float imageAspectRatio = imageSize.x / imageSize.y;

	glm::vec2 newSize;
	newSize.x = viewportSize.x; // Set new width to viewport width
	newSize.y = viewportSize.x / imageAspectRatio; // Adjust height based on the image aspect ratio

	// If the new height exceeds the viewport height, scale down proportionally
	if (newSize.y > viewportSize.y) {
		float scaleDownFactor = viewportSize.y / newSize.y;
		newSize.x *= scaleDownFactor;
		newSize.y = viewportSize.y;
	}

	return newSize;
}

void saveImage() {
	GLint width, height;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	// Allocate buffer
	unsigned char* buffer = new unsigned char[width * height * 4]; // 4 for RGBA

	// Read pixels
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	// Flip the image Y axis
	for (int i = 0; i * 2 < height; ++i)
	{
		int index1 = i * width * 4;
		int index2 = (height - 1 - i) * width * 4;
		for (int j = width * 4; j > 0; --j)
		{
			unsigned char temp = buffer[index1];
			buffer[index1] = buffer[index2];
			buffer[index2] = temp;
			++index1;
			++index2;
		}
	}

	// Save image
	stbi_write_png("output.png", width, height, 4, buffer, width * 4);

	// Clean up
	delete[] buffer;
}

void renderMenu() {
	if (ImGui::BeginMainMenuBar()) {
		/*-File-------------------------------------------*/

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load shader")) {
				show_open_File = true;
			}
			if (ImGui::MenuItem("Save")) {

			}
			if (ImGui::MenuItem("Quit")) {				
				exit(0);
			}
			ImGui::EndMenu();
		}

		/*-Edit-------------------------------------------*/

		if (ImGui::BeginMenu("Edit")) {

			if (ImGui::MenuItem("Save image")) {
				saveImage();
			}

			if (ImGui::MenuItem("Recompile shader")) {
				recompileShader();
			}
			ImGui::EndMenu();

		}
		/*-View-------------------------------------------*/

		if (ImGui::BeginMenu("View")) {

			if (ImGui::MenuItem("Shader Editor")) {
				editor_open = true;
			}
			if (ImGui::MenuItem("Shader Output")) {
				output_open = true;
			}
			if (ImGui::MenuItem("Shader settings")) {
				settings_open = true;
			}
			if (ImGui::MenuItem("Fullscreen")) {
				fullscreen = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

	}
	


}

void renderUI() {
	settings_open = ImGui::Begin("Settings", &settings_open);

	if (ImGui::DragFloat("Specular strength", (float*)&specStrength))
	{
	}
	if (ImGui::DragFloat("Specular exponent", (float*)&exponent))
	{
	}
	if (ImGui::DragFloat("Speed", (float*)&speed))
	{
	}

	ImGui::End();

	std::map<unsigned int, ComputeShader*>::iterator it;
	for (it = computeShaders.begin(); it != computeShaders.end(); it++)
	{		}

	ImGui::PushFont(editorFont);
	editor_open = ImGui::Begin("Shader code", &editor_open);
	editor->Render("Shader");
	ImGui::End();
	ImGui::PopFont();
	ImGui::PopFont();


	if (show_open_File) {
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".comp",
			".", 1, nullptr, ImGuiFileDialogFlags_Modal);

		// display
		if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				// action
				unsigned int id;
				glGenTextures(1, &id);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, id);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
				glBindImageTexture(id, id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

				ComputeShader* shader = new ComputeShader(filePathName.c_str(),id);
				computeShaders.insert({ id,shader });
				editor->SetText(computeShaders[id]->computeCode);
			}

			// close
			ImGuiFileDialog::Instance()->Close();
			show_open_File = false;
		}

	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void addTexture(unsigned int id) {

}

int quitApplication() {
	glDeleteTextures(1, &textures[0]);
	glDeleteProgram(screenQuad.ID);
	// glDeleteProgram(computeShader.ID);

	if (!fullscreen) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	delete (editor);

	glfwTerminate();

	return EXIT_SUCCESS;

}