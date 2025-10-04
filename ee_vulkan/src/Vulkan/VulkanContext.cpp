#include "VulkanContext.h"
#include <GLM/glm.hpp>
#include <map>
#include <iostream>
#include <fstream>
#include <set>
#include <array>
namespace ev
{
	/// <summary>
	/// ��֤�����Ϣ�ص�
	/// </summary>
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		std::cerr << "validation layer : " << pCallbackData->pMessage << std::endl;
		std::cerr << std::endl;
		return VK_FALSE;
	}

	void VulkanContext::init(GLFWwindow* window)
	{
		m_window = window;

		create_instance();
		create_debug();
		create_surface(m_window);
		pick_physical_device();
		create_logical_device();
		create_swapchain();
		create_img_views();
		create_renderpass();
		create_graphic_piple();
		create_framebuffer();
		create_command_pool();
		create_vertex_buffer();
		create_command_buffer();
		create_semaphore();
	}

	void VulkanContext::clear()
	{
		vkDestroySemaphore(m_logical_device, m_img_avaliable_semaphore, nullptr);
		vkDestroySemaphore(m_logical_device, m_render_finish_semaphore, nullptr);
		vkDestroyFence(m_logical_device, m_inflight_fence, nullptr);

		vkDestroyBuffer(m_logical_device, m_vertex_buffer, nullptr);
		vkFreeMemory(m_logical_device, m_vertex_buffer_memory, nullptr);
		vkDestroyBuffer(m_logical_device, m_index_buffer, nullptr);
		vkFreeMemory(m_logical_device, m_index_buffer_memory, nullptr);

		for (auto& view : m_swapchain_imgviews)
		{
			vkDestroyImageView(m_logical_device, view, nullptr);
		}

		for (auto& framebuffer : m_swapchain_framebuffers)
		{
			vkDestroyFramebuffer(m_logical_device, framebuffer, nullptr);
		}

		vkDestroyCommandPool(m_logical_device, m_command_pool, nullptr);

		//����Vulkan
		vkDestroyRenderPass(m_logical_device, m_renderpass, nullptr);
		vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
		vkDestroyPipeline(m_logical_device, m_graphic_pipeline, nullptr);
		vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
		vkDestroyDevice(m_logical_device, nullptr);
		vkDestroySurfaceKHR(m_vk_instace, m_surface, nullptr);

		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vk_instace, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(m_vk_instace, m_callback, nullptr);
		}
		else
		{
			throw std::runtime_error("failed to upset debug!");
		}

		vkDestroyInstance(m_vk_instace, nullptr);

	}

	/// <summary>
	/// 1.ȡͼƬ���첽��
	/// 2.�ύ����첽��
	/// 3.���֣��첽��
	/// ����֮��Ҫͬ��˳��
	/// </summary>
	void VulkanContext::draw_frame()
	{
		//�ȴ�fence��Ϊsignaled
		//vkWaitForFences(m_logical_device, 1, &m_inflight_fence, VK_TRUE
		//	, std::numeric_limits<uint64_t>::max());
		//vkResetFences(m_logical_device, 1, &m_inflight_fence);	//��fence��Ϊunsignaled

		//�ӽ�������ȡһ��ͼ��
		uint32_t img_index;
		auto result = vkAcquireNextImageKHR(m_logical_device, m_swapchain, std::numeric_limits<uint64_t>::max(),
			m_img_avaliable_semaphore, VK_NULL_HANDLE, &img_index);

		//��KHR�޷�ʹ�û��߲�ƥ���ʱ��ֱ���ؽ������������˳���ǰ�Ļ���
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_is_wanna_recreate_swapchain)
		{
			m_is_wanna_recreate_swapchain = false;
			printf("**********************recreate_swapchain!\n");
			recreate_swapchain();
			return;
		}

		//ִ��ָ��
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_semaphores[] = { m_img_avaliable_semaphore };
		VkPipelineStageFlags wait_stages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &m_command_buffers[img_index];

		//ָ����������źţ��൱���ź���+1��
		VkSemaphore signalSemaphores[] = { m_render_finish_semaphore };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signalSemaphores;

		//����ָ����Ҫһ��fence,Ҫ��fence��unsignaled״̬
		if (vkQueueSubmit(m_graphic_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to queue submit!");
		}

		//������Ⱦ���ͼ�񵽽��������г���
		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swap_chains[] = { m_swapchain };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swap_chains;
		present_info.pImageIndices = &img_index;

		vkQueuePresentKHR(m_present_queue, &present_info);
		vkQueueWaitIdle(m_present_queue);
	}

	void VulkanContext::recreate_swapchain()
	{
		int width = 0, height = 0;

		//��С������ʱ�������swapchain
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_logical_device);		//�ȴ�ִ�н���

		vkDestroySemaphore(m_logical_device, m_img_avaliable_semaphore, nullptr);
		vkDestroySemaphore(m_logical_device, m_render_finish_semaphore, nullptr);
		vkDestroyFence(m_logical_device, m_inflight_fence, nullptr);

		for (size_t i = 0; i < m_swapchain_framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_logical_device, m_swapchain_framebuffers[i], nullptr);
		}
		vkFreeCommandBuffers(m_logical_device, m_command_pool, static_cast<uint32_t>(m_command_buffers.size())
			, m_command_buffers.data());

		vkDestroyPipeline(m_logical_device, m_graphic_pipeline, nullptr);
		vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
		vkDestroyRenderPass(m_logical_device, m_renderpass, nullptr);

		for (size_t i = 0; i < m_swapchain_imgviews.size(); i++)
		{
			vkDestroyImageView(m_logical_device, m_swapchain_imgviews[i], nullptr);
		}

		vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);

		create_swapchain();
		create_img_views();
		create_renderpass();
		create_graphic_piple();
		create_framebuffer();
		create_command_buffer();
		create_semaphore();
	}

	#pragma region step
	
	//vulkanʵ��������չ����֤��ȵ�
	void VulkanContext::create_instance()
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

	//������֤��Ļص�
	void VulkanContext::create_debug()
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

	//ʹ��glfw�����������surface
	void VulkanContext::create_surface(GLFWwindow* window)
	{
		if (glfwCreateWindowSurface(m_vk_instace, window, nullptr, &m_surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create surface KHR");
		}
	}

	void VulkanContext::pick_physical_device()
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

	void VulkanContext::create_logical_device()
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

	//��������������img
	void VulkanContext::create_swapchain()
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
			//���ڴ�С�ı��Ҫ���»�ȡ��extent
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device.device, m_surface, &m_physical_device.capabilities);

			if (m_physical_device.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				choose_extent = m_physical_device.capabilities.currentExtent;
			}
			else
			{
				int width, height;
				glfwGetFramebufferSize(m_window, &width, &height);

				choose_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
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

	//Ϊ��������ͼƬ�������
	void VulkanContext::create_img_views()
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

	void VulkanContext::create_renderpass()
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = m_swapchain_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	//ÿ����Ⱦǰһ֡���֡����
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	//��Ⱦ�����ݻᱻ�洢����

		//��ģ�建�岻���ģ���ʱ��
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		//��ʾ��������Ⱦǰ��ͼ�񲼾�
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//ͼ�������ڽ������г���

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;	//attachment description������
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		//��������������
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	//�ȴ���ɫ��������Ľ׶�
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderpass_create_info = {};
		renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpass_create_info.attachmentCount = 1;
		renderpass_create_info.pAttachments = &color_attachment;
		renderpass_create_info.subpassCount = 1;
		renderpass_create_info.pSubpasses = &subpass;
		renderpass_create_info.dependencyCount = 1;
		renderpass_create_info.pDependencies = &dependency;

		if (vkCreateRenderPass(m_logical_device, &renderpass_create_info, nullptr, &m_renderpass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void VulkanContext::create_graphic_piple()
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

		//���������
		VkVertexInputBindingDescription bind_descriptions = {};
		bind_descriptions.binding = 0;
		bind_descriptions.stride = sizeof(Vertex);
		bind_descriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		//������������
		std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};
		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[0].offset = 0;

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, color);

		//1.��������
		//ָ������������ɫ���ض������ݵĸ�ʽ
		VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
		vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
		vertex_input_state_create_info.pVertexBindingDescriptions = &bind_descriptions;
		vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
		vertex_input_state_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

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
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;		//ָ��˳ʱ��Ķ���˳��Ϊ����

		rasterization_create_info.depthBiasEnable = VK_FALSE;	//�Ƿ�Ƭ�������߶ε�б�ʣ��ŵ����ֵ�ϣ�
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;

		//5.���ز���
		VkPipelineMultisampleStateCreateInfo multi_sample_create_info = {};
		multi_sample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multi_sample_create_info.sampleShadingEnable = VK_FALSE;	//���ö��ز���
		multi_sample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	//����һ�Σ�

		//6.��Ⱥ�ģ�����

		//7.��ɫ���
		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
			| VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;		//��ʱ������ɫ���

		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment;

		//8.��̬״̬
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,		//�ӿڱ任
			VK_DYNAMIC_STATE_LINE_WIDTH		//�߿�
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 0;	//sizeof(dynamicStates) / sizeof(VkDynamicState);	//��ʱ������
		dynamic_create_info.pDynamicStates = dynamicStates;

		//9.���߲���(uniform)
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 0; // Optional
		pipeline_layout_create_info.pSetLayouts = nullptr; // Optional
		pipeline_layout_create_info.pushConstantRangeCount = 0; // Optional
		pipeline_layout_create_info.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(m_logical_device, &pipeline_layout_create_info, nullptr,
			&m_pipeline_layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		//������Ⱦ����
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;	//������ɫ���׶�
		pipeline_create_info.pStages = shaderStages;

		pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pViewportState = &viewport_state_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multi_sample_create_info;
		pipeline_create_info.pDepthStencilState = nullptr; // Optional
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;

		pipeline_create_info.layout = m_pipeline_layout;
		pipeline_create_info.renderPass = m_renderpass;
		pipeline_create_info.subpass = 0;	//�����̵�����

		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipeline_create_info.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(m_logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info
			, nullptr, &m_graphic_pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphic pipeline!");
		}

		//�����˹���֮���������shader module��
		vkDestroyShaderModule(m_logical_device, vertex_shader_module, nullptr);
		vkDestroyShaderModule(m_logical_device, frag_shader_module, nullptr);
	}

	//Ϊ��������ͼƬ����֡����
	void VulkanContext::create_framebuffer()
	{
		m_swapchain_framebuffers.resize(m_swapchain_imgviews.size());
		for (int i = 0; i < m_swapchain_imgviews.size(); i++)
		{
			VkImageView attachments[] = {
				m_swapchain_imgviews[i]
			};

			VkFramebufferCreateInfo framebuffer_create_info = {};
			framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_create_info.renderPass = m_renderpass;
			framebuffer_create_info.attachmentCount = 1;
			framebuffer_create_info.pAttachments = attachments;
			framebuffer_create_info.width = m_swapchain_extent.width;
			framebuffer_create_info.height = m_swapchain_extent.height;
			framebuffer_create_info.layers = 1;

			if (vkCreateFramebuffer(m_logical_device, &framebuffer_create_info, nullptr, &m_swapchain_framebuffers[i])
				!= VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	//��ʼ������Ⱦ����
	void VulkanContext::create_command_pool()
	{
		VkCommandPoolCreateInfo command_pool_create_info = {};
		command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_create_info.queueFamilyIndex = m_physical_device.graphic_queue_index;
		command_pool_create_info.flags = 0;

		if (vkCreateCommandPool(m_logical_device, &command_pool_create_info, nullptr, &m_command_pool)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void VulkanContext::create_vertex_buffer()
	{
		vertices =
		{
			{{-0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
			{{0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
			{{-0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
			{{ 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
		};

		indices =
		{
			0, 1, 2, 1, 3, 2
		};
		
		VkBuffer staging_buffer;
		VkDeviceMemory staging_memory;

		create_buffer(sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	//CPU�ɼ�/CPU��GPU���汣��һ��
			, staging_buffer, staging_memory
		);

		//���ݴ滺�����������
		void* data;
		vkMapMemory(m_logical_device, staging_memory, 0, sizeof(Vertex) * vertices.size(), 0, &data);	//memoryӳ�䵽cpu���Է��ʵ��ڴ���
		memcpy(data, vertices.data(), sizeof(Vertex) * vertices.size());
		vkUnmapMemory(m_logical_device, staging_memory);

		create_buffer(sizeof(Vertex) * vertices.size()
			, (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT		//��������ڴ���GPU�ڲ�
			, m_vertex_buffer, m_vertex_buffer_memory
			);

		//���ݴ�buffer�����ݴ��뵽vertex buffer��
		copy_buffer(staging_buffer, m_vertex_buffer, sizeof(Vertex) * vertices.size());

		vkDestroyBuffer(m_logical_device, staging_buffer, nullptr);
		vkFreeMemory(m_logical_device, staging_memory, nullptr);

		//*********************************************** Index buffer *****************************************************

		create_buffer(sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	//CPU�ɼ�/CPU��GPU���汣��һ��
			, staging_buffer, staging_memory
		);

		//���ݴ滺�����������
		vkMapMemory(m_logical_device, staging_memory, 0, sizeof(uint16_t) * indices.size(), 0, &data);	//memoryӳ�䵽cpu���Է��ʵ��ڴ���
		memcpy(data, indices.data(), sizeof(uint16_t) * indices.size());
		vkUnmapMemory(m_logical_device, staging_memory);

		create_buffer(sizeof(uint16_t) * indices.size()
			, (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT		//��������ڴ���GPU�ڲ�
			, m_index_buffer, m_index_buffer_memory
		);

		//���ݴ�buffer�����ݴ��뵽index buffer��
		copy_buffer(staging_buffer, m_index_buffer, sizeof(uint16_t) * indices.size());

		vkDestroyBuffer(m_logical_device, staging_buffer, nullptr);
		vkFreeMemory(m_logical_device, staging_memory, nullptr);
	}

	void VulkanContext::create_command_buffer()
	{
		m_command_buffers.resize(m_swapchain_framebuffers.size());

		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = m_command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = (uint32_t)m_command_buffers.size();

		if (vkAllocateCommandBuffers(m_logical_device, &alloc_info, m_command_buffers.data())
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command buffers!");
		}

		for (size_t i = 0; i < m_command_buffers.size(); i++)
		{
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			begin_info.pInheritanceInfo = nullptr;

			//��ʼ¼�Ʋ���
			if (vkBeginCommandBuffer(m_command_buffers[i], &begin_info) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin command buffers!");
			}

			VkRenderPassBeginInfo renderpass_info = {};
			renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpass_info.renderPass = m_renderpass;
			renderpass_info.framebuffer = m_swapchain_framebuffers[i];
			renderpass_info.renderArea.offset = { 0, 0 };
			renderpass_info.renderArea.extent = m_swapchain_extent;

			VkClearValue clear_color = { 0.1f, 0.1f, 0.1f, 1.0f };
			renderpass_info.clearValueCount = 1;
			renderpass_info.pClearValues = &clear_color;

			//��ʼrenderpass
			vkCmdBeginRenderPass(m_command_buffers[i], &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

			//����Ⱦ����
			vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphic_pipeline);

			VkBuffer vertex_buffers[] = { m_vertex_buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_command_buffers[i], 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(m_command_buffers[i], m_index_buffer, 0, VK_INDEX_TYPE_UINT16);

			//����
			vkCmdDrawIndexed(m_command_buffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			//����renderpass
			vkCmdEndRenderPass(m_command_buffers[i]);

			//������¼ָ��
			if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to end command buffers!");
			}
		}

	}

	//����GPU������ͬ��
	void VulkanContext::create_semaphore()
	{
		VkSemaphoreCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_create_info = {};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(m_logical_device, &create_info, nullptr, &m_img_avaliable_semaphore) != VK_SUCCESS
			|| vkCreateSemaphore(m_logical_device, &create_info, nullptr, &m_render_finish_semaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphore!");
		}

		if (vkCreateFence(m_logical_device, &fence_create_info, nullptr, &m_inflight_fence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create fence!");
		}
	}

	#pragma endregion

	uint32_t VulkanContext::find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(m_physical_device.device, &mem_properties);
		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
	}

	/// <summary>
	/// ��������Կ��Ƿ�ϸ�
	/// </summary>
	bool VulkanContext::check_device(VkPhysicalDevice device, int* score
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

	void VulkanContext::create_buffer(VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties
		, VkBuffer& buffer, VkDeviceMemory& memory)
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = size;
		buffer_create_info.usage = usage;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(m_logical_device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		//�ڴ�
		VkMemoryRequirements mem_requirment;
		vkGetBufferMemoryRequirements(m_logical_device, buffer, &mem_requirment);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirment.size;
		alloc_info.memoryTypeIndex = find_memory_type(mem_requirment.memoryTypeBits
			, properties
			);
		
		if (vkAllocateMemory(m_logical_device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate memory!");
		}

		vkBindBufferMemory(m_logical_device, buffer, memory, 0);
	}

	void VulkanContext::read_shader(const std::string& path, std::vector<char>& source)
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

	VkShaderModule VulkanContext::create_shader_module(const std::vector<char>& source)
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

	//ʹ������ش�������
	void VulkanContext::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
	{
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = m_command_pool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(m_logical_device, &alloc_info, &command_buffer);

		//��ʼ��¼ָ��
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(command_buffer, &begin_info);
		VkBufferCopy copy_region = {};
		copy_region.srcOffset = 0;
		copy_region.dstOffset = 0;
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
		vkEndCommandBuffer(command_buffer);

		//�ύָ��
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(m_graphic_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_graphic_queue);

		vkFreeCommandBuffers(m_logical_device, m_command_pool, 1, &command_buffer);
	}
}