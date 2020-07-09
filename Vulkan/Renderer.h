#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

class Renderer {
public:
	Renderer();
	~Renderer();

private:
	VkInstance _instance = nullptr;
	VkDevice _device = nullptr;
	VkPhysicalDevice _gpu = nullptr;

	uint32_t _graphics_family_index = 0;

	void _InitInstance();
	void _DeconstructInstance();

	void _InitDevice();
	void _DeconstructDevice();
};

