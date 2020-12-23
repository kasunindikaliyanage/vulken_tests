#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>

#include "..\headers\vulkan_renderer.h"

GLFWwindow* window;
vulkan_renderer renderer;

void init_window(std::string w_name = "Test window", const int width = 800, const int height = 600)
{
	//init GLFW
	glfwInit();
	
	//setup GLFW window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, w_name.c_str(), nullptr, nullptr);
}

int main()
{
	//create window
	init_window();

	//create a vulkan renderer instance
	if (renderer.init(window) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	//loop until closed
	while (!(glfwWindowShouldClose(window)))
	{
		glfwPollEvents();
		renderer.draw();
	}

	renderer.cleanup();

	//clean things up
	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}