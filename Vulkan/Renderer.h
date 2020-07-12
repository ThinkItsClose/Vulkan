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
	VkDebugUtilsMessengerEXT _DebugMessanger;

	std::vector<const char*> _requestedLayers;

	void _InitWindow();
	void _InitInstance();
	void _InitDevice();

	void _MainLoop();

	void _DeconstructWindow();
	void _DeconstructInstance();
	void _DeconstructDevice();

	// For validation layers and debugging
	bool _CheckValidationLayerSupport();
	std::vector<const char*> _GetRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL _DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);
	void _InitDebugMessanger();
	void _DeconstructDebugMessanger();

};

