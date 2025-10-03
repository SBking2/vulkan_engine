#include "Application.h"
#include "Vulkan/VulkanContext.h"
#include "Window/Window.h"
namespace ev
{

	void Application::init()
	{
		Window::get_instace()->init(800, 600, "vulkan_example");
		VulkanContext::get_instace()->init(Window::get_instace()->get_window());
	}

	void Application::run()
	{
		while (!Window::get_instace()->should_close())
		{
			Window::get_instace()->handle_event();
			VulkanContext::get_instace()->draw_frame();
		}
	}

	void Application::clear()
	{
		VulkanContext::get_instace()->clear();
		Window::get_instace()->clear();
	}
}