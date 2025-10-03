#include "Window.h"
#include "Vulkan/VulkanContext.h"
namespace ev
{
	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
	{
		int width2, height2;
		glfwGetFramebufferSize(window, &width2, &height2);
		VulkanContext::get_instace()->wanna_recreate_swapchain();
	}

	void Window::init(uint32_t width, uint32_t height, const std::string& title)
	{
		//GLFW窗口初始化
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//阻止自动创建OpenGL上下文
		m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
	}

	GLFWwindow* Window::get_window()
	{
		return m_window;
	}

	void Window::clear()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Window::handle_event()
	{
		glfwPollEvents();
	}

	bool Window::should_close()
	{
		return glfwWindowShouldClose(m_window);
	}
}