#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <fstream>

// Include structs
#include "Renderer Structs.h"

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

	// Required device extensions
	std::vector<const char*> _requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	
	// All vulkan member attributes
	VkInstance _instance = nullptr;
	VkDebugUtilsMessengerEXT _DebugMessanger = nullptr;
	VkSurfaceKHR _surface = nullptr;
	VkPhysicalDevice _physicalDevice = nullptr;
	VkDevice _device = nullptr;
	VkQueue _graphicsQueue = nullptr;
	VkQueue _presentQueue = nullptr;

	// Swapchain members
	VkSwapchainKHR _swapChain;
	std::vector<VkImage> _swapChainImages;
	VkFormat _swapChainFormat;
	VkExtent2D _swapChainExtent;
	std::vector<VkImageView> _swapChainImageViews;

	// Graphics pipeline memebers
	VkRenderPass _renderPass;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;

	// Framebuffer members
	std::vector<VkFramebuffer> _framebuffers;


	// Initialisation of Vulkan
	void _InitWindow();
	void _InitInstance();
	void _InitDebugMessanger();
	void _CreateSurface();
	void _InitPhysicalDevice();
	void _InitDevice();
	void _CreateImageViews();
	
	// Debugging, instance and device creation
	std::vector<const char*> _GetRequiredExtensions();
	bool _CheckValidationLayerSupport();
	int _RatePhysicalDevice(VkPhysicalDevice);
	bool _CheckDeviceExtensionSupport(VkPhysicalDevice);
	QueueFamilyIndices _FindQueueFamilys(VkPhysicalDevice);
	PhysicalDeviceSurface _GetSwapChainCapabilities(VkPhysicalDevice);

	// Swapchain initialisation methods
	void _InitSwapChain();
	VkSurfaceFormatKHR _GetSurfaceFormat(std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR _GetPresentMode(std::vector<VkPresentModeKHR>&);
	VkExtent2D _GetSwapExtent(VkSurfaceCapabilitiesKHR&);

	// Graphics pipeline
	void _CreateRenderPass();
	void _CreateGraphicsPipeline();
	VkShaderModule _GetShaderModule(const std::vector<char>&);

	// Framebuffer creation methods
	void _CreateFramebuffers();

	// Post initialisation
	void _MainLoop();
};

