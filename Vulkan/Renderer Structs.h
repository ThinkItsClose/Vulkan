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
		VkVertexInputBindingDescription input_binding_description{};
		input_binding_description.binding = 0;
		input_binding_description.stride = sizeof(Vertex);
		input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return input_binding_description;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> input_attribute_description{};

		input_attribute_description[0].binding = 0;
		input_attribute_description[0].location = 0;
		input_attribute_description[0].format = VK_FORMAT_R32G32_SFLOAT;
		input_attribute_description[0].offset = offsetof(Vertex, position);

		input_attribute_description[1].binding = 0;
		input_attribute_description[1].location = 1;
		input_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		input_attribute_description[1].offset = offsetof(Vertex, colour);

		return input_attribute_description;
	}
};

// Temporary structure to hold the MVP matricies
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};