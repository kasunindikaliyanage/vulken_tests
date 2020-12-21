#pragma once
//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

#include "utilities.h"

class vulkan_renderer {
	
	GLFWwindow* window;
	VkInstance instance;

	struct {
		VkPhysicalDevice	physical_device;
		VkDevice			logical_device;
	}main_device;

	VkQueue graphics_queue;
	VkQueue presentation_queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swap_chain;

	VkFormat swap_chain_image_format;
	VkExtent2D swap_chain_extent;

	std::vector<SwapChainImage> swap_chain_images;

	VkPipelineLayout pipeline_layout;

	// Create the vulkan instance
	void create_instance();
	void create_logical_device();
	void create_surface();
	void create_swap_chain();
	void create_graphic_pipeline();
	void create_renderpass();

	// Get functions
	void get_physical_device();

	// Check whether the extension for the instance are supported
	bool check_instance_extension_support( std::vector<const char*>* extensions );
	bool check_device_extension_support( VkPhysicalDevice physical_device);
	bool check_device_suitable( VkPhysicalDevice physical_device );
	
	// Getter
	QueueFamilyIndicies get_queue_family(VkPhysicalDevice physical_device);
	SwapChainDetails get_swap_chain_details(VkPhysicalDevice physical_device);

	// Choose
	VkSurfaceFormatKHR choose_best_surface_format( std::vector<VkSurfaceFormatKHR> formats );
	VkPresentModeKHR choose_best_present_mode( std::vector<VkPresentModeKHR> modes );
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR surface_capabilities );

	VkImageView create_image_view( VkImage image, VkFormat format, VkImageAspectFlags flags );
	VkShaderModule create_shader_module( const std::vector<char> code );

public:
	vulkan_renderer();

	int init(GLFWwindow* new_window);

	void cleanup();
};