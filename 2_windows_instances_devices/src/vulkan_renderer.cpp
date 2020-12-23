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
		create_swap_chain();
		create_renderpass();
		create_graphic_pipeline();
		create_framebuffers();
		create_command_pool();
		create_commandbuffer();
		record_commands();
		create_synchronization();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR : %s \n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void vulkan_renderer::draw()
{
	//Get the next image
	uint32_t image_index;
	vkAcquireNextImageKHR(main_device.logical_device, swap_chain, std::numeric_limits<uint32_t>::max(), image_available, VK_NULL_HANDLE, &image_index);

	// submit command buffer to render
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &image_available;
	
	VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandbuffers[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &render_finished;

	VkResult result = vkQueueSubmit( graphics_queue, 1, &submit_info, VK_NULL_HANDLE);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit the commands to the queue \n");
	}
	else
	{
		printf("Submit to queue for drawing is success \n");
	}


	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(graphics_queue, &present_info);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present image \n");
	}
	else
	{
		printf("Presenting image is success \n");
	}

}


void vulkan_renderer::cleanup()
{
	vkDestroySemaphore(main_device.logical_device, render_finished, nullptr);
	vkDestroySemaphore(main_device.logical_device, image_available, nullptr);

	vkDestroyCommandPool(main_device.logical_device, graphics_cmd_pool, nullptr);

	for (auto framebuffer : swapchain_framebuffers)
	{
		vkDestroyFramebuffer(main_device.logical_device, framebuffer, nullptr);
	}

	vkDestroyPipeline(main_device.logical_device, graphics_pipeline, nullptr);

	vkDestroyPipelineLayout(main_device.logical_device, pipeline_layout, nullptr);

	vkDestroyRenderPass(main_device.logical_device, render_pass, nullptr);

	for (auto image: swap_chain_images)
	{
		vkDestroyImageView(main_device.logical_device, image.image_view, nullptr);
	}

	vkDestroySwapchainKHR(main_device.logical_device, swap_chain, nullptr);
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


void vulkan_renderer::create_swap_chain()
{
	SwapChainDetails details = get_swap_chain_details(main_device.physical_device);

	// Choose best format
	VkSurfaceFormatKHR surface_format = choose_best_surface_format(details.surface_formats);

	// Choose best presentation mode
	VkPresentModeKHR present_mode = choose_best_present_mode(details.present_modes);
	
	// Choose best swap chain image resolution
	VkExtent2D extent = choose_swap_extent(details.surface_capabilities);

	uint32_t image_count = details.surface_capabilities.minImageCount + 1;

	if ( details.surface_capabilities.maxImageCount > 0 
		&& image_count > details.surface_capabilities.maxImageCount)
	{
		image_count = details.surface_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swap_chain_create_info = {};
	swap_chain_create_info.sType			=	VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_chain_create_info.surface			=	surface;
	swap_chain_create_info.imageFormat		=	surface_format.format;
	swap_chain_create_info.presentMode		=	present_mode;
	swap_chain_create_info.imageExtent		=	extent;
	swap_chain_create_info.minImageCount	=	image_count;
	swap_chain_create_info.imageArrayLayers =	1 ;
	swap_chain_create_info.imageUsage		=	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swap_chain_create_info.preTransform		=	details.surface_capabilities.currentTransform;
	swap_chain_create_info.compositeAlpha	=	VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_chain_create_info.clipped			=	VK_TRUE;

	QueueFamilyIndicies indices = get_queue_family( main_device.physical_device );

	if (indices.graphics_family != indices.presentation_family)
	{
		uint32_t queue_family_indices[] = {
			(uint32_t)indices.graphics_family,
			(uint32_t)indices.presentation_family
		};

		swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swap_chain_create_info.queueFamilyIndexCount = 2;
		swap_chain_create_info.pQueueFamilyIndices = queue_family_indices;

	}
	else
	{
		swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE ;
		swap_chain_create_info.queueFamilyIndexCount = 0 ;
		swap_chain_create_info.pQueueFamilyIndices = nullptr;
	}

	swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

	//create the swap_chain
	VkResult result =  vkCreateSwapchainKHR(main_device.logical_device, &swap_chain_create_info, nullptr, &swap_chain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(" Error : Faild to create swap chain \n");
	}
	else
	{
		printf("Swap chain creation sucessful \n");
	}

	swap_chain_image_format = surface_format.format;
	swap_chain_extent = extent;

	uint32_t swap_chain_image_count;
	vkGetSwapchainImagesKHR(main_device.logical_device, swap_chain, &swap_chain_image_count, nullptr);
	std::vector<VkImage> images(swap_chain_image_count);
	vkGetSwapchainImagesKHR(main_device.logical_device, swap_chain, &swap_chain_image_count, images.data());

	for ( VkImage image : images)
	{
		SwapChainImage swap_chain_image = {};
		swap_chain_image.image = image;
		swap_chain_image.image_view = create_image_view(image, swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
		//Create image view

		swap_chain_images.push_back(swap_chain_image);
	}

}


void vulkan_renderer::create_renderpass()
{
	//Color attachment of the renderpass
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = swap_chain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_reference = {};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;

	//layout transition using subpass dependencies
	std::array<VkSubpassDependency, 2> subpass_dependencies;

	subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	subpass_dependencies[0].dstSubpass = 0;
	subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[0].dependencyFlags = 0;

	subpass_dependencies[1].srcSubpass = 0;
	subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpass_dependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderpass_create_info = {};
	renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpass_create_info.attachmentCount = 1;
	renderpass_create_info.pAttachments = &color_attachment;
	renderpass_create_info.subpassCount = 1;
	renderpass_create_info.pSubpasses = &subpass;
	renderpass_create_info.dependencyCount = static_cast<uint32_t>(subpass_dependencies.size());
	renderpass_create_info.pDependencies = subpass_dependencies.data();

	VkResult result = vkCreateRenderPass(main_device.logical_device, &renderpass_create_info, nullptr, &render_pass);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(" Error: Failed to create the RenderPass \n");
	}
	else
	{
		printf("RenderPass creation is  a success \n");
	}
}


void vulkan_renderer::create_framebuffers()
{
	//There will be one framebuffer for each image in swapchain
	swapchain_framebuffers.resize(swap_chain_images.size());

	for (size_t i = 0; i < swapchain_framebuffers.size(); i++)
	{
		std::array<VkImageView, 1> attachments = {
			swap_chain_images[i].image_view
		};

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass;
		framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.pAttachments = attachments.data();
		framebuffer_create_info.width = swap_chain_extent.width;
		framebuffer_create_info.height = swap_chain_extent.height;
		framebuffer_create_info.layers = 1;

		VkResult result = vkCreateFramebuffer(main_device.logical_device, &framebuffer_create_info, nullptr, &swapchain_framebuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(" Error: Failed to create the framebuffer \n");
		}
		else
		{
			printf("Framebuffer creation is  a success \n");
		}
	}

}


void vulkan_renderer::create_command_pool()
{
	QueueFamilyIndicies queue_family_indicies = get_queue_family(main_device.physical_device);

	VkCommandPoolCreateInfo cmd_pool_create_info = {};
	cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_create_info.queueFamilyIndex = queue_family_indicies.graphics_family;

	VkResult result = vkCreateCommandPool(main_device.logical_device, &cmd_pool_create_info, nullptr, &graphics_cmd_pool);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(" Error: Failed to create the Command pool \n");
	}
	else
	{
		printf("Command pool creation is  a success \n");
	}

}


void vulkan_renderer::create_commandbuffer()
{
	commandbuffers.resize(swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo cb_alloc_info = {};
	cb_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cb_alloc_info.commandPool = graphics_cmd_pool;
	cb_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cb_alloc_info.commandBufferCount = static_cast<uint32_t>(commandbuffers.size());

	VkResult result=  vkAllocateCommandBuffers(main_device.logical_device, &cb_alloc_info, commandbuffers.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(" Error: Failed to allocate command buffer \n");
	}
	else
	{
		printf("Command buffer allocation is  a success \n");
	}
}


void vulkan_renderer::create_synchronization()
{
	VkSemaphoreCreateInfo semaphore_ci = {};
	semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if ((vkCreateSemaphore(main_device.logical_device, &semaphore_ci, nullptr, &image_available) != VK_SUCCESS)
		|| (vkCreateSemaphore(main_device.logical_device, &semaphore_ci, nullptr, &render_finished) != VK_SUCCESS))
	{
		throw std::runtime_error(" Error: Failed to create Semaphore \n");
	}
}


void vulkan_renderer::record_commands()
{
	VkCommandBufferBeginInfo cb_begin_info = {};
	cb_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cb_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkRenderPassBeginInfo rp_begin_info = {};
	rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin_info.renderPass = render_pass;
	rp_begin_info.renderArea.offset = { 0,0 };	//start point of render pass
	rp_begin_info.renderArea.extent = swap_chain_extent;
	
	VkClearValue clear_value[] = {
		{ 0.6f, 0.65f, 0.4f, 1.0f }
	};

	rp_begin_info.pClearValues = clear_value;
	rp_begin_info.clearValueCount = 1;

	for (size_t i = 0; i < commandbuffers.size(); i++)
	{
		rp_begin_info.framebuffer = swapchain_framebuffers[i];

		VkResult result = vkBeginCommandBuffer(commandbuffers[i], &cb_begin_info);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(" Error: Failed record command buffer \n");
		}
		else
		{
			printf("Command buffer Recording is  a success \n");
		}

		//render pass
		vkCmdBeginRenderPass(commandbuffers[i], &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		
		{
			vkCmdBindPipeline(commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
			vkCmdDraw(commandbuffers[i], 3, 1, 0, 0);
		}
		
		vkCmdEndRenderPass(commandbuffers[i]);

		result = vkEndCommandBuffer(commandbuffers[i]);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(" Error: Failed Stop record command buffer \n");
		}
		else
		{
			printf("Command buffer Recording Stopping is  a success \n");
		}
	}
}


void vulkan_renderer::create_graphic_pipeline()
{
	auto vertex_shader_code = read_shader_file("../shaders/vert.spv");
	auto fragment_shader_code = read_shader_file("../shaders/frag.spv");

	//build shader modules
	VkShaderModule vertex_shader_module = create_shader_module(vertex_shader_code);
	VkShaderModule fragment_shader_module = create_shader_module(fragment_shader_code);

	//vertex shader creation info
	VkPipelineShaderStageCreateInfo vertex_shader_create_info = {};
	vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_create_info.stage	= VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_create_info.module = vertex_shader_module;
	vertex_shader_create_info.pName = "main";

	//fragment shader creation info
	VkPipelineShaderStageCreateInfo fragment_shader_create_info = {};
	fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_create_info.module = fragment_shader_module;
	fragment_shader_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stage_info[] = { vertex_shader_create_info, fragment_shader_create_info };
		
	// CREATE PIPELINE
	
	// PIPELINE - Vertex input
	VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
	vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_info.pVertexBindingDescriptions = nullptr;
	vertex_input_state_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_info.pVertexAttributeDescriptions = nullptr;

	// PIPELINE - input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = false;

	// PIPELINE - Viewport & Scissor
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swap_chain_extent.width;
	viewport.height = (float)swap_chain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0,0};
	scissor.extent = swap_chain_extent;

	VkPipelineViewportStateCreateInfo viewport_create_info = {};
	viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_create_info.viewportCount = 1;
	viewport_create_info.pViewports = &viewport;
	viewport_create_info.scissorCount = 1;
	viewport_create_info.pScissors = &scissor;

	// PIPELINE - dynamic state for resize and etc:
	// for now not implemented.

	// PIPELINE - Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
	rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_create_info.depthClampEnable = VK_FALSE;
	rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_create_info.lineWidth = 1.0f;
	rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer_create_info.depthBiasEnable = VK_FALSE;

	// PIPELINE - Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling_create_info = {};
	multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_create_info.sampleShadingEnable = VK_FALSE;
	multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// PIPELINE - Blending
	VkPipelineColorBlendAttachmentState blend_attach_state = {};
	blend_attach_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	blend_attach_state.blendEnable = VK_TRUE;

	blend_attach_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC1_ALPHA;
	blend_attach_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attach_state.colorBlendOp = VK_BLEND_OP_ADD;

	blend_attach_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attach_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attach_state.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
	color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.logicOpEnable = VK_FALSE;
	color_blend_state_create_info.attachmentCount = 1;
	color_blend_state_create_info.pAttachments = &blend_attach_state;

	// PIPELINE - Layout
	VkPipelineLayoutCreateInfo layout_create_info = {};
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.setLayoutCount = 0;
	layout_create_info.pSetLayouts = nullptr;
	layout_create_info.pushConstantRangeCount = 0;
	layout_create_info.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(main_device.logical_device, &layout_create_info, nullptr, &pipeline_layout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Pipeline layout");
	}
	else
	{
		printf("Pipeline layout creation is  a success \n");
	}

	// PIPELINE - Deapth/Stencil configuration. TODO
	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;
	pipeline_create_info.pStages = shader_stage_info;
	pipeline_create_info.pVertexInputState = &vertex_input_state_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_info;
	pipeline_create_info.pViewportState = &viewport_create_info;
	pipeline_create_info.pDynamicState = nullptr;
	pipeline_create_info.pRasterizationState = &rasterizer_create_info;
	pipeline_create_info.pMultisampleState = &multisampling_create_info;
	pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
	pipeline_create_info.pDepthStencilState = nullptr;
	pipeline_create_info.layout = pipeline_layout;
	pipeline_create_info.renderPass = render_pass;
	pipeline_create_info.subpass = 0;

	// Piepeline derivatives
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = 0;

	result = vkCreateGraphicsPipelines(main_device.logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(" Error: Failed to create a Graphics pipeline\n");
	}
	else
	{
		printf("Graphics pipeline creation is  a success \n");
	}

	// Destroy shader modules
	vkDestroyShaderModule(main_device.logical_device, fragment_shader_module, nullptr);
	vkDestroyShaderModule(main_device.logical_device, vertex_shader_module, nullptr);
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


// We select
// format		: R8G8B8A8 UNORM
// colorspace	: SRGB
VkSurfaceFormatKHR vulkan_renderer::choose_best_surface_format(std::vector<VkSurfaceFormatKHR> formats)
{
	//VK_FORMAT_B8G8R8A8_UNORM
	//VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (auto const &format: formats )
	{
		if ((format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}


VkPresentModeKHR vulkan_renderer::choose_best_present_mode(std::vector<VkPresentModeKHR> modes)
{
	for (auto const& presentation_mode : modes)
	{
		if (presentation_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentation_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D vulkan_renderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR surface_capabilities)
{
	if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surface_capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D new_extent = {};
		new_extent.width = static_cast<uint32_t>(width);
		new_extent.height = static_cast<uint32_t>(height);

		new_extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, new_extent.width));
		new_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, new_extent.height));
	
		return new_extent;
	}
}


VkImageView vulkan_renderer::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags flags)
{
	VkImageViewCreateInfo imageview_create_info = {};
	imageview_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageview_create_info.image = image;
	imageview_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageview_create_info.format = format;
	imageview_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageview_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageview_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageview_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	imageview_create_info.subresourceRange.aspectMask = flags;
	imageview_create_info.subresourceRange.baseMipLevel = 0;
	imageview_create_info.subresourceRange.levelCount = 1;
	imageview_create_info.subresourceRange.baseArrayLayer = 0;
	imageview_create_info.subresourceRange.layerCount = 0;

	VkImageView image_view;
	VkResult result = vkCreateImageView(main_device.logical_device, &imageview_create_info, nullptr, &image_view);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Image view");
	}
	else
	{
		printf("Image view creation is  a success \n");
	}

	return image_view;
}


VkShaderModule vulkan_renderer::create_shader_module(const std::vector<char> code)
{
	VkShaderModuleCreateInfo shader_module_info = {};
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.codeSize = code.size();
	shader_module_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(main_device.logical_device, &shader_module_info, nullptr, &shader_module);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a shader module");
	}
	else
	{
		printf("Shader module creation is  a success \n");
	}

	return shader_module;
}
