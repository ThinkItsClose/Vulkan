#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	// Check to see if the queue family is complete
	bool IsComplete() {
		return graphicsFamily.has_value();
	}
};

class Renderer {
public:
	Renderer();
	~Renderer();

private:
	GLFWwindow* window;

	VkInstance _instance = nullptr;
	VkPhysicalDevice _physicalDevice = nullptr;
	VkDevice _device = nullptr;
	VkDebugUtilsMessengerEXT _DebugMessanger;
	VkQueue _graphicsQueue;

	std::vector<const char*> _requestedLayers;

	void _InitWindow();
	void _InitInstance();

	void _MainLoop();

	void _DeconstructWindow();
	void _DeconstructInstance();

	bool _CheckValidationLayerSupport();
	std::vector<const char*> _GetRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL _DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);
	void _InitDebugMessanger();
	void _DeconstructDebugMessanger();

	void _InitPhysicalDevice();
	int _RatePhysicalDevice(VkPhysicalDevice device);
	void _DeconstructDevice();

	void _InitDevice();

	QueueFamilyIndices _FindQueueFamilys(VkPhysicalDevice device);

};

