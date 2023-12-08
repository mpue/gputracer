#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <string>
#include <fstream>

// function which loads the contents of a text file into a string.

void checkError() {
	GLenum  err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cerr << "OpenGL error: " << err << "\n";
	}
}


std::string load_file_to_string(const std::string& file_path)
{
	std::ifstream file(file_path);
	std::string str((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	return str;
}

struct Color {
	float r, g, b;
};

unsigned int CreateShader(const std::string& source, GLint type)
{
	// Create a compute shader object
	unsigned int computeShader = glCreateShader(type);

	// Set the compute shader source code
	const char* src = source.c_str();
	glShaderSource(computeShader, 1, &src, nullptr);

	// Compile the compute shader
	glCompileShader(computeShader);

	checkError();
	// Check if the compute shader compiled successfully
	int success;
	glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		// If the compile failed, print the error message
		char infoLog[512];
		glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
		std::cerr << "Error: Compute shader compilation failed\n" << infoLog << std::endl;
		return 0;
	}

	return computeShader;
}

struct ComputeShader
{
	unsigned int id;

	ComputeShader(const std::string& source) {
		std::string shader = load_file_to_string(source);
		id = CreateShader(shader, GL_COMPUTE_SHADER);



	}

	// function which sets an uniform 

	void SetUniform(std::string name, glm::vec3 value) {
		int location = glGetUniformLocation(id, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void SetUniform(std::string name, float value) {
		int location = glGetUniformLocation(id, name.c_str());
		glUniform1f(location, value);
	}


	void Dispatch(int width, int height) {
		glDispatchCompute(width, height, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
};

struct Image
{
	int width;
	int height;
	std::vector<Color> pixels;

	Image(int width, int height) {
		this->width = width;
		this->height = height;
		pixels.resize(width * height);
	}

	Color GetPixel(int x, int y) const {
		return pixels[y * width + x];
	}
	void SetPixel(int x, int y, const Color& color) {
		pixels[y * width + x] = color;
	}


	void Save(const std::string& filename) const {

	}
};
struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

struct Sphere {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	glm::vec3 normal;
};


struct Camera
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 up;
	float fov;
	float aspectRatio;

	Ray GenerateRay(float x, float y) const {
		Ray ray;
		ray.origin = position;
		ray.direction = glm::normalize(direction + x * glm::normalize(glm::cross(direction, up)) + y * glm::normalize(up));
		return ray;
	}
};

struct Scene
{
	std::vector<Sphere> spheres;

	static Scene FromString(const std::string& str) {
		Scene scene;
		scene.spheres.push_back({ glm::vec3(0, 0, 0), 1 });
		return scene;
	}

	void Render(const Camera& camera, Image& image, ComputeShader& raytracer) {
		raytracer.SetUniform("cameraPosition", camera.position);
		raytracer.SetUniform("cameraDirection", camera.direction);
		raytracer.SetUniform("cameraUp", camera.up);
		raytracer.SetUniform("cameraFov", camera.fov);
		raytracer.SetUniform("cameraAspectRatio", camera.aspectRatio);
		raytracer.SetUniform("outputImage", 0);
		raytracer.SetUniform("numSpheres", 1);

		raytracer.Dispatch(image.width, image.height);
	}
};


struct Light {
	glm::vec3 position;
	glm::vec3 color;
};




int main()
{
	// Initialize GLFW
	if (!glfwInit())
	{
		std::cerr << "Error: GLFW initialization failed" << std::endl;
		return -1;
	}

	// Set the OpenGL version to 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

	// Create a GLFW window
	GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Error: GLFW window creation failed" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Make the OpenGL context current
	glfwMakeContextCurrent(window);

	// Load the OpenGL functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Error: Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Set the clear color
	glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
	// Parse the scene from a string
	Scene scene = Scene::FromString("scene.txt");

	// Create a camera
	Camera camera;
	camera.position = glm::vec3(0, 0, -2);
	camera.direction = glm::vec3(0, 0, 1);
	camera.up = glm::vec3(0, 1, 0);
	camera.fov = 45.0f;
	camera.aspectRatio = 800.0f / 600.0f;

	// Create a vector of spheres
	std::vector<Sphere> spheres = {
	  { glm::vec3(0, 0, 0), 1.0, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) },
	  // Add more spheres here...
	};

	// Create a buffer to store the sphere data
	GLuint sphereBuffer;
	glGenBuffers(1, &sphereBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(Sphere), spheres.data(), GL_STATIC_DRAW);

	// Bind the buffer to binding point 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereBuffer);

	struct Light {
		glm::vec3 position;
		glm::vec3 color;
	};

	// Create a vector of lights
	std::vector<Light> lights = {
	  { glm::vec3(10, 10, 10), glm::vec3(1, 1, 1) },
	  // Add more lights here...
	};

	// Create a buffer to store the light data
	GLuint lightBuffer;
	glGenBuffers(1, &lightBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(Light), lights.data(), GL_STATIC_DRAW);

	// Bind the buffer to binding point 1
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightBuffer);

	// Create an image
	Image image(800, 600);

	GLuint outputImage;
	glGenTextures(1, &outputImage);
	glBindTexture(GL_TEXTURE_2D, outputImage);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 800, 600);


	// Set the texture coordinates for each vertex in the fullscreen quad
	const float vertices[] = {
	  -1.0f,  1.0f, 0.0f, 1.0f,
	  -1.0f, -1.0f, 0.0f, 0.0f,
	   1.0f,  1.0f, 1.0f, 1.0f,
	   1.0f, -1.0f, 1.0f, 0.0f,
	};

	GLint fragmentShaderProgram = CreateShader(load_file_to_string("fragment.glsl"), GL_FRAGMENT_SHADER);
	GLint vertexShaderProgram = CreateShader(load_file_to_string("vertex.glsl"), GL_VERTEX_SHADER);
	ComputeShader raytracer("raytrace.glsl");
	// Run the main loop
	while (!glfwWindowShouldClose(window))
	{
		// Poll for events
		glfwPollEvents();

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// Swap the front and back buffers
		glfwSwapBuffers(window);

		glUseProgram(raytracer.id);
		// Bind the output image to binding point 0
		glBindImageTexture(0, outputImage, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		scene.Render(camera, image, raytracer);
		// Dispatch the compute shader
		raytracer.Dispatch(800, 600);
		glDispatchCompute(800 / 16, 600 / 16, 1);

		glUseProgram(fragmentShaderProgram);

		// Set the texture as the input to the fragment shader
		glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_2D, outputImage);
		glUniform1i(glGetUniformLocation(fragmentShaderProgram, "image"), 0);

		// Render the fullscreen quad
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}


	// Clean up
	glfwTerminate();
	return 0;
}
