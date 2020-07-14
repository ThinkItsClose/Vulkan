#pragma once

#include <optional>

/*

Header file containing structs used in the renderer class
These are typically used in setup for validation

*/


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

// Struct used to hold information about the swapchain capabilities of a device
struct PhysicalDeviceSurface {
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};