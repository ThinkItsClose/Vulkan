#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	// Check to see if the queue family is complete
	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class Renderer {
public:
	Renderer();
	~Renderer();

private:
	GLFWwindow* _window;

	VkInstance _instance = nullptr;
	VkSurfaceKHR _surface = nullptr;
	VkPhysicalDevice _physicalDevice = nullptr;
	VkDevice _device = nullptr;
	VkDebugUtilsMessengerEXT _DebugMessanger;
	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

	std::vector<const char*> _requestedLayers;

	void _InitWindow();
	void _InitInstance();
	
	void _CreateSurface();


	bool _CheckValidationLayerSupport();
	std::vector<const char*> _GetRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL _DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);
	void _InitDebugMessanger();

	void _InitPhysicalDevice();
	int _RatePhysicalDevice(VkPhysicalDevice device);

	void _InitDevice();

	QueueFamilyIndices _FindQueueFamilys(VkPhysicalDevice device);

	void _MainLoop();

};

