#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class Renderer {
public:
	Renderer();
	~Renderer();

private:
	GLFWwindow* window;

	VkInstance _instance = nullptr;
	VkDevice _device = nullptr;
	VkPhysicalDevice _gpu = nullptr;

	std::vector<const char*> _requestedLayers;

	void _InitWindow();
	void _InitInstance();
	void _InitDevice();

	void _MainLoop();

	void _DeconstructWindow();
	void _DeconstructInstance();
	void _DeconstructDevice();

	bool _CheckValidationLayerSupport();
};

