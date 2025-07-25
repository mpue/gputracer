﻿#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Shader.h"
#include "Camera.h"
#include "ComputeShader.h"
#include <sstream>
#include <iostream>
#include <random>
#include <map>
#include <regex>
#include <filesystem>

#include "TextEditor.h"
#include "imgui/ImGuiFileDialog.h"
#include "ShaderBrowser.h"
#include "ShaderClip.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


using namespace glm;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void renderQuad();
int InitGL(bool& retFlag);
void InitUI();
void QueryLimits();
void DispatchAndDisplay(GLuint _ubo, float time, int fCounter);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void recompileShader();
void renderMenu();
int quitApplication();
void renderUI();
void addTexture(unsigned int* id);
void createErrorMarkers(std::vector<std::string> errors);
glm::vec2 calculateNewImageSize(const glm::vec2& imageSize, const glm::vec2& viewportSize);
void saveImage(int frame, unsigned int texture);
void updateCamera(GLFWwindow* window);
void createFramebuffer();

GLFWwindow* window = nullptr;

std::vector<unsigned int*> texture_list;
std::map<unsigned int, bool> visibles;
std::vector<unsigned int> shadersToDelete;

Shader screenQuad;
ComputeShader* Channel0;
ComputeShader* Channel1;
unsigned int channel0Tex;
unsigned int channel1Tex;
Camera camera;
vec3 campos;
unsigned int framebuffer;
unsigned int textureColorbuffer;
unsigned int rbo;

TextEditor* editor = nullptr;

unsigned int quadVAO = 0;
unsigned int quadVBO;

bool texture_slots_open = true;
bool editor_open = true;
bool output_open = true;
bool settings_open = false;
bool fullscreen = false;
bool show_open_file = false;
bool show_Save_new_file = false;
bool running = false;
bool rendering = false;

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
const unsigned int TEXTURE_WIDTH = 1920, TEXTURE_HEIGHT = 1080;

std::unique_ptr<ShaderBrowser> shaderBrowser;
void load(const std::string& filePath, const std::string& filePathName);

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

unsigned int textures[16];

uint numshaders = 0;
ComputeShader* currentShader = nullptr;

