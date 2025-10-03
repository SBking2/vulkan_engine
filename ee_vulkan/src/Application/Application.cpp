#include "Application.h"
#include "Vulkan/VulkanContext.h"

namespace ev
{
	void Application::init(uint32_t width, uint32_t height)
	{
		//GLFW窗口初始化
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//阻止自动创建OpenGL上下文
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		//禁止窗口可变化
		m_window = glfwCreateWindow(width, height, "EE_Vulkan", nullptr, nullptr);

		VulkanContext::get_instace()->init(width, height, m_window);
	}

	void Application::run()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			VulkanContext::get_instace()->draw_frame();
		}
	}

	void Application::clear()
	{
		VulkanContext::get_instace()->clear();

		//清理GLFW
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}
}