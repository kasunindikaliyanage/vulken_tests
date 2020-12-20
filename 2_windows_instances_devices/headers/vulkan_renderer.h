#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <stdexcept>
#include <vector>

class vulkan_renderer {
	
	GLFWwindow* window;
	VkInstance instance;

	void create_instance();

public:
	vulkan_renderer();

	int init(GLFWwindow* new_window);
};