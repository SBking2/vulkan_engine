#pragma once
#include "Singleton/Singleton.h"
#define GLFW_INCLUED_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
namespace ev
{
	class Window : public Singleton<Window>
	{
	public:
		void init(uint32_t width, uint32_t height, const std::string& title);
		GLFWwindow* get_window();
		void clear();
		bool should_close();
		void handle_event();
	private:
		GLFWwindow* m_window;
	};
}