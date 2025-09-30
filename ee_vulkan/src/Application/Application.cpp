#include "Application.h"

#include <map>
#include <iostream>
#include <fstream>
#include <set>
namespace ev
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		std::cerr << "validation layer : " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void Application::init(uint32_t width, uint32_t height)
	{
		//GLFW���ڳ�ʼ��
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//��ֹ�Զ�����OpenGL������
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		//��ֹ���ڿɱ仯

		m_width = width;
		m_height = height;
		m_window = glfwCreateWindow(m_width, m_height, "EE_Vulkan", nullptr, nullptr);

		vulkan_init();
		setup_debug();
		create_surface();
		pick_physical_device();
		create_logical_device();
		create_swapchain();
		create_img_views();
	}

	void Application::run()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	void Application::clear()
	{
		destroy_debug();	//���ٻص�����

		for (auto& view : m_swapchain_imgviews)
		{
			vkDestroyImageView(m_logical_device, view, nullptr);
		}

		//����Vulkan
		vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
		vkDestroyDevice(m_logical_device, nullptr);
		vkDestroySurfaceKHR(m_vk_instace, m_surface, nullptr);
		vkDestroyInstance(m_vk_instace, nullptr);

		//����GLFW
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Application::vulkan_init()
	{
		//����vulkan֧�ֵ���չ
		uint32_t supported_extension_count;
		vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(supported_extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, extensions.data());

		std::cout << "***************** Vulkan Supported Extension *********************" << std::endl;

		for (const auto& extension : extensions)
		{
			std::cout << "EE: Vulkan Supported Extension : " << extension.extensionName << std::endl;
		}

		std::cout << std::endl;

		//����vulkan֧�ֵ�layer
		uint32_t supported_layer_count;
		vkEnumerateInstanceLayerProperties(&supported_layer_count, nullptr);
		std::vector<VkLayerProperties> layers(supported_layer_count);
		vkEnumerateInstanceLayerProperties(&supported_layer_count, layers.data());

		std::cout << "***************** Vulkan Supported Layer *********************" << std::endl;

		for (int i = 0; i < supported_layer_count; i++)
		{
			std::cout << "EE: Vulkan Supported Layers : " << layers[i].layerName << std::endl;
		}

		std::cout << std::endl;

		//��ӡglfw��Ҫ����չ
		uint32_t glfw_extension_count = 0;
		const char** gl_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::cout << "***************** GLFW Required Extension *********************" << std::endl;

		for (int i = 0; i < glfw_extension_count; i++)
		{
			std::cout << "EE: GLFW Required Extension : " << gl_extensions[i] << std::endl;
		}

		std::cout << std::endl;

		//��Ҫ�����Ĳ�
		std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		//��Ҫ��������չ
		std::vector<const char*> required_extensions;
		for (int i = 0; i < glfw_extension_count; i++)
		{
			required_extensions.push_back(gl_extensions[i]);
		}

		required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);	//��Ϣ�ص���Ҫ����չ

		//����Instance
		VkApplicationInfo app_info = {};	//�������������ֵ
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Hello_Triangle";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No_Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		//��glfw��Ҫ����չ
		create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
		create_info.ppEnabledExtensionNames = required_extensions.data();
		//����У���
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		VkResult result = vkCreateInstance(&create_info, nullptr, &m_vk_instace);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create vulkan instance");
		}
	}

	void Application::pick_physical_device()
	{
		uint32_t device_count;
		vkEnumeratePhysicalDevices(m_vk_instace, &device_count, nullptr);
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(m_vk_instace, &device_count, devices.data());

		std::multimap<int, PhysicalDevice> candidates;

		std::cout << "***************** Physical Device *********************" << std::endl;

		//���ϸ���Կ�,����map��
		for (const VkPhysicalDevice& device : devices)
		{
			PhysicalDevice phy_deivce;
			phy_deivce.device = device;

			int score;

			if (check_device(device, &score, &phy_deivce.graphic_queue_index, &phy_deivce.present_queue_index
				, phy_deivce.capabilities
				, phy_deivce.formats, phy_deivce.present_modes))
			{
				candidates.insert(std::make_pair(score, phy_deivce));
			}

			VkPhysicalDeviceProperties property;
			vkGetPhysicalDeviceProperties(device, &property);

			std::cout << "EE: Vulkan Physical Device : " << property.deviceName << std::endl
				<< "score : " << score << std::endl
				<< "graphic queue index : " << phy_deivce.graphic_queue_index << std::endl
				<< "present queue index : " << phy_deivce.present_queue_index << std::endl
				<< "Surface Capabilities : " 
				<< " Width: " << phy_deivce.capabilities.currentExtent.width
				<< " Height: " << phy_deivce.capabilities.currentExtent.height
				<< std::endl
				<< "Surface Format : " << std::endl;

			for (const auto& format : phy_deivce.formats)
			{
				std::cout << "	Format : " << format.format << "  "
				<< "	Color Space : " << format.colorSpace << std::endl;
			}

			std::cout << "Surface Mode : " << std::endl;

			for (const auto& mode : phy_deivce.present_modes)
			{
				std::cout << "	Mode : " << mode << std::endl;
			}

			std::cout << std::endl;
		}


		bool is_find = false;

		//���ҷ�������Ҿ���ͼ���ܵ��Կ�
		for (auto it = candidates.rbegin(); it != candidates.rend(); it++)
		{
			if (it->first > 0 && it->second.graphic_queue_index != -1 && it->second.present_queue_index != -1)
			{
				m_physical_device = it->second;

				VkPhysicalDeviceProperties property;
				vkGetPhysicalDeviceProperties(m_physical_device.device, &property);
				std::cout << "EE: Vulkan Pick Physical Device : " << property.deviceName
					<< " |||| Graphic Queue Index : " << m_physical_device.graphic_queue_index 
					<< " |||| Present Queue Index : " << m_physical_device.present_queue_index
					<< std::endl;
				is_find = true;
				break;
			}
		}

		if (!is_find)
		{
			throw std::runtime_error("failed to pick physical device!");
		}
	}

	bool Application::check_device(VkPhysicalDevice device, int* score
		, int* graphic_queue_index, int* present_queue_index
		, VkSurfaceCapabilitiesKHR& capabilities
		, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& modes
	)
	{
		#pragma region �����Կ��ķ���

		{
			//��ȡProperty
			VkPhysicalDeviceProperties property;
			vkGetPhysicalDeviceProperties(device, &property);

			VkPhysicalDeviceFeatures feature;
			vkGetPhysicalDeviceFeatures(device, &feature);

			//���㵱ǰ�Կ��ķ���
			*score = 0;
			if (property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				*score += 1000;
			*score += property.limits.maxImageDimension2D;
			if (!feature.geometryShader)
				*score = 0;

			if (*score == 0)
				return false;
		}

		#pragma endregion

		#pragma region �����д��Ƿ���ͼ�����ʾ����

		{
			//��Ҫ�������Կ���Graphic�Ĺ���
			uint32_t queue_family_count;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
			std::vector<VkQueueFamilyProperties> properties(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, properties.data());

			for (int i = 0; i < properties.size(); i++)
			{
				if (*graphic_queue_index == -1 && properties[i].queueCount > 0
					&& properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					*graphic_queue_index = i;		//��¼�¾���Graphic���ܵĶ��д�
				}

				if (*present_queue_index == -1)
				{
					VkBool32 is_present_supported = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &is_present_supported);
					if (is_present_supported)
						*present_queue_index = i;
				}

				if (*graphic_queue_index != -1 && *present_queue_index != -1)
					break;
			}

			if (*graphic_queue_index == -1 || *present_queue_index == -1)
				return false;
		}

		#pragma endregion

		#pragma region ����Ƿ�֧��SwapChain��չ

		{
			uint32_t extension_count;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
			std::vector<VkExtensionProperties> properties(extension_count);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, properties.data());
			
			bool is_supported_swapchain = false;

			for (const auto& extension : properties)
			{
				if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
				{
					is_supported_swapchain = true;
					break;
				}
			}

			if (!is_supported_swapchain) return false;
		}

		#pragma endregion

		#pragma region ����Կ���Swapchain�Ƿ���Surface����

		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &capabilities);

			uint32_t format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
			formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, formats.data());

			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
			modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, modes.data());

			if (formats.empty() || modes.empty())
				return false;
		}

		#pragma endregion
		
		return true;
	}

	void Application::create_logical_device()
	{
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		//ʹ��set�Ļ���Ԫ�����ݲ����ظ�
		std::set<int> queue_indices = { m_physical_device.graphic_queue_index, m_physical_device.present_queue_index };

		//�߼��豸��Ҫ���Ķ��д�
		float queue_priority = 1.0f;		//�������ȼ�
		for (const auto& index : queue_indices)
		{
			VkDeviceQueueCreateInfo queue_create_info = {};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = index;
			queue_create_info.queueCount = 1;	//��ʱֻʹ�ô�ͼ�����Ķ���
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		//�߼��豸��Ҫ������
		VkPhysicalDeviceFeatures features = {};

		std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> extensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		//�����߼��豸
		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		create_info.pEnabledFeatures = &features;
		//���ý�������չ
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();
		//ͬ��������֤��
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		VkResult result = vkCreateDevice(m_physical_device.device, &create_info, nullptr,
			&m_logical_device);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device");
		}

		//��ȡ���о��
		vkGetDeviceQueue(m_logical_device, m_physical_device.graphic_queue_index, 0, &m_graphic_queue);
		vkGetDeviceQueue(m_logical_device, m_physical_device.present_queue_index, 0, &m_present_queue);
	}

	void Application::create_surface()
	{
		if (glfwCreateWindowSurface(m_vk_instace, m_window, nullptr, &m_surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create surface KHR");
		}
	}

	void Application::setup_debug()
	{
		//����vulkan����Ϣ�ص�
		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		//ָ���ص��ᴦ�����Ϣ����
		create_info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		//ָ���ص��ᴦ�����Ϣ����
		create_info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = debug_callback;
		create_info.pUserData = nullptr;

		//Ѱ�Һ�����ַ
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vk_instace, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(m_vk_instace, &create_info, nullptr, &m_callback);
		}
		else
		{
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	void Application::create_swapchain()
	{
		//���ȵ�һ���£����Ǵ������Կ�֧�ֵ�swapchain�����У��������ʵ�
		VkExtent2D choose_extent;
		VkSurfaceFormatKHR choose_format;
		VkPresentModeKHR choose_present_mode;

		//ѡ��format
		{
			if (m_physical_device.formats.size() == 1 && m_physical_device.formats[0].format == VK_FORMAT_UNDEFINED)
			{
				choose_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}

			bool is_have_format = false;
			for (const auto& format : m_physical_device.formats)
			{
				if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					choose_format = format;
					is_have_format = true;
					break;
				}
			}

			if (!is_have_format) choose_format = m_physical_device.formats[0];
		}

		//ѡ��mode
		{
			bool is_have_mode = false;
			for (const auto& mode : m_physical_device.present_modes)
			{
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					choose_present_mode = mode;
					is_have_mode = true;
					break;
				}
			}
			if (!is_have_mode) choose_present_mode = VK_PRESENT_MODE_FIFO_KHR;	//�����豸��֧�����
		}

		//ѡ��extent
		{
			if (m_physical_device.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				choose_extent = m_physical_device.capabilities.currentExtent;
			}
			else
			{
				choose_extent = { m_width, m_height };
				choose_extent.width = std::max(m_physical_device.capabilities.minImageExtent.width
					, std::min(m_physical_device.capabilities.maxImageExtent.width, choose_extent.width));

				choose_extent.height = std::max(m_physical_device.capabilities.minImageExtent.height
					, std::min(m_physical_device.capabilities.maxImageExtent.height, choose_extent.height));
			}
		}

		uint32_t img_count = m_physical_device.capabilities.minImageCount + 1;
		if (m_physical_device.capabilities.maxImageCount > 0 &&
			img_count > m_physical_device.capabilities.maxImageCount)
			img_count = m_physical_device.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = m_surface;
		create_info.minImageCount = img_count;
		create_info.imageFormat = choose_format.format;
		create_info.imageColorSpace = choose_format.colorSpace;
		create_info.imageExtent = choose_extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { m_physical_device.graphic_queue_index, m_physical_device.present_queue_index };

		if (m_physical_device.graphic_queue_index != m_physical_device.present_queue_index)
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}
		create_info.preTransform = m_physical_device.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = choose_present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_logical_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swapchain KHR");
		}

		//��ȡ��������ͼƬ���
		uint32_t swap_chain_img_count;
		vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &swap_chain_img_count, nullptr);
		m_swapchain_imgs.resize(swap_chain_img_count);
		vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &swap_chain_img_count, m_swapchain_imgs.data());
		
		m_swapchain_format = choose_format.format;
		m_swapchain_extent = choose_extent;
	}

	void Application::create_img_views()
	{
		m_swapchain_imgviews.resize(m_swapchain_imgs.size());
		for (int i = 0; i < m_swapchain_imgs.size(); i++)
		{
			VkImageViewCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = m_swapchain_imgs[i];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = m_swapchain_format;

			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_logical_device, &create_info, nullptr, &m_swapchain_imgviews[i])
				!= VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image view!");
			}
		}
	}

	void Application::create_graphic_piple()
	{
		//׼��Shader Module
		std::vector<char> vert_source;
		std::vector<char> frag_source;

		read_shader("src/Shader/GLSL/vert.spv", vert_source);
		read_shader("src/Shader/GLSL/frag.spv", frag_source);

		VkShaderModule vertex_shader_module = create_shader_module(vert_source);
		VkShaderModule frag_shader_module = create_shader_module(frag_source);

		//ָ��shader���ĸ���ɫ���׶�ʹ��
		VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
		vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_create_info.module = vertex_shader_module;
		vert_shader_stage_create_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
		frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_create_info.module = frag_shader_module;
		frag_shader_stage_create_info.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] =
		{
			vert_shader_stage_create_info,
			frag_shader_stage_create_info
		};

		//1.��������
		//ָ������������ɫ���ض������ݵĸ�ʽ
		VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
		vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
		vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
		vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
		vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;

		//2.����װ��
		//�������ļ������͵�ͼԪ
		//�Ƿ����ü���ͼԪ����
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = VK_FALSE;

		//3.�ӿںͲü�
		
		//�����ӿ�
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapchain_extent.width;
		viewport.height = (float)m_swapchain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//���òü�
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapchain_extent;

		VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
		viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state_create_info.viewportCount = 1;
		viewport_state_create_info.pViewports = &viewport;
		viewport_state_create_info.scissorCount = 1;
		viewport_state_create_info.pScissors = &scissor;

		//4.��դ��
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.depthClampEnable = VK_FALSE;		//��������Զ��ƽ����Ķ����������ض�ΪԶ��ƽ��
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;		//�����True�����ֹһ��Ƭ�������֡����
		rasterization_create_info.lineWidth = 1.0f;		//�����True�����ֹһ��Ƭ�������֡����
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;		//�����޳�
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;		//����˳��

		rasterization_create_info.depthBiasEnable = VK_FALSE;	//�Ƿ�Ƭ�������߶ε�б�ʣ��ŵ����ֵ�ϣ�
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;

		//5.���ز���

		vkDestroyShaderModule(m_logical_device, vertex_shader_module, nullptr);
		vkDestroyShaderModule(m_logical_device, frag_shader_module, nullptr);
	}

	void Application::read_shader(const std::string& path, std::vector<char>& source)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);		//ateģʽ�ǿ�ʼʱ�����ļ�β��
		if (!file.is_open())
		{
			throw std::runtime_error("failed to read file : " + path);
		}

		size_t file_size = (size_t)file.tellg();
		source.resize(file_size);
		
		//���ļ�ָ��ָ����ͷ
		file.seekg(0);
		file.read(source.data(), file_size);

		file.close();
	}

	VkShaderModule Application::create_shader_module(const std::vector<char>& source)
	{
		VkShaderModule shader_module;
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = source.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(source.data());
		
		if (vkCreateShaderModule(m_logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module!");
		}

		return shader_module;
	}

	void Application::destroy_debug()
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vk_instace, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(m_vk_instace, m_callback, nullptr);
		}
	}
}