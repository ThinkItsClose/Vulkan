#include "Renderer.h"

// For importing images
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

const char* appName = "Vulkan";

Renderer::Renderer() {
	_InitWindow();
	_InitInstance();
	if (_enableDebug) {
		_InitDebugMessanger();
	}
	_CreateSurface();
	_InitPhysicalDevice();
	_InitDevice();

	_InitSwapChain();
	_CreateImageViews();
	_CreateRenderPass();
	_CreateDescriptorSetLayout();

	_CreateGraphicsPipeline();
	_CreateCommandPool();

	_CreateDepthResources();
	_CreateFramebuffers();

	_CreateTextureImage();
	_CreateTextureImageView();
	_CreateTextureSampler();

	_CreateVertexBuffer();
	_CreateIndexBuffer();

	_CreateUniformBuffers();
	_CreateDescriptorPool();
	_CreateDescriptorSets();
	_CreateCommandBuffers();

	_CreateSyncObjects();
	
	_MainLoop();
}

Renderer::~Renderer() {
	_DeconstructSwapChain();

	vkDestroySampler(_device, _textureSampler, nullptr);
	vkDestroyImageView(_device, _textureImageView, nullptr);

	vkDestroyImage(_device, _textureImage, nullptr);
	vkFreeMemory(_device, _textureImageMemory, nullptr);

	// Cleanup the descriptor set layout
	vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);

	// Free the memory of the vertice and index buffers
	vkDestroyBuffer(_device, _indexBuffer, nullptr);
	vkFreeMemory(_device, _indexBufferMemory, nullptr);
	vkDestroyBuffer(_device, _vertexBuffer, nullptr);
	vkFreeMemory(_device, _vertexBufferMemory, nullptr);

	// Cleanup syncronisation objects
	for (size_t i = 0; i < _max_frames_in_flight; i++) {
		vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(_device, _inFlightFences[i], nullptr);
	}

	// Cleanup the command pool
	vkDestroyCommandPool(_device, _commandPool, nullptr);

	// Cleanup the device
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;

	// If the validation layers have been enabled then cleanup the debug messanger
	if (_enableDebug) {
		// Validate the instance exists before accessing it
		if (_instance == nullptr) {
			std::cout << "ERROR::Renderer::Deconstructor::DeconstructDebugMessanger::InstanceIsNullPointer" << std::endl;
			exit(-1);
		}

		// Get a handle to the destroy debug messanger function
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT != nullptr) {
			vkDestroyDebugUtilsMessengerEXT(_instance, _DebugMessanger, nullptr);
		}
		else {
			std::cout << "ERROR::Renderer::Deconstructor::DeconstructDebugMessanger::InvalidFunctionPointer" << std::endl;
			exit(-1);
		}
	}

	// Now cleanup the surface
	vkDestroySurfaceKHR(_instance, _surface, nullptr);

	// Cleanup the Vulkan instance
	vkDestroyInstance(_instance, nullptr);
	_instance = nullptr;

	// Cleaup the window
	glfwDestroyWindow(_window);
	glfwTerminate();
}

// Notify the rest of the program when the framebuffer size has changed
void Renderer::_WindowResized(GLFWwindow* window, int width, int height) {

	// Need to aquire pointer to class as this is a static member
	Renderer* renderer = (Renderer*)glfwGetWindowUserPointer(window);
	renderer->_framebufferResize = true;
}

void Renderer::_InitWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	_window = glfwCreateWindow(_width, _height, appName, nullptr, nullptr);

	// Set the pointer to the current class and set the window size change callback
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, _WindowResized);
}


void Renderer::_InitInstance() {
	// First check to see if validation layers are required and supported
	if (_enableDebug && !_CheckValidationLayerSupport()) {
		std::cout << "ERROR::Renderer::InitInstance::ValidationLayersRequestedButNotSupported" << std::endl;
	}

	VkApplicationInfo application_info = {};
	application_info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.applicationVersion		= VK_MAKE_VERSION(1, 0, 0);
	application_info.pApplicationName		= appName;
	application_info.engineVersion			= VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName			= "Geton Engine";

	// Get a list of all the required extensions
	std::vector<const char*> extensions = _GetRequiredExtensions();
	VkInstanceCreateInfo instance_create_info {};
	instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo			= &application_info;
	instance_create_info.enabledExtensionCount		= static_cast<uint32_t>(extensions.size());
	instance_create_info.ppEnabledExtensionNames	= extensions.data();
	// If validation layers are enabled then add them to the instance
	if (_enableDebug) {
		instance_create_info.enabledLayerCount		= static_cast<uint32_t>(_requestedLayers.size());
		instance_create_info.ppEnabledLayerNames	= _requestedLayers.data();
	} else {
		instance_create_info.enabledLayerCount		= 0;
	}
	
	if (vkCreateInstance(&instance_create_info, nullptr, &_instance) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitInstance::CreateInstance" << std::endl;
		exit(-1);
	}
}

std::vector<const char*> Renderer::_GetRequiredExtensions() {

	// Get the extensions needed to interface with GLFW
	uint32_t glfwCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);

	// Create a vector with all of the extensions
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwCount);

	// If validation layers are enabled add the debug messanger extension
	if (_enableDebug) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

