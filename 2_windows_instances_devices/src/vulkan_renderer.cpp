#include "..\headers\vulkan_renderer.h"

vulkan_renderer::vulkan_renderer()
{
}

int vulkan_renderer::init(GLFWwindow* new_window)
{
	window = new_window;

	try
	{
		create_instance();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR : %s \n", e.what());
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}


void vulkan_renderer::create_instance()
{
	//Application info
	VkApplicationInfo app_info = {};
	app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName	= "Vulkan test";
	app_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 1 );
	app_info.pEngineName		= "No Engine";
	app_info.engineVersion		= VK_MAKE_VERSION( 1, 0, 0 );
	app_info.apiVersion			= VK_API_VERSION_1_2;

	// Extension information
	std::vector<const char*> instance_extensions	= std::vector<const char*>();
	uint32_t extension_count						= 0;
	
	const char** glfw_extensions;
	glfw_extensions	= glfwGetRequiredInstanceExtensions( &extension_count );

	for ( size_t i = 0; i < extension_count; i++ )
	{
		instance_extensions.push_back( glfw_extensions[i] );
	}

	// Create information for VkInstance
	VkInstanceCreateInfo create_info = {};
	create_info.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo		= &app_info;
	create_info.enabledLayerCount		= instance_extensions.size();
	create_info.ppEnabledExtensionNames = instance_extensions.data();
	create_info.enabledLayerCount		= 0;
	create_info.ppEnabledLayerNames		= nullptr;

	VkResult result = vkCreateInstance(&create_info, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance");
	}
	else
	{
		printf("Instance creation is  a success \n");
	}

}