int main(int argc, char* argv[])
{
	for (int i = 0; i < 64; i++) {
		visibles[i] = false;
	}

	bool retFlag;
	int retVal = InitGL(retFlag);
	if (retFlag) return retVal;

	InitUI();

	editor = new TextEditor();
	editor->SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	QueryLimits();
	createFramebuffer();

	// build and compile shaders
	// -------------------------
	screenQuad = Shader("vertex.vert", "fragment.frag");
	computeShaders = std::map<unsigned int, ComputeShader*>();

	screenQuad.use();
	screenQuad.setInt("tex", 0);

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
		if (!fullscreen) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::PushFont(defaultFont);;
			renderMenu();
			ImGui::DockSpaceOverViewport();
		}

		if (rendering) {
			saveImage(fCounter, *texture_list.at(0));
			deltaTime = 1.0 / 60.0f;
			time += deltaTime;
			fCounter++;
		}
		else {
			// Set frame time
			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

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

		}

		DispatchAndDisplay(_ubo, time, fCounter);

		shaderBrowser = std::make_unique<ShaderBrowser>("./shaders/");
		shaderBrowser->onShaderSelected = [](const std::filesystem::path& path) {
			load("./shaders", path.filename().string());
			};

		// render image to quad
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (fullscreen) {
			screenQuad.use();
			glActiveTexture(GL_TEXTURE0 + *texture_list.at(0));
			glBindTexture(GL_TEXTURE_2D, *texture_list.at(0));
			screenQuad.setInt("tex", *texture_list.at(0));
			renderQuad();
		}
		else {
			renderUI();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		processInput(window);
		updateCamera(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	for (auto& pair : computeShaders) {
		delete pair.second;  // pair.second is the pointer to the object
	}

	// Clear the map
	computeShaders.clear();

	quitApplication();
}

void DispatchAndDisplay(GLuint _ubo, float time, int fCounter)
{
	// make sure writing to image has finished before read
	glDispatchCompute((unsigned int)TEXTURE_WIDTH / 10, (unsigned int)TEXTURE_HEIGHT / 10, 1);

	int v_idx = 0;
	std::map<unsigned int, ComputeShader*>::iterator it;

	std::vector<std::pair<unsigned int, ComputeShader*>> copy(
		computeShaders.begin(), computeShaders.end()
	);

	for (auto& it : copy)
	{
		if (running) {
			it.second->use();

			glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(FragUBO), (void*)&ubo, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, 1, _ubo);

			if (it.second->computePath.find("channel") == std::string::npos) {
				glActiveTexture(GL_TEXTURE0 + channel0Tex);
				glBindTexture(GL_TEXTURE_2D, channel0Tex);
				it.second->setInt("iChannel0", channel0Tex);
				glActiveTexture(GL_TEXTURE0 + channel1Tex);
				glBindTexture(GL_TEXTURE_2D, channel1Tex);
				it.second->setInt("iChannel1", channel1Tex);
			}
			// meet shadertoy compat 
			it.second->setInt("numLights", 1);
			it.second->setFloat("specStrength", specStrength);
			it.second->setFloat("exponent", exponent);
			it.second->setFloat("time", time * speed);
			it.second->setFloat("iTime", time * speed);
			it.second->setFloat("speed", speed);
			it.second->setVec3("iMouse", mouse);
			it.second->setInt("iFrame", fCounter);

			// make sure writing to image has finished before read
			glBindImageTexture(GL_TEXTURE0 + it.first, it.first, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

			glDispatchCompute((unsigned int)TEXTURE_WIDTH / 10, (unsigned int)TEXTURE_HEIGHT / 10, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		}

		std::stringstream title;
		title << it.second->computePath;


	}
	// Erstelle eine Kopie der computeShaders, um Iterator-Probleme zu vermeiden
	std::vector<std::pair<unsigned int, ComputeShader*>> shaderList(
		computeShaders.begin(), computeShaders.end()
	);

	for (auto& it : shaderList) {
		unsigned int id = it.first;
		ComputeShader* shader = it.second;

		// Sichtbarkeitsstatus initialisieren, falls neu
		if (visibles.count(id) == 0)
			visibles[id] = true;

		std::stringstream title;
		title << "Shader " << id;

		bool open = visibles[id];
		// Fenster anzeigen mit ImGui, Sichtbarkeitsstatus verlinken
		if (ImGui::Begin(title.str().c_str(), &open)) {
			// Bild zeichnen (wenn vorhanden)
			ImVec2 size = ImGui::GetContentRegionAvail();
			ImGui::Image((void*)(intptr_t)id, size);
			ImGui::End();

		}
		visibles[id] = open;

		// Fenster wurde vom Benutzer geschlossen → Shader entfernen
		if (!open)
			shadersToDelete.push_back(id);
	}

	// Shader und Texturen entfernen, wenn geschlossen
	for (auto id : shadersToDelete) {
		auto it = computeShaders.find(id);
		if (it != computeShaders.end()) {
			if (currentShader == it->second)
				currentShader = nullptr;

			delete it->second;
			computeShaders.erase(it);
		}

		auto texIt = std::find_if(texture_list.begin(), texture_list.end(), [id](unsigned int* texPtr) {
			return *texPtr == id;
			});

		if (texIt != texture_list.end()) {
			glDeleteTextures(1, *texIt);
			texture_list.erase(texIt);
			// delete* texIt;
			visibles[id] = false;
		}

	}
}

void QueryLimits()
{
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
}

void InitUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	editorFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", 18.0f);
	defaultFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\arial.ttf", 18.0f);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");
}

