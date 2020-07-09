#include "Renderer.h"

Renderer::Renderer() {
	_InitInstance();
	_InitDevice();
}

Renderer::~Renderer() {
	_DeconstructDevice();
	_DeconstructInstance();
}

void Renderer::_InitInstance() {
	VkApplicationInfo application_info = {};
	application_info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.apiVersion				= VK_MAKE_VERSION(1, 0, 3);
	application_info.applicationVersion		= VK_MAKE_VERSION(0, 1, 0);
	application_info.pApplicationName		= "Vulkan tutorial";
	application_info.engineVersion			= VK_MAKE_VERSION(0, 1, 0);
	application_info.pEngineName			= "Geton Engine";

	VkInstanceCreateInfo instance_create_info{};
	instance_create_info.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo	= &application_info;
	
	if (vkCreateInstance(&instance_create_info, nullptr, &_instance) != VK_SUCCESS) {
		std::cout << "Error creating a vulkan instance" << std::endl;
		exit(-1);
	}
}

void Renderer::_DeconstructInstance() {
	// Destroy vulkan intsance and point to null
	vkDestroyInstance(_instance, nullptr);
	_instance = nullptr;
}

void Renderer::_InitDevice() {

	// Get number of all physical devices
	uint32_t gpu_count;
	if (vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr) != VK_SUCCESS) {
		std::cout << "ERROR::RENDERER::INITDEVICE::EnumeratePhysicalDevices::0" << std::endl;
		exit(-1);
	}

	// Get list of all physical devices
	std::vector<VkPhysicalDevice> gpu_list(gpu_count);
	if (vkEnumeratePhysicalDevices(_instance, &gpu_count, gpu_list.data()) != VK_SUCCESS) {
		std::cout << "ERROR::RENDERER::INITDEVICE::EnumeratePhysicalDevices::1" << std::endl;
		exit(-1);
	}
	
	// If there are no physical devices then quit
	if (!gpu_count) {
		std::cout << "ERROR::RENDERER::INITDEVICE::GPUCOUNT::ZeroDevices" << std::endl;
		exit(-1);
	}

	// Assume gpu#0 is the gpu we want to use
	uint32_t deviceIndex = 0;
	_gpu = gpu_list[deviceIndex];

	// Just for debug print out the device name
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(_gpu, &physical_device_properties);
	std::cout << "Using physical device " << deviceIndex << " ( " << physical_device_properties.deviceName << " )" << std::endl;

	// Get the amount of familys in the GPU
	uint32_t family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, nullptr);
	std::vector<VkQueueFamilyProperties> family_property_list(family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, family_property_list.data());

	// Find which family supports the graphics bit
	bool foundGraphicsBit = false;
	for (uint32_t i = 0; i < family_count; i++) {
		if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			foundGraphicsBit = true;
			_graphics_family_index = i;
		}
	}

	if (!foundGraphicsBit) {
		std::cout << "ERROR::RENDERER::INITDEVICE::GraphicsBitNotFound" << std::endl;
		exit(-1);
	}

	float queue_prioroties[]{ 1.0f };

	VkDeviceQueueCreateInfo device_queue_create_info = {};
	device_queue_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.queueFamilyIndex	= _graphics_family_index;
	device_queue_create_info.queueCount			= 1;
	device_queue_create_info.pQueuePriorities	= queue_prioroties;

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos	= &device_queue_create_info;

	if (vkCreateDevice(_gpu, &device_create_info, nullptr, &_device) != VK_SUCCESS) {
		std::cout << "ERROR::RENDERER::INITDEVICE::CreateDevice" << std::endl;
		exit(-1);
	}
}

void Renderer::_DeconstructDevice() {
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}

