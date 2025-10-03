#pragma once
#define GLFW_INCLUED_VULKAN

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
namespace ev
{
	class Application
	{
	public:
		void init();
		void run();
		void clear();
	};
}