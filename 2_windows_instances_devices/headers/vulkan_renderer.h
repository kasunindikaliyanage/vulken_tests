#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <stdexcept>
#include <vector>

#include "utilities.h"

class vulkan_renderer {
	
	GLFWwindow* window;
	VkInstance instance;

	struct {
		VkPhysicalDevice	physical_device;
		VkDevice			logical_device;
	}main_device;

	VkQueue graphics_queue;

	// Create the vulkan instance
	void create_instance();
	void create_logical_device();

	// Get functions
	void get_physical_device();

	// Check whether the extension for the instance are supported
	bool check_instance_extension_support( std::vector<const char*>* extensions );
	bool check_device_suitable( VkPhysicalDevice physical_device );
	QueueFamilyIndicies get_queue_family(VkPhysicalDevice physical_device);

public:
	vulkan_renderer();

	int init(GLFWwindow* new_window);

	void cleanup();
};