#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <fstream>
#include <chrono>
#include <numeric>

// Maths headers
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
	VkDescriptorSetLayout _descriptorSetLayout;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;

	// Framebuffer members
	std::vector<VkFramebuffer> _framebuffers;

	// Commandbuffer stuff
	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;
	
	// For vertex buffers 
	VkDeviceMemory _vertexBufferMemory;
	VkBuffer _vertexBuffer;
	VkDeviceMemory _indexBufferMemory;
	VkBuffer _indexBuffer;
	
	// For UBO's and stuff
	std::vector<VkBuffer> _uniformBuffers;
	std::vector<VkDeviceMemory> _uniformBuffersMemory;
	VkDescriptorPool _descriptorPool;
	std::vector<VkDescriptorSet> _descriptorSets;

	// For syncronising and having frames in flight
	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;
	std::vector<VkFence> _imagesInFlight;
	const int _max_frames_in_flight = 2;
	size_t _currentFrame = 0;

	// This is used to handle when the window has been resized
	bool _framebufferResize = false;
	static void _WindowResized(GLFWwindow*, int, int);

	// Test vertices for now
	const std::vector<Vertex> _vertices = { {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
	const std::vector<uint16_t> _indices = { 0, 1, 2, 2, 3, 0 }; // uint16_t since using less than 65535 vertices

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
	QueueFamilyIndices _FindQueueFamilies(VkPhysicalDevice);
	PhysicalDeviceSurface _GetSwapChainCapabilities(VkPhysicalDevice);

	// Swapchain initialisation methods
	void _InitSwapChain();
	void _DeconstructSwapChain();
	void _RecreateSwapChain();
	VkSurfaceFormatKHR _GetSurfaceFormat(std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR _GetPresentMode(std::vector<VkPresentModeKHR>&);
	VkExtent2D _GetSwapExtent(VkSurfaceCapabilitiesKHR&);

	// Descriptor sets are analogous to uniforms in opengl. I think
	void _CreateDescriptorSetLayout();

	// Graphics pipeline
	void _CreateRenderPass();
	void _CreateGraphicsPipeline();
	VkShaderModule _GetShaderModule(const std::vector<char>&);

	// Framebuffer creation methods
	void _CreateFramebuffers();
	
	// Vertex buffers and helper functions
	uint32_t _FindMemoryType(uint32_t, VkMemoryPropertyFlags);
	void _CreateVertexBuffer();
	void _CreateIndexBuffer();
	void _CopyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	void _CreateBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void _CreateUniformBuffers();
	void _CreateDescriptorPool();
	void _CreateDescriptorSets();

	// Commandbuffer stuff
	void _CreateCommandPool();
	void _CreateCommandBuffers();

	// Setup semaphores
	void _CreateSyncObjects();

	// Post initialisation
	void _MainLoop();
	void _DrawFrame();

	// For updating shader uniforms
	void _UpdateUniformBuffer(uint32_t);
};