// Debug Callback is not a member of the class right now to keep the header clean
// Private members can be passed through in the (void*) parameter if it is required
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	// For now just output the message
	std::cerr << pCallbackData->pMessage << std::endl << std::endl;

	// Return VK_FALSE here because VK_TRUE throws a runtime error at the error
	return VK_FALSE;
}

void Renderer::_InitDebugMessanger() {
	// Check validation layers have been enabled
	if (!_enableDebug) return;

	VkDebugUtilsMessengerCreateInfoEXT messanger_create_info{};
	messanger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messanger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messanger_create_info.pfnUserCallback = DebugCallback;
	messanger_create_info.pUserData = nullptr; // Optional (void*) parameter to pass custom variables to the callback

	// Since the function to set the 'debug messanger callback' is not loaded the address needs to be looked up
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");

	// Now if the function was found then execute it
	if (vkCreateDebugUtilsMessengerEXT != nullptr) {
		vkCreateDebugUtilsMessengerEXT(_instance, &messanger_create_info, nullptr, &_DebugMessanger);
	}
	else {
		std::cout << "ERROR::Renderer::InitDebugMessanger::InvalidFunctionPointer" << std::endl;
		exit(-1);
	}
}

void Renderer::_CreateSurface() {
	// Create a window surface for the vulkan instance
	if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateSurface::glfwCreateWindowSurface" << std::endl;
		exit(-1);
	}

}

void Renderer::_InitPhysicalDevice() {

	// First of all get the number of physical devices
	uint32_t deviceCount;
	if (vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitDevice::vkEnumeratePhysicalDevices::0" << std::endl;
		exit(-1);
	}

	if (!deviceCount) {
		std::cout << "ERROR::Renderer::InitDevice::NoPhysicalDevicesFound" << std::endl;
		exit(-1);
	}
	
	// Now get a list of all physical devices
	std::vector<VkPhysicalDevice> devices(deviceCount);
	if (vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data()) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitDevice::vkEnumeratePhysicalDevices::1" << std::endl;
		exit(-1);
	}

	// Evaluate the suitability of each device
	int maxRating = 0;
	int rating;
	VkPhysicalDevice currentDevice = nullptr;
	for (VkPhysicalDevice& device : devices) {
		rating = _RatePhysicalDevice(device);
		if (rating > maxRating) {
			currentDevice = device;
			maxRating = rating;
		}
	}

	// If the current device is still a nullptr then no suitable device was found
	if (currentDevice == nullptr) {
		std::cout << "ERROR::Renderer::InitDevice::NoSuitableDevicesFound" << std::endl;
		exit(-1);
	}

	// If it is make it the current physical device
	_physicalDevice = currentDevice;
}

int Renderer::_RatePhysicalDevice(VkPhysicalDevice device) {
	
	// The calls to get properties and features here can be used in the future to ensure that the vulkan device
	// matches the requirements that the programs have, for now the only criteria is that they support certian queue families
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	// Check if the device supports the required extensions
	bool deviceExtensionsSupported = _CheckDeviceExtensionSupport(device);

	// The indices of the required queue families are now in 'indices'
	QueueFamilyIndices indices = _FindQueueFamilies(device);

	// Get the capabilities of the swapchain in the device
	// Only do this if it is known that the device extentions are supported (it contains the swapchain extension)
	bool swapChainGood = false;
	if (deviceExtensionsSupported) {
		// Bare minimum requirements for now
		PhysicalDeviceSurface swapChainSupport = _GetSwapChainCapabilities(device);
		swapChainGood = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.IsComplete() && deviceExtensionsSupported && swapChainGood && features.samplerAnisotropy;
}

// Checks if the device has all the required extensions
bool Renderer::_CheckDeviceExtensionSupport(VkPhysicalDevice device) {

	// Get the number of extensions supported by the device
	uint32_t extensionCount;
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::CheckDeviceExtensionSupport::EnumerateDeviceExtensionProperties::0" << std::endl;
		exit(-1);
	}

	// Get the extensions in a vector
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::CheckDeviceExtensionSupport::EnumerateDeviceExtensionProperties::1" << std::endl;
		exit(-1);
	}

	// Check that all required extensions exist
	uint32_t supportedExtensions = 0;
	for (auto& extension : availableExtensions) {
		for (auto& requestedName : _requiredDeviceExtensions) {
			if (!strcmp(requestedName, extension.extensionName)) {
				supportedExtensions++;
			}
		}
	}

	// If the amount of required extensions and supported extensions are equal return true
	return ((uint32_t)_requiredDeviceExtensions.size() == supportedExtensions);
}

// Returns a struct containing the indices of certain queue families
QueueFamilyIndices Renderer::_FindQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	// Get the number of queue families the device supports 
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	// Now get the queue family properties in an vector
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// Check each queue family for the requirements
	int index = 0;
	for (auto& queueFamily : queueFamilies) {

		// Check that the queue family supports graphics operations
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = index;
		}

		// Check that the queue family supports presentation to a surface
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, _surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = index;
		}

		// If all the criteria have been meet then exit
		if (indices.IsComplete()) {
			break;
		}

		index++;
	}

	return indices;
}

