#pragma once
#define GLFW_INCLUED_VULKAN

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
namespace ev
{
	class Application
	{
	public:
		void init(uint32_t width, uint32_t height);
		void run();
		void clear();
	private:
		void vulkan_init();
		void pick_physical_device();	//选择物理显卡
		void create_logical_device();	//创建逻辑设备
		void create_surface();
		void setup_debug();
		void destroy_debug();
		void create_swapchain();
		void create_img_views();
		void create_graphic_piple();
	private:
		VkShaderModule create_shader_module(const std::vector<char>& source);
		void read_shader(const std::string& path, std::vector<char>& source);
		bool check_device(VkPhysicalDevice device, int* score
			, int* graphic_queue_index, int* present_queue_index
			, VkSurfaceCapabilitiesKHR& capabilities
			, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& modes
		);
	private:
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
	private:
		GLFWwindow* m_window;
		uint32_t m_width, m_height;

		VkInstance m_vk_instace;
		PhysicalDevice m_physical_device;
		VkDevice m_logical_device;
		VkQueue m_graphic_queue;
		VkQueue m_present_queue;
		VkSurfaceKHR m_surface;
		VkSwapchainKHR m_swapchain;

		std::vector<VkImage> m_swapchain_imgs;
		VkFormat m_swapchain_format;
		VkExtent2D m_swapchain_extent;

		std::vector<VkImageView> m_swapchain_imgviews;

		VkDebugUtilsMessengerEXT m_callback;	//存储回调函数信息
	};
}