#include "Renderer.h"

const char* appName = "Vulkan";

Renderer::Renderer() {
	_InitWindow();
	_InitInstance();
	if (enableValidationLayers) { _InitDebugMessanger(); }
	_InitPhysicalDevice();
	_InitDevice();

	_MainLoop();
}

Renderer::~Renderer() {

	if (enableValidationLayers) { _DeconstructDebugMessanger(); }

	_DeconstructDevice();
	_DeconstructInstance();
	_DeconstructWindow();
}

void Renderer::_InitWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, appName, nullptr, nullptr);
}

void Renderer::_DeconstructWindow() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::_MainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Renderer::_InitInstance() {
	// First check to see if validation layers are required and supported
	if (enableValidationLayers && !_CheckValidationLayerSupport()) {
		std::cout << "ERROR::Renderer::InitInstance::ValidationLayersRequestedButNotSupported" << std::endl;
	}

	VkApplicationInfo application_info = {};
	application_info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.applicationVersion		= VK_MAKE_VERSION(1, 0, 0);
	application_info.pApplicationName		= appName;
	application_info.engineVersion			= VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName			= "Geton Engine";


	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


	std::vector<const char*> extensions = _GetRequiredExtensions();
	VkInstanceCreateInfo instance_create_info {};
	instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo			= &application_info;
	instance_create_info.enabledExtensionCount		= static_cast<uint32_t>(extensions.size());
	instance_create_info.ppEnabledExtensionNames	= extensions.data();
	// If validation layers are enabled then add them to the instance
	if (enableValidationLayers) {
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

void Renderer::_DeconstructInstance() {
	// Destroy vulkan intsance and point to null
	vkDestroyInstance(_instance, nullptr);
	_instance = nullptr;
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

	// If the current device is still a nullptr then no device was found
	if (currentDevice == nullptr) {
		std::cout << "ERROR::Renderer::InitDevice::NoDevicesFound" << std::endl;
		exit(-1);
	}

	// If it is not assign it to the renderer attribute physical device
	_physicalDevice = currentDevice;
}

int Renderer::_RatePhysicalDevice(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	QueueFamilyIndices indices = _FindQueueFamilys(device);

	return indices.IsComplete();
}

void Renderer::_DeconstructDevice() {
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}

void Renderer::_InitDevice() {
	QueueFamilyIndices indices = _FindQueueFamilys(_physicalDevice);

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo device_queue_create_info{};
	device_queue_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.queueFamilyIndex	= indices.graphicsFamily.value();
	device_queue_create_info.queueCount			= 1;
	device_queue_create_info.pQueuePriorities	= &queuePriority;

	VkPhysicalDeviceFeatures physical_device_features{};

	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos		= &device_queue_create_info;
	device_create_info.queueCreateInfoCount		= 1;
	device_create_info.pEnabledFeatures			= &physical_device_features;
	device_create_info.enabledExtensionCount	= 0;
	if (enableValidationLayers) {
		device_create_info.enabledLayerCount	= static_cast<uint32_t>(_requestedLayers.size());
		device_create_info.ppEnabledLayerNames	= _requestedLayers.data();
	} else {
		device_create_info.enabledLayerCount	= 0;
	}

	if (vkCreateDevice(_physicalDevice, &device_create_info, nullptr, &_device) != VK_SUCCESS) {
		std::cout << "ERROR::Renderer::_InitLogicalDevice::vkCreateDevice" << std::endl;
		exit(-1);
	}

	vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
}

QueueFamilyIndices Renderer::_FindQueueFamilys(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int index = 0;
	for (auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = index;
		}

		if (indices.IsComplete()) {
			break;
		}

		index++;
	}

	return indices;
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
	_requestedLayers = {"VK_LAYER_KHRONOS_validation"};
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

std::vector<const char*> Renderer::_GetRequiredExtensions() {

	uint32_t glfwCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::_DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void Renderer::_InitDebugMessanger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT messanger_create_info {};
	messanger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messanger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messanger_create_info.pfnUserCallback = _DebugCallback;
	messanger_create_info.pUserData = nullptr; // Optional (void*) parameter to pass custom variables to the callback

	// Since the function to set the create the debug messanger is not loaded
	// The address needs to be looked up
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");

	// Now if the function was found then execute it
	if (vkCreateDebugUtilsMessengerEXT != nullptr) {
		vkCreateDebugUtilsMessengerEXT(_instance, &messanger_create_info, nullptr, &_DebugMessanger);
	} else {
		std::cout << "ERROR::Renderer::InitDebugMessanger::InvalidFunctionPointer" << std::endl;
		exit(-1);
	}
}

void Renderer::_DeconstructDebugMessanger() {
	// Validate the instance exists before accessing it
	if (_instance == nullptr) {
		std::cout << "ERROR::Renderer::DeconstructDebugMessanger::InstanceIsNullPointer" << std::endl;
		exit(-1);
	}

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT != nullptr) {
		vkDestroyDebugUtilsMessengerEXT(_instance, _DebugMessanger, nullptr);
	} else {
		std::cout << "ERROR::Renderer::DeconstructDebugMessanger::InvalidFunctionPointer" << std::endl;
		exit(-1);
	}
}