// Function that returns a struct containting details about the swapchain
PhysicalDeviceSurface Renderer::_GetSwapChainCapabilities(VkPhysicalDevice device) {
	PhysicalDeviceSurface details;

	// Get the capabilities
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::GetSwapChainCapabilities::GetPhysicalDeviceSurfaceCapabilitiesKHR" << std::endl;
		exit(-1);
	}

	// Get the formats
	// Get the number of formats
	uint32_t formatCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::GetSwapChainCapabilities::GetPhysicalDeviceSurfaceFormatsKHR::0" << std::endl;
		exit(-1);
	}

	// Populate the struct vector with formats
	if (formatCount) {
		details.formats.resize(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data()) != VK_SUCCESS) {
			std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::GetSwapChainCapabilities::GetPhysicalDeviceSurfaceFormatsKHR::1" << std::endl;
			exit(-1);
		}
	}

	// Get the present modes
	// Get the number of present modes then populate the struct vector
	uint32_t presentModeCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::GetSwapChainCapabilities::GetPhysicalDeviceSurfacePresentModesKHR::0" << std::endl;
		exit(-1);
	}

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data()) != VK_SUCCESS) {
			std::cout << "ERROR::Renderer::InitPhysicalDevice::RatePhysicalDevice::GetSwapChainCapabilities::GetPhysicalDeviceSurfacePresentModesKHR::0" << std::endl;
			exit(-1);
		}
	}


	return details;
}


void Renderer::_InitDevice() {
	// Initalisation of the logical device
	// Get the families supported by the device
	QueueFamilyIndices indices = _FindQueueFamilies(_physicalDevice);

	// Create a unique list of the queue family's indices
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	// Create a vector of structs to add to the device create info struct
	float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo device_queue_create_info{};
		device_queue_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_info.queueFamilyIndex	= queueFamily;
		device_queue_create_info.queueCount			= 1;
		device_queue_create_info.pQueuePriorities	= &queuePriority;

		device_queue_create_infos.push_back(device_queue_create_info);
	}

	// Ensure the logical device has the required families, extensions and validation layers
	VkPhysicalDeviceFeatures physical_device_features{};
	physical_device_features.samplerAnisotropy = VK_TRUE;
	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount		= static_cast<uint32_t>(device_queue_create_infos.size());
	device_create_info.pQueueCreateInfos		= device_queue_create_infos.data();
	device_create_info.pEnabledFeatures			= &physical_device_features;
	device_create_info.enabledExtensionCount	= static_cast<uint32_t>(_requiredDeviceExtensions.size());
	device_create_info.ppEnabledExtensionNames	= _requiredDeviceExtensions.data();
	if (_enableDebug) {
		device_create_info.enabledLayerCount	= static_cast<uint32_t>(_requestedLayers.size());
		device_create_info.ppEnabledLayerNames	= _requestedLayers.data();
	} else {
		device_create_info.enabledLayerCount	= 0;
	}

	if (vkCreateDevice(_physicalDevice, &device_create_info, nullptr, &_device) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::_InitLogicalDevice::vkCreateDevice" << std::endl;
		exit(-1);
	}

	// Get the handle of the queues created in the logical device
	vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

bool Renderer::_CheckValidationLayerSupport() {

	// Get number of layers supported by the system
	uint32_t layerCount;
	if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CheckValidationLayerSupport::EnumerateInstanceLayerProperties::0" << std::endl;
		exit(-1);
	}
	
	// Get the layers and add them to a vector
	std::vector<VkLayerProperties> systemLayers(layerCount);
	if (vkEnumerateInstanceLayerProperties(&layerCount, systemLayers.data()) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CheckValidationLayerSupport::EnumerateInstanceLayerProperties::1" << std::endl;
		exit(-1);
	}

	// Now do a search over the vector to see if the requested layers are availiable
	for (uint32_t i = 0; i < _requestedLayers.size(); i++) {

		bool layerFound = false;
		for (uint32_t x = 0; x < layerCount; x++) {

			if (!strcmp(_requestedLayers.at(i), systemLayers.at(x).layerName)) {
				// The requested layer exists
				// Continue to the next layer
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			std::cout << "ERROR::Renderer::CheckValidationLayerSupport::RequestedLayersNotAvailiable::" << _requestedLayers.at(i) << std::endl;
			return false;
		}	
	}

	return true;
}


