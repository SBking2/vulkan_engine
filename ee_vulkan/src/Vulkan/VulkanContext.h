#pragma once
#include "Singleton/Singleton.h"

#define GLFW_INCLUED_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <vector>
#include <string>

namespace ev
{
	class VulkanContext : public Singleton<VulkanContext>
	{
	public:
		void init(GLFWwindow* window);
		void draw_frame();
		void clear();
		void wanna_recreate_swapchain() { m_is_wanna_recreate_swapchain = true; }
	private:
		void create_instance();
		void create_debug();
		void create_surface(GLFWwindow* window);
		void pick_physical_device();
		void create_logical_device();	//创建逻辑设备
		void create_swapchain();
		void create_img_views();
		void create_renderpass();
		void create_graphic_piple();
		void create_framebuffer();
		void create_command_pool();
		void create_vertex_buffer();
		void create_command_buffer();
		void create_semaphore();
	private:
		void recreate_swapchain();
	private:
		bool check_device(VkPhysicalDevice device, int* score
			, int* graphic_queue_index, int* present_queue_index
			, VkSurfaceCapabilitiesKHR& capabilities
			, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& modes
		);
		void create_buffer(VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties
			, VkBuffer& buffer, VkDeviceMemory& memory);
		void read_shader(const std::string& path, std::vector<char>& source);
		VkShaderModule create_shader_module(const std::vector<char>& source);
		uint32_t find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties);
		void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
	private:
		GLFWwindow* m_window;
		bool m_is_wanna_recreate_swapchain;

		struct PhysicalDevice
		{
		public:
			VkPhysicalDevice device = VK_NULL_HANDLE;

			//显卡具有相应功能的队列簇索引
			int graphic_queue_index = -1;
			int present_queue_index = -1;

			//显卡支持的swapchain设置
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> present_modes;
		};

		struct Vertex
		{
			glm::vec2 pos;
			glm::vec3 color;
		};

		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		VkInstance m_vk_instace;

		VkDebugUtilsMessengerEXT m_callback;	//存储回调函数信息

		VkSurfaceKHR m_surface;

		PhysicalDevice m_physical_device;

		VkDevice m_logical_device;
		VkQueue m_graphic_queue;
		VkQueue m_present_queue;

		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchain_format;
		VkExtent2D m_swapchain_extent;
		std::vector<VkImage> m_swapchain_imgs;

		std::vector<VkImageView> m_swapchain_imgviews;

		VkRenderPass m_renderpass;

		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_graphic_pipeline;

		std::vector<VkFramebuffer> m_swapchain_framebuffers;

		VkCommandPool m_command_pool;

		VkBuffer m_vertex_buffer;
		VkDeviceMemory m_vertex_buffer_memory;
		VkBuffer m_index_buffer;
		VkDeviceMemory m_index_buffer_memory;

		std::vector<VkCommandBuffer> m_command_buffers;

		VkSemaphore m_img_avaliable_semaphore;
		VkSemaphore m_render_finish_semaphore;
		VkFence m_inflight_fence;
	};
}