int InitGL(bool& retFlag)
{
	retFlag = true;
	texture_list = std::vector<unsigned int*>();

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
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "gputracer", NULL, NULL);
	if (window == nullptr)
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
	retFlag = false;
	return {};
}


void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f, 1.0f, 0.0f,		0.0f,1.0f,
			-1.0f,-1.0f, 0.0f,		0.0f,0.0f,
			 1.0f, 1.0f, 0.0f,  	1.0f,1.0f,
			1.0f, -1.0f, 0.0f,		1.0f,0.0f,
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
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		fullscreen = false;
		rendering = false;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if ((glfwGetKey(window, GLFW_KEY_F5) == GLFW_RELEASE)) {
		running = !running;
	}
	if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS)
	{
		fullscreen = true;
		rendering = true;
	}

	if ((glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS)) {
		fullscreen = !fullscreen;
		std::cout << "Fullscreen " << fullscreen << std::endl;
	}
	if ((glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS))
		recompileShader();
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS)
	{
		saveImage(-1, *texture_list.at(texture_list.size() - 1));
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

void recompileShader() {
	if (currentShader != nullptr) {
		currentShader->computeCode = editor->GetText().c_str();
		createErrorMarkers(currentShader->compile());
	}
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

void saveImage(int frame, unsigned int texture) {

	GLint width, height;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

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

	std::stringstream ss;

	if (frame >= 0) {
		ss << "output_" << frame << ".png";

	}
	else {
		ss << "output.png";
	}

	// Save image
	stbi_write_png(ss.str().c_str(), width, height, 4, buffer, width * 4);

	// Clean up
	delete[] buffer;
}

void renderMenu() {
	if (ImGui::BeginMainMenuBar()) {
		/*-File-------------------------------------------*/

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New shader")) {
				std::ifstream cShaderFile;
				// ensure ifstream objects can throw exceptions:
				cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
				try
				{
					// open files
					cShaderFile.open("shaders/template.comp");
					std::stringstream cShaderStream;
					cShaderStream << cShaderFile.rdbuf();
					cShaderFile.close();
					std::string templateShader = cShaderStream.str();
					editor->SetText(templateShader);
					show_Save_new_file = true;


				}
				catch (std::ifstream::failure& e)
				{
					std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
				}
			}
			if (ImGui::MenuItem("Load shader")) {
				show_open_file = true;
			}
			if (ImGui::MenuItem("Save")) {
				if (currentShader != nullptr) {
					currentShader->save();
					createErrorMarkers(currentShader->compile());
				}
			}
			if (ImGui::MenuItem("Quit")) {
				exit(0);
			}
			ImGui::EndMenu();
		}

		/*-Edit-------------------------------------------*/

		if (ImGui::BeginMenu("Edit")) {

			if (ImGui::MenuItem("Save image")) {
				saveImage(-1, *texture_list.at(texture_list.size() - 1));
			}

			if (ImGui::MenuItem("Recompile shader")) {
				recompileShader();
			}
			if (ImGui::MenuItem("Render images")) {
				rendering = true;
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
			if (ImGui::MenuItem("Texture slots")) {
				texture_slots_open = true;
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

	if (settings_open) {
		if (ImGui::Begin("Settings", &settings_open)) {			
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
		}
		else {
		}
	}

	if (texture_slots_open) {
		if (ImGui::Begin("Texture slots", &texture_slots_open)) {			
			ImGui::Columns(4, NULL);
			ImGui::Separator();

			for (int i = 0; i < texture_list.size(); i++) {
				if (i > 0 && i % 4 == 0) {
					ImGui::Separator();
				}
				vec2 imageSize = vec2(128, 128);

				ImGui::Image((void*)(intptr_t)*texture_list.at(i), ImVec2(imageSize.x, imageSize.y), { 0, 1 }, { 1, 0 });

				ImGui::NextColumn();
			}
			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::End();
		}
	}

	if (editor_open) {
		ImGui::PushFont(editorFont);
		if (ImGui::Begin("Shader code", &editor_open)) {
			editor->Render("Shader");
			ImGui::End();
		}
		ImGui::PopFont();
	}

	ImGui::PopFont();

	if (show_open_file) {
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
				load(filePath, filePathName);
			}

			// close
			ImGuiFileDialog::Instance()->Close();
			show_open_file = false;
		}

	}

	if (show_Save_new_file) {
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

				std::ofstream outputFile(filePathName);

				if (outputFile.is_open()) {

					outputFile << editor->GetText();
					outputFile.close();

					std::cout << "String written to file successfully." << std::endl;
				}
				else {
					std::cerr << "Unable to open file for writing." << std::endl;
				}

				unsigned int id;

				addTexture(&id);

				ComputeShader* shader = new ComputeShader(filePathName.c_str(), id);
				computeShaders.insert({ id,shader });
				editor->SetText(computeShaders[id]->computeCode);

			}

			// close
			ImGuiFileDialog::Instance()->Close();
			show_Save_new_file = false;
		}

	}
	shaderBrowser->renderUI();

}

