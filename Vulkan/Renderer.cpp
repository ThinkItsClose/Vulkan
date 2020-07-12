#include "Renderer.h"

const char* appName = "Vulkan";

Renderer::Renderer() {
	_InitWindow();
	_InitInstance();
	_InitDevice();

	_CheckValidationLayerSupport();
	if (enableValidationLayers) { _InitDebugMessanger(); }


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

void Renderer::_InitDevice() {
}

void Renderer::_DeconstructDevice() {
	//vkDestroyDevice(_device, nullptr);
	//_device = nullptr;
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