void Renderer::_InitSwapChain() {

	// Get the swapchain parameters
	PhysicalDeviceSurface support = _GetSwapChainCapabilities(_physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = _GetSurfaceFormat(support.formats);
	VkPresentModeKHR presentMode = _GetPresentMode(support.presentModes);
	VkExtent2D extent = _GetSwapExtent(support.capabilities);

	// Define the amount of images in the swapchain
	uint32_t images = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 && images > support.capabilities.maxImageCount) {
		images = support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swap_chain_create_info{};
	swap_chain_create_info.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_chain_create_info.surface			= _surface;
	swap_chain_create_info.minImageCount	= images;
	swap_chain_create_info.imageFormat		= surfaceFormat.format;
	swap_chain_create_info.imageColorSpace	= surfaceFormat.colorSpace;
	swap_chain_create_info.imageExtent		= extent;
	swap_chain_create_info.imageArrayLayers = 1;
	swap_chain_create_info.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Get the indices of the graphics family and the present family
	QueueFamilyIndices indices = _FindQueueFamilies(_physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	// If they are not in the same family then allow the images to be used across multiple familys
	// without tranfering ownership (less performance)
	if (indices.graphicsFamily != indices.presentFamily) {
		swap_chain_create_info.imageSharingMode			= VK_SHARING_MODE_CONCURRENT;
		swap_chain_create_info.queueFamilyIndexCount	= 2;
		swap_chain_create_info.pQueueFamilyIndices		= queueFamilyIndices;
	} else {
		swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	swap_chain_create_info.preTransform		= support.capabilities.currentTransform;
	swap_chain_create_info.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_chain_create_info.presentMode		= presentMode;
	swap_chain_create_info.clipped			= VK_TRUE;
	swap_chain_create_info.oldSwapchain		= VK_NULL_HANDLE;

	_swapChainFormat = surfaceFormat.format;
	_swapChainExtent = extent;

	// Now create the swapchain
	if (vkCreateSwapchainKHR(_device, &swap_chain_create_info, nullptr, &_swapChain) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::InitSwapChain::CreateSwapchainKHR" << std::endl;
		exit(-1);
	}

	uint32_t imageCount;
	vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

}

void Renderer::_DeconstructSwapChain() {
	// Destroy the depth buffer images
	vkDestroyImageView(_device, _depthImageView, nullptr);
	vkDestroyImage(_device, _depthImage, nullptr);
	vkFreeMemory(_device, _depthImageMemory, nullptr);

	// Destroy all uniform buffer objects
	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
		vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
	}

	// Destroy the current descriptor pool
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

	// Destroy all the framebuffers
	for (auto framebuffer : _framebuffers) {
		vkDestroyFramebuffer(_device, framebuffer, nullptr);
	}

	// Free command buffers and pools
	vkFreeCommandBuffers(_device, _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());

	// Destory the pipeline, pipeline layout and the render pass objects
	vkDestroyPipeline(_device, _pipeline, nullptr);
	vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
	vkDestroyRenderPass(_device, _renderPass, nullptr);

	// Loop to destroy each image view from the vector member
	for (auto& view : _swapChainImageViews) {
		vkDestroyImageView(_device, view, nullptr);
	}

	// Cleaup the current swapchain
	vkDestroySwapchainKHR(_device, _swapChain, nullptr);
}

void Renderer::_RecreateSwapChain() {
	// If window is minimized then wait till it is in the foreground again
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	// Wait on all async operations to finish
	vkDeviceWaitIdle(_device);

	// Remove the old swapchain
	_DeconstructSwapChain();

	_InitSwapChain();
	_CreateImageViews();
	_CreateRenderPass();
	_CreateGraphicsPipeline();

	_CreateDepthResources();
	_CreateFramebuffers();
	_CreateUniformBuffers();
	_CreateDescriptorPool();
	_CreateDescriptorSets();
	
	// Cleanup the old command pool
	vkDestroyCommandPool(_device, _commandPool, nullptr);
	_CreateCommandPool();

	_CreateCommandBuffers();

}

// Function to get the best format from a vector of surface formats
VkSurfaceFormatKHR Renderer::_GetSurfaceFormat(std::vector<VkSurfaceFormatKHR>& avaliableFormats) {
	// Prefer the combination of VK_FORMAT_B8G8R8A8_SRGB and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	// If this could not be found then return the first value
	for (auto& format : avaliableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return avaliableFormats[0];
}

// Function to select the best presentation mode from a vector
VkPresentModeKHR Renderer::_GetPresentMode(std::vector<VkPresentModeKHR>& avaliableModes) {

	// For now return MAILBOX_KHR if it is avaliable
	// mailbox is a triple buffering mode with less input lag than v-sync (double)
	for (auto& mode : avaliableModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}

	// If mailbox not availiable just return the standard immediate (no v-sync)
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

// Get the swapchain extent, this takes into account the width and height of the window
VkExtent2D Renderer::_GetSwapExtent(VkSurfaceCapabilitiesKHR& capabilities) {
	// If the current width or height is the max value of uint32_t then we can set the width and height
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		// Set swapchain extent to width and height of window
		int width, height;
		glfwGetFramebufferSize(_window, &width, &height);

		VkExtent2D extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		// Ensure it is within the bounds of the swapchain
		extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
		extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

		return extent;
	}
}


void Renderer::_CreateImageViews() {
	// Resize to fit all the images that will be created
	_swapChainImageViews.resize(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		_swapChainImageViews[i] = _CreateImageView(_swapChainImages[i], _swapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

// Function that returns binary data from a file in a vector of chars
std::vector<char> ReadFile(const std::string& filename) {

	// ios::ate to read from the end of the file so a buffer can be allocated
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::cout << "ERROR::ReadFile::CannotReadFile " << filename << std::endl;
		std::vector<char> value;
		return value;
	}
	
	// Allocate buffer
	size_t size = file.tellg();
	std::vector<char> buffer(size);

	// Go back to start of file and read into buffer
	file.seekg(0);
	file.read(buffer.data(), size);

	file.close();
	return buffer;
}


void Renderer::_CreateDescriptorSetLayout() {
	// Every binding needs a new VkDescriptorSetLayoutBinding struct
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; // Binding accessable in both shaders

	VkDescriptorSetLayoutBinding sampler_layout_binding{};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

	VkDescriptorSetLayoutCreateInfo layout_create_info{};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());;
	layout_create_info.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(_device, &layout_create_info, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
		std::cout << "ERROR:Renderer::CreateDescriptorSetLayout::CreateDescriptorSetLayout" << std::endl;
		exit(-1);
	}
}

void Renderer::_CreateRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = _swapChainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // For multisampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = _FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo render_pass_create_info{};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments = attachments.data();
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency;

	if (vkCreateRenderPass(_device, &render_pass_create_info, nullptr, &_renderPass) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateRenderPass::CreateRenderPass" << std::endl;
		exit(-1);
	}
}

void Renderer::_CreateGraphicsPipeline() {

	// Read the code from file
	auto vertCode = ReadFile("shaders/vert.spv");
	auto fragCode = ReadFile("shaders/frag.spv");

	// Load the code into shader modules 
	VkShaderModule vertShaderModule = _GetShaderModule(vertCode);
	VkShaderModule fragShaderModule = _GetShaderModule(fragCode);

	// The code can be freed now
	vertCode.clear();
	fragCode.clear();

	// Create structs to house the shader info
	VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
	vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_create_info.module = vertShaderModule;
	vert_shader_stage_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
	frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_create_info.module = fragShaderModule;
	frag_shader_stage_create_info.pName = "main";

	// Combine the shader info structs into an array
	VkPipelineShaderStageCreateInfo shaderStages[] = { vert_shader_stage_create_info, frag_shader_stage_create_info };

	// Now create all the structs that will be used to create the render pipeline
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
	vertex_input_state_create_info.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	// TODO: refactor this
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertex_input_state_create_info.pVertexBindingDescriptions = &bindingDescription;
	vertex_input_state_create_info.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
	input_assembly_state_create_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology				= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)_swapChainExtent.width;
	viewport.height = (float)_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = _swapChainExtent;

	VkPipelineViewportStateCreateInfo viewport_state_create_info{};
	viewport_state_create_info.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount	= 1;
	viewport_state_create_info.pViewports		= &viewport;
	viewport_state_create_info.scissorCount		= 1;
	viewport_state_create_info.pScissors		= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info{};
	rasterizer_state_create_info.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_state_create_info.depthClampEnable			= VK_FALSE;
	rasterizer_state_create_info.rasterizerDiscardEnable	= VK_FALSE;
	rasterizer_state_create_info.polygonMode				= VK_POLYGON_MODE_FILL;
	rasterizer_state_create_info.lineWidth					= 1.0f;
	rasterizer_state_create_info.cullMode					= VK_CULL_MODE_BACK_BIT;
	rasterizer_state_create_info.frontFace					= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer_state_create_info.depthBiasEnable			= VK_FALSE;
	rasterizer_state_create_info.depthBiasConstantFactor	= 0.0f; // Optional
	rasterizer_state_create_info.depthBiasClamp				= 0.0f; // Optional
	rasterizer_state_create_info.depthBiasSlopeFactor		= 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
	multisample_state_create_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable	= VK_FALSE;
	multisample_state_create_info.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
	multisample_state_create_info.minSampleShading		= 1.0f; // Optional
	multisample_state_create_info.pSampleMask			= nullptr; // Optional
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE; // Optional
	multisample_state_create_info.alphaToOneEnable		= VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
	depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
	depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
	depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state_create_info.minDepthBounds = 0.0f; // Optional
	depth_stencil_state_create_info.maxDepthBounds = 1.0f; // Optional
	depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
	color_blend_attachment_state.colorWriteMask			= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_state.blendEnable			= VK_FALSE;
	color_blend_attachment_state.srcColorBlendFactor	= VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment_state.dstColorBlendFactor	= VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment_state.colorBlendOp			= VK_BLEND_OP_ADD; // Optional
	color_blend_attachment_state.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment_state.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment_state.alphaBlendOp			= VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
	color_blend_state_create_info.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.logicOpEnable		= VK_FALSE;
	color_blend_state_create_info.logicOp			= VK_LOGIC_OP_COPY; // Optional
	color_blend_state_create_info.attachmentCount	= 1;
	color_blend_state_create_info.pAttachments		= &color_blend_attachment_state;
	color_blend_state_create_info.blendConstants[0] = 0.0f; // Optional
	color_blend_state_create_info.blendConstants[1] = 0.0f; // Optional
	color_blend_state_create_info.blendConstants[2] = 0.0f; // Optional
	color_blend_state_create_info.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount			= 1;
	pipeline_layout_create_info.pSetLayouts				= &_descriptorSetLayout;

	if (vkCreatePipelineLayout(_device, &pipeline_layout_create_info, nullptr, &_pipelineLayout) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateGraphicsPipeline::CreatePipelineLayout" << std::endl;
		exit(-1);
	}

	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount				= 2;
	pipeline_create_info.pStages				= shaderStages;
	pipeline_create_info.pVertexInputState		= &vertex_input_state_create_info;
	pipeline_create_info.pInputAssemblyState	= &input_assembly_state_create_info;
	pipeline_create_info.pViewportState			= &viewport_state_create_info;
	pipeline_create_info.pRasterizationState	= &rasterizer_state_create_info;
	pipeline_create_info.pMultisampleState		= &multisample_state_create_info;
	pipeline_create_info.pDepthStencilState		= nullptr; // Optional
	pipeline_create_info.pColorBlendState		= &color_blend_state_create_info;
	pipeline_create_info.pDynamicState			= nullptr; // Optional
	pipeline_create_info.layout					= _pipelineLayout;
	pipeline_create_info.renderPass				= _renderPass;
	pipeline_create_info.subpass				= 0;
	pipeline_create_info.pDepthStencilState		= &depth_stencil_state_create_info;

	if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &_pipeline) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateGraphicsPipeline::CreateGraphicsPipelines" << std::endl;
		exit(-1);
	}

	// Cleanup the shader modules
	vkDestroyShaderModule(_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(_device, vertShaderModule, nullptr);
}

// Function that takes a char vector of bytecode and converts it to a VkShaderModule
VkShaderModule Renderer::_GetShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo shader_module_create_info{};
	shader_module_create_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize	= code.size();
	shader_module_create_info.pCode		= reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule module;
	if (vkCreateShaderModule(_device, &shader_module_create_info, nullptr, &module) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateGraphicsPipeline::GetShaderModule::CreateShaderModule" << std::endl;
		exit(-1);
	}

	return module;
}

void Renderer::_CreateFramebuffers() {
	// Resize the vector to hold all of the framebuffers
	_framebuffers.resize(_swapChainImageViews.size());

	// Itterate over the image views and create frame buffers from them
	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {

		std::array<VkImageView, 2> attachments = {
			_swapChainImageViews[i],
			_depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = _swapChainExtent.width;
		framebufferInfo.height = _swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
			std::cout << "ERROR::Renderer::CreateFramebuffers::CreateFramebuffer" << std::endl;
			exit(-1);
		}
	}
}

bool Renderer::_HasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat Renderer::_FindDepthFormat() {
	return _FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat Renderer::_FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

void Renderer::_CreateDepthResources() {
	VkFormat depthFormat = _FindDepthFormat();
	_CreateImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage, _depthImageMemory);
	_depthImageView = _CreateImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	
	// No need to transfer explicitly to a depth attachment but might aswell do it incase the function is copied later
	_TransitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

void Renderer::_CreateTextureImage() {
	int width;
	int height;
	int textureChannels;

	// Load the image into memory
	// STBI_rgb_alpha forces the image loaded to have an alpha channel, prevents some errors with misalignment
	stbi_uc* pixels = stbi_load("shaders/texture.jpg", &width, &height, &textureChannels, STBI_rgb_alpha);

	// Calculate the required device size using w*h*channels
	VkDeviceSize imageSize = (uint64_t)width * (uint64_t)height * 4;

	if (!pixels) {
		std::cout << "ERROR::Renderer::CreateTextureImage::LoadFailed" << std::endl;
	}

	// Setup the host visible buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	_CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// Copy the pixel data to the buffer
	void* data;
	vkMapMemory(_device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(_device, stagingBufferMemory);

	stbi_image_free(pixels);

	_CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);
	_TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_CopyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	_TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(_device, stagingBuffer, nullptr);
	vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void Renderer::_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo image_create_info{};
	image_create_info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType		= VK_IMAGE_TYPE_2D;
	image_create_info.extent.width	= width;
	image_create_info.extent.height	= height;
	image_create_info.extent.depth	= 1;
	image_create_info.mipLevels		= 1;
	image_create_info.arrayLayers	= 1;
	image_create_info.format		= format;
	image_create_info.tiling		= tiling;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage			= usage;
	image_create_info.samples		= VK_SAMPLE_COUNT_1_BIT;
	image_create_info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

	// VK_FORMAT_R8G8B8A8_SRGB may not be supported here, could always create a list to fall back on
	if (vkCreateImage(_device, &image_create_info, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(_device, image, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = _FindMemoryType(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &memory_allocate_info, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(_device, image, imageMemory, 0);
}

void Renderer::_TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkImageMemoryBarrier image_memory_barrier{};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.oldLayout = oldLayout;
	image_memory_barrier.newLayout = newLayout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;
	image_memory_barrier.srcAccessMask = 0; // TODO
	image_memory_barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (_HasStencilComponent(format)) {
			image_memory_barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.srcAccessMask = 0;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.srcAccessMask = 0;
		image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &image_memory_barrier
	);

	_EndSingleTimeCommands(commandBuffer);
}

void Renderer::_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkBufferImageCopy buffer_image_copy{};
	buffer_image_copy.bufferOffset = 0;
	buffer_image_copy.bufferRowLength = 0;
	buffer_image_copy.bufferImageHeight = 0;

	buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	buffer_image_copy.imageSubresource.mipLevel = 0;
	buffer_image_copy.imageSubresource.baseArrayLayer = 0;
	buffer_image_copy.imageSubresource.layerCount = 1;

	buffer_image_copy.imageOffset = { 0, 0, 0 };
	buffer_image_copy.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&buffer_image_copy
	);

	_EndSingleTimeCommands(commandBuffer);
}

void Renderer::_CreateTextureImageView() {
	_textureImageView = _CreateImageView(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkImageView Renderer::_CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void Renderer::_CreateTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(_device, &samplerInfo, nullptr, &_textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

uint32_t Renderer::_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);
	

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void Renderer::_CreateVertexBuffer() {

	// Create the staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkDeviceSize bufferSize = sizeof(_vertices[0]) * _vertices.size();
	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// Move the raw vertex data to the staging buffer
	void* data;
	vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, stagingBufferMemory);

	// Create the local device buffer (in physical device memory)
	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexBufferMemory);

	// Copy the RAM buffer to device memory
	_CopyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

	// Cleanup
	vkDestroyBuffer(_device, stagingBuffer, nullptr);
	vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void Renderer::_CreateIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(_indices[0]) * _indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _indices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, stagingBufferMemory);

	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);

	_CopyBuffer(stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(_device, stagingBuffer, nullptr);
	vkFreeMemory(_device, stagingBufferMemory, nullptr);

}

// Create single time command buffers
VkCommandBuffer Renderer::_BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandPool = _commandPool;
	command_buffer_allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(_device, &command_buffer_allocate_info, &command_buffer);

	// Inform the driver that the buffer is one time use only
	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Start recording into the command buffer
	vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

	return command_buffer;
}

// End single use command buffers
void Renderer::_EndSingleTimeCommands(VkCommandBuffer command_buffer) {

	// Finish recording
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	// Send the command to copy the memory
	vkQueueSubmit(_graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);

	// Wait for operation to finish and cleanup
	vkQueueWaitIdle(_graphicsQueue);
	vkFreeCommandBuffers(_device, _commandPool, 1, &command_buffer);
}

// For copying one buffer into another buffer
void Renderer::_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	// Memory transfers are executed using command buffers so create a temporary one
	// Could use a seperate command pool with memory allocation optimisations ( VK_COMMAND_POOL_CREATE_TRANSIENT_BIT )
	VkCommandBuffer command_buffer = _BeginSingleTimeCommands();

	// Copy the data
	VkBufferCopy copy_region{};
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, srcBuffer, dstBuffer, 1, &copy_region);

	_EndSingleTimeCommands(command_buffer);
}

// Create a VkBuffer object
void Renderer::_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
 
	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(_device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(_device, buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = _FindMemoryType(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &memory_allocate_info, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

void Renderer::_CreateUniformBuffers() {
	// Get the size of the UBO
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	// Resize the buffer vectors to the swap chain image size
	_uniformBuffers.resize(_swapChainImages.size());
	_uniformBuffersMemory.resize(_swapChainImages.size());

	// For each swap chain image create a buffer and assign it to the vectors
	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		_CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);
	}
}

void Renderer::_CreateDescriptorPool() {

	// Allocate one descriptor for every frame
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(_swapChainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(_swapChainImages.size());

	VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
	descriptor_pool_create_info.pPoolSizes = poolSizes.data();
	descriptor_pool_create_info.maxSets = static_cast<uint32_t>(_swapChainImages.size());

	if (vkCreateDescriptorPool(_device, &descriptor_pool_create_info, nullptr, &_descriptorPool) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateDescriptorPool::CreateDescriptorPool" << std::endl;
		exit(-1);
	}
}

void Renderer::_CreateDescriptorSets() {
	// Create a descriptor set layout vector for each frame
	std::vector<VkDescriptorSetLayout> layouts(_swapChainImages.size(), _descriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = _descriptorPool;
	descriptor_set_allocate_info.descriptorSetCount = static_cast<uint32_t>(_swapChainImages.size());
	descriptor_set_allocate_info.pSetLayouts = layouts.data();

	_descriptorSets.resize(_swapChainImages.size());
	if (vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, _descriptorSets.data()) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateDescriptorSets::AllocateDescriptorSets" << std::endl;
		exit(-1);
	}

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = _textureImageView;
		imageInfo.sampler = _textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = _descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = _descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::_CreateCommandPool() {

	// Get the queue family indices of the current device
	QueueFamilyIndices queueFamilyIndices = _FindQueueFamilies(_physicalDevice);

	// Since the commands will be for drawing use the graphics family
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex	= queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(_device, &command_pool_create_info, nullptr, &_commandPool) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateCommandPool::CreateCommandPool" << std::endl;
		exit(-1);
	}

}

void Renderer::_CreateCommandBuffers() {
	
	// Resize the command buffer vector to the number of framebuffers
	_commandBuffers.resize(_framebuffers.size());

	// Create the primary command buffer
	VkCommandBufferAllocateInfo command_buffer_alloc_info{};
	command_buffer_alloc_info.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.commandPool			= _commandPool;
	command_buffer_alloc_info.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount	= (uint32_t)_commandBuffers.size();

	if (vkAllocateCommandBuffers(_device, &command_buffer_alloc_info, _commandBuffers.data()) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::CreateCommandBuffers::AllocateCommandBuffers" << std::endl;
		exit(-1);
	}

	// Begin recording each command buffer
	for (size_t i = 0; i < _commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo command_buffer_begin_info{};
		command_buffer_begin_info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags				= 0;
		command_buffer_begin_info.pInheritanceInfo	= nullptr;

		if (vkBeginCommandBuffer(_commandBuffers[i], &command_buffer_begin_info) != VK_SUCCESS) {
			std::cout << "ERROR::Renderer::CreateCommandBuffers::BeginCommandBuffer" << std::endl;
			exit(-1);
		}

		// Drawing starts by beginning the render pass
		VkRenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass			= _renderPass;
		render_pass_begin_info.framebuffer			= _framebuffers[i];
		render_pass_begin_info.renderArea.offset	= { 0, 0 };
		render_pass_begin_info.renderArea.extent	= _swapChainExtent;

		// Define what the clear colour value should be
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		render_pass_begin_info.clearValueCount	= static_cast<uint32_t>(clearValues.size());
		render_pass_begin_info.pClearValues		= clearValues.data();

		// Start recording the render pass for this buffer
		vkCmdBeginRenderPass(_commandBuffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			// Bind the graphics pipeline
			vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

			VkBuffer vertexBuffers[] = { _vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			// Bind the descriptor sets containing the UBO
			vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[i], 0, nullptr);

			// Actually draw the vertices, finally
			vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

		// Finished recording the render pass
		vkCmdEndRenderPass(_commandBuffers[i]);

		if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
			std::cout << "ERROR::Renderer::CreateCommandBuffers::EndCommandBuffer" << std::endl;
			exit(-1);
		}
	}
}

// Creates syncronisation objects used in the rendering (fences and semaphores)
void Renderer::_CreateSyncObjects() {

	// Ensure the vectors are the correct size
	_imageAvailableSemaphores.resize(_max_frames_in_flight);
	_renderFinishedSemaphores.resize(_max_frames_in_flight);
	_inFlightFences.resize(_max_frames_in_flight);
	_imagesInFlight.resize(_swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphore_create_info{};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// For each potential frame in flight add sync objects
	for (size_t i = 0; i < _max_frames_in_flight; i++) {
		if (vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(_device, &fence_create_info, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {

			std::cout << "ERROR::Renderer::CreateSemaphores::CreateSemaphore" << std::endl;
			exit(-1);
		}
	}
}

void Renderer::_MainLoop() {

	while (!glfwWindowShouldClose(_window)) {
		

		glfwPollEvents();
		_DrawFrame();
	}

	// Wait for async operations to finish before cleaning up
	vkDeviceWaitIdle(_device);
}

// Aquire image from swapchain, execute the command buffer with that image as attachment in the framebuffer and return the image to the swap chain
void Renderer::_DrawFrame() {

	// If the current frame is inflight wait for the signal from the fence for the current frame (GPU-CPU sync)
	vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	// First aquire an image from the swap chain.
	// UINT64_MAX for the timeout disables the timeout
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	// If the swap chain has become incompatible recreate it and skip this frame
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_RecreateSwapChain();
		return;
	} else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		std::cout << "ERROR::Renderer::Mainloop::DrawFrame::FailedToAquireSwapChainImage" << std::endl;
		exit(-1);
	}


	// Check if a frame is already using this image, if there is then wait on that fence
	if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(_device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as being in use by this frame
	_imagesInFlight[imageIndex] = _inFlightFences[_currentFrame];

	// Update the uniforms for the shaders now that we know what image is going to be aquired 
	_UpdateUniformBuffer(imageIndex);


	VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submit_info{};
	submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount	= 1;
	submit_info.pWaitSemaphores		= waitSemaphores;
	submit_info.pWaitDstStageMask	= waitStages;
	submit_info.commandBufferCount	= 1;
	submit_info.pCommandBuffers		= &_commandBuffers[imageIndex];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signalSemaphores;
	
	// Reset the fence for the current frame
	vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

	// Submit the command buffer to the graphics queue
	if (vkQueueSubmit(_graphicsQueue, 1, &submit_info, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// Now present the frame to the surface
	VkSwapchainKHR swapChains[] = { _swapChain };
	VkPresentInfoKHR present_info{};
	present_info.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores	= signalSemaphores;
	present_info.swapchainCount		= 1;
	present_info.pSwapchains		= swapChains;
	present_info.pImageIndices		= &imageIndex;
	present_info.pResults			= nullptr; // Takes an array of VkResult so success can be validated

	// Submit the request to present an image to the swap chain
	result = vkQueuePresentKHR(_presentQueue, &present_info);

	// If window resized then recreate the swapchain for the next frame to be drawn
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResize) {
		_framebufferResize = false;
		_RecreateSwapChain();
	} else if (result != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::Mainloop::DrawFrame::FailedToPresentSwapChainImage" << std::endl;
		exit(-1);
	}

	// Increment and loop current frame back round
	_currentFrame = (_currentFrame + 1) % _max_frames_in_flight;
}

void Renderer::_UpdateUniformBuffer(uint32_t currentImage) {

	// Get the time from the start of the rendering
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 10.0f);

	// Flip the Y coordinate since GLM is designed for OpenGL and vulkan has the opposite of OpenGL
	ubo.proj[1][1] *= -1;

	// Acctually move the data into the ubo memory buffer
	void* data;
	vkMapMemory(_device, _uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(_device, _uniformBuffersMemory[currentImage]);
}
