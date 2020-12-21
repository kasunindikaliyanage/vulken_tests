#pragma once

#include <fstream>

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


struct SwapChainImage {
	VkImage image;
	VkImageView image_view;

};

inline std::vector<char> read_shader_file(const std::string file_name)
{
	std::ifstream file(file_name, std::ios::binary| std::ios::ate);

	if ( !file.is_open())
	{
		std::string error_msg("Fail to open the file " + file_name);
		throw std::runtime_error(error_msg.c_str());
	}

	size_t file_size = file.tellg();

	std::vector<char> char_buffer(file_size);

	//move the file pointer to start position
	file.seekg(0);
	file.read(char_buffer.data(), file_size);
	file.close();

	return char_buffer;
}