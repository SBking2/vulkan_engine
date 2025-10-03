#pragma once
#define GLFW_INCLUED_VULKAN

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
namespace ev
{
	class Application
	{
	public:
		void init(uint32_t width, uint32_t height);
		void run();
		void clear();
	private:
		GLFWwindow* m_window;
	};
}