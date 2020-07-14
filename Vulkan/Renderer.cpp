#include "Renderer.h"

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

	_MainLoop();
}

Renderer::~Renderer() {
	// Cleaup the current swapchain
	vkDestroySwapchainKHR(_device, _swapChain, nullptr);

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

void Renderer::_InitWindow() {

	// No window resize right now as changing viewport size is not being handled
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_window = glfwCreateWindow(_width, _height, appName, nullptr, nullptr);
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
	std::cerr << pCallbackData->pMessage << std::endl;

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
	QueueFamilyIndices indices = _FindQueueFamilys(device);

	// Get the capabilities of the swapchain in the device
	// Only do this if it is known that the device extentions are supported (it contains the swapchain extension)
	bool swapChainGood = false;
	if (deviceExtensionsSupported) {
		// Bare minimum requirements for now
		PhysicalDeviceSurface swapChainSupport = _GetSwapChainCapabilities(device);
		swapChainGood = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.IsComplete() && deviceExtensionsSupported && swapChainGood;
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
QueueFamilyIndices Renderer::_FindQueueFamilys(VkPhysicalDevice device) {
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
	QueueFamilyIndices indices = _FindQueueFamilys(_physicalDevice);

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
	QueueFamilyIndices indices = _FindQueueFamilys(_physicalDevice);
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
		VkExtent2D extent = { _width, _height };

		// Ensure it is within the bounds of the swapchain
		extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
		extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

		return extent;
	}
}


void Renderer::_MainLoop() {
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
	}
}

