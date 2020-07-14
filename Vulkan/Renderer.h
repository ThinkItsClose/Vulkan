#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <set>

// Struct containting the indices of the queue families
struct QueueFamilyIndices {

	// Optional members containing the indices
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
	// Pointer to the window and window width/height
	GLFWwindow* _window;
	uint32_t _width = 800;
	uint32_t _height = 600;

	// Requested layers for debugging
	std::vector<const char*> _requestedLayers = { "VK_LAYER_KHRONOS_validation" };
	const bool _enableDebug = true;
	
	// All vulkan member attributes
	VkInstance _instance = nullptr;
	VkDebugUtilsMessengerEXT _DebugMessanger = nullptr;
	VkSurfaceKHR _surface = nullptr;
	VkPhysicalDevice _physicalDevice = nullptr;
	VkDevice _device = nullptr;
	VkQueue _graphicsQueue = nullptr;
	VkQueue _presentQueue = nullptr;

	// Initialisation of Vulkan
	void _InitWindow();
	void _InitInstance();
	void _InitDebugMessanger();
	void _CreateSurface();
	void _InitPhysicalDevice();
	void _InitDevice();
	
	// Debugging, instance and device creation
	std::vector<const char*> _GetRequiredExtensions();
	bool _CheckValidationLayerSupport();
	int _RatePhysicalDevice(VkPhysicalDevice device);
	QueueFamilyIndices _FindQueueFamilys(VkPhysicalDevice device);

	// Post initialisation
	void _MainLoop();
};