void load(const std::string& filePath, const std::string& filePathName) {
	std::filesystem::path pathObj(filePathName);
	std::string fileName = pathObj.stem().string();

	std::filesystem::path channel0Path(filePath + "/" + fileName + "_channel0.comp");

	if (std::filesystem::exists(channel0Path)) {
		addTexture(&channel0Tex);
		ComputeShader* shader = new ComputeShader(channel0Path.string().c_str(), channel0Tex);
		shader->compile();
		computeShaders.insert({ channel0Tex,shader });
	}

	std::filesystem::path channel1Path(filePath + "/" + fileName + "_channel1.comp");

	if (std::filesystem::exists(channel1Path)) {
		addTexture(&channel1Tex);
		ComputeShader* shader = new ComputeShader(channel1Path.string().c_str(), channel1Tex);
		shader->compile();
		computeShaders.insert({ channel1Tex,shader });
	}

	unsigned int* id = new unsigned int;

	addTexture(id);

	std::filesystem::path mainShaderPath(filePath + "/" + fileName + ".comp");

	ComputeShader* shader = new ComputeShader(mainShaderPath.string().c_str(), *id);
	shader->compile();
	computeShaders.insert({ *id,shader });
	editor->SetText(computeShaders[*id]->computeCode);
	visibles[*id] = true;
	currentShader = shader;
}


void addTexture(unsigned int* id) {
	glGenTextures(1, id);
	std::cerr << "Generated texture ID: " << *id << std::endl;
	texture_list.push_back(id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(*id, *id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}



void createErrorMarkers(std::vector<std::string> errors) {
	TextEditor::ErrorMarkers markers;

	editor->SetErrorMarkers(markers);

	for (unsigned int i = 0; i < errors.size(); i++) {
		std::string error = errors.at(i);

		// parse message and line number
		std::regex re("\\((\\d+)\\).*: (.*)$");
		std::smatch match;

		if (std::regex_search(error, match, re) && match.size() > 2) {
			std::string lineNumber = match[1].str();
			std::string errorMessage = match[2].str();

			markers.insert(std::make_pair<int, std::string>(std::stoi(lineNumber), errorMessage.c_str()));
		}

	}

	editor->SetErrorMarkers(markers);
}

int quitApplication() {
	glDeleteTextures(1, &textures[0]);
	glDeleteProgram(screenQuad.ID);

	if (!fullscreen) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	delete (editor);

	glfwTerminate();

	return EXIT_SUCCESS;

}

void updateCamera(GLFWwindow* window) {

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

}

void createFramebuffer() {
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, TEXTURE_WIDTH, TEXTURE_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


uint ToUInt(int r, int g, int b, int a)
{
	return (uint)r << 24 | (uint)g << 16 | (uint)b << 8 | (uint)a;
}

double generateSineWave(double frequency, double amplitude, double time)
{
	return amplitude * std::sin(2 * PI * frequency * time);
}
