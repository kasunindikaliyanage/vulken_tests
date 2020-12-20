#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#define GLM_FORCE_RADIANCE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm\glm.hpp>
#include <glm\mat4x4.hpp>

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan test", nullptr, nullptr);

	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	printf("Extension count : %i \n", extension_count);

	glm::mat4x4 test_matrix(1.0f);
	glm::vec4 test_vec(1.0f);
	auto result = test_matrix * test_vec;

	while ( !(glfwWindowShouldClose(window)))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}