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
		create_surface();
		get_physical_device();
		create_logical_device();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR : %s \n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void vulkan_renderer::cleanup()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(main_device.logical_device, nullptr);
	vkDestroyInstance(instance, nullptr);
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

	// Check whether the instance extensions are supported
	if (!check_instance_extension_support(&instance_extensions))
	{
		throw std::runtime_error(" VKinstance does not support required extensions \n ");
	}

	// Create information for VkInstance
	VkInstanceCreateInfo create_info = {};
	create_info.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo		= &app_info;
	create_info.enabledExtensionCount	= instance_extensions.size();
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

void vulkan_renderer::create_logical_device()
{
	QueueFamilyIndicies indicies = get_queue_family(main_device.physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<int> queue_family_indices = { indicies.graphics_family, indicies.presentation_family };

	for (int queue_family_index: queue_family_indices)
	{
		VkDeviceQueueCreateInfo	queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount = 1;

		float priority = 1.0f;
		queue_create_info.pQueuePriorities = &priority;

		queue_create_infos.push_back(queue_create_info);
	}

	VkDeviceCreateInfo logical_device_info = {};
	logical_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logical_device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	logical_device_info.pQueueCreateInfos = queue_create_infos.data();
	logical_device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	logical_device_info.ppEnabledExtensionNames = device_extensions.data();

	VkPhysicalDeviceFeatures physical_device_features = {};

	logical_device_info.pEnabledFeatures = &physical_device_features;
	
	VkResult result = vkCreateDevice(main_device.physical_device,&logical_device_info,nullptr,&main_device.logical_device);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Error : Failed to create Logical device \n");
	}
	else
	{
		printf("Logical device creation is  a success \n");
	}

	// Queues are created at the same time as device.
	// 0 since only one queue
	vkGetDeviceQueue( main_device.logical_device, indicies.graphics_family, 0, &graphics_queue );
	vkGetDeviceQueue(main_device.logical_device, indicies.presentation_family, 0, &presentation_queue);
}

void vulkan_renderer::create_surface()
{
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(" Error: Failed to create the surface \n");
	}
	else
	{
		printf("Surface creation is  a success \n");
	}
}

void vulkan_renderer::get_physical_device()
{
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

	if ( physical_device_count == 0 )
	{
		throw std::runtime_error(" Cannot find any GPUs for this Vulkan instance \n");
	}

	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

	for( const auto &device: physical_devices)
	{
		if (check_device_suitable(device))
		{
			main_device.physical_device = device;
			break;
		}
	}
}

bool vulkan_renderer::check_instance_extension_support(std::vector<const char*>* check_extensions)
{
	// First get the number of extension
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr); 

	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

	for ( const auto &check_extension: *check_extensions )
	{
		bool has_extension = false;
		
		for ( const auto& extension : extensions )
		{
			if ( !strcmp( check_extension,extension.extensionName ))
			{
				has_extension = true;
				break;
			}
		}

		if (!has_extension)
		{
			return false;
		}
	}

	return true;
}

bool vulkan_renderer::check_device_extension_support(VkPhysicalDevice physical_device)
{
	// First get the number of extension
	uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	if (extension_count == 0)
		return false;

	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extensions.data());

	for (const auto& check_extension : device_extensions)
	{
		bool has_extension = false;

		for (const auto& extension : extensions)
		{
			if (!strcmp(check_extension, extension.extensionName))
			{
				has_extension = true;
				break;
			}
		}

		if (!has_extension)
		{
			return false;
		}
	}

	return true;
}

bool vulkan_renderer::check_device_suitable(VkPhysicalDevice physical_device)
{
	//VkPhysicalDeviceProperties physical_device_props;
	//vkGetPhysicalDeviceProperties(physical_device, &physical_device_props);

	//VkPhysicalDeviceFeatures physical_device_features;
	//vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
	bool extension_supported = check_device_extension_support(physical_device);

	bool swap_chain_valid = false;
	if (extension_supported)
	{
		SwapChainDetails swap_chain_details = get_swap_chain_details(physical_device);
		swap_chain_valid = !swap_chain_details.present_modes.empty() && !swap_chain_details.surface_formats.empty();
	}

	QueueFamilyIndicies indicies = get_queue_family(physical_device);
	return indicies.is_valid() && extension_supported && swap_chain_valid;
}

QueueFamilyIndicies vulkan_renderer::get_queue_family( VkPhysicalDevice physical_device )
{
	QueueFamilyIndicies indicies;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &queue_family_count, nullptr );

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
	
	int i = 0;
	for ( const auto &queue_family : queue_families )
	{
		if ( queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			indicies.graphics_family = i;
		}

		////check if queue family support presentation
		VkBool32 presentation_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR( physical_device, i, surface, &presentation_support );

		//check if queue is presentation type, can be both graphics and presentation
		if (queue_family.queueCount > 0 && presentation_support)
		{
			indicies.presentation_family = i;
		}

		// Check if queue familiy indices are in valid state
		if (indicies.is_valid())
			break;

		i++;
	}

	return indicies;
}

SwapChainDetails vulkan_renderer::get_swap_chain_details(VkPhysicalDevice physical_device)
{
	SwapChainDetails swap_chain_details;

	// Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &swap_chain_details.surface_capabilities);

	// Formats
	uint32_t formats_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, nullptr);
	
	if (formats_count != 0)
	{
		swap_chain_details.surface_formats.resize(formats_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, 
			swap_chain_details.surface_formats.data());
	}

	// Present modes
	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0)
	{
		swap_chain_details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count,
			swap_chain_details.present_modes.data());
	}

	return swap_chain_details;
}
