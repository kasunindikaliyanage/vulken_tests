#pragma once

const std::vector< const char*> device_extensions
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndicies{
	int graphics_family = -1;
	int presentation_family = -1;

	//check if queue families are valid
	bool is_valid()
	{
		return graphics_family >= 0 && presentation_family >= 0 ;
	}
};


struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surface_capabilities;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};