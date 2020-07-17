#pragma once

#include <optional>
#include <array>

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

struct Vertex {
	glm::vec2 position;
	glm::vec3 colour;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, colour);

		return attributeDescriptions;
	}
};