/// @file device.cpp
/// @brief Choosing a GPU.
#include "device.h"
#include "../logging/logger.h"
#include "../logging/vulkan_logging.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

std::vector<const char*> required_device_extensions() {
	return {
		// Stage 4: presenting images to a window is an extension, because
		// a GPU can be used without any screen at all.
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		// Stage 9: shaders without pipelines (see shader.h).
		VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
	};
}

namespace {

/// Does this GPU offer every extension in `wanted`?
bool supports_extensions(vk::PhysicalDevice device, const std::vector<const char*>& wanted) {
	const std::vector<vk::ExtensionProperties> available = device.enumerateDeviceExtensionProperties();
	for (const char* name : wanted) {
		// std::any_of: does any available extension have this name?
		const bool found = std::any_of(available.begin(), available.end(),
			[name](const vk::ExtensionProperties& extension) {
				return std::strcmp(extension.extensionName.data(), name) == 0;
			});
		if (!found) {
			Logger::get_logger().print(std::string("\tmissing extension ") + name);
			return false;
		}
	}
	return true;
}

/// Stage 9: having an extension is not enough; its FEATURES must be
/// supported too, and then turned on when the device is made. getFeatures2
/// fills a chain of feature structs, one per extension/version we ask about.
bool supports_features(vk::PhysicalDevice device) {
	auto chain = device.getFeatures2<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceShaderObjectFeaturesEXT>();
	// Stage 10: dynamic rendering is core in Vulkan 1.3, but still a feature
	// that has to be checked and turned on.
	if (!chain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering) {
		Logger::get_logger().print("\tno dynamicRendering feature");
		return false;
	}
	if (!chain.get<vk::PhysicalDeviceShaderObjectFeaturesEXT>().shaderObject) {
		Logger::get_logger().print("\tno shaderObject feature");
		return false;
	}
	return true;
}

/// Can we use this GPU at all? The author only checks extensions; a GPU must
/// also support Vulkan 1.3, which we ask for in the instance (entry 14).
bool is_suitable(vk::PhysicalDevice device) {
	if (device.getProperties().apiVersion < VK_API_VERSION_1_3) {
		Logger::get_logger().print("\tsupports Vulkan older than 1.3");
		return false;
	}
	// Extensions first: asking about the features of an extension the GPU
	// doesn't have is not allowed.
	return supports_extensions(device, required_device_extensions())
		&& supports_features(device);
}

}

vk::PhysicalDevice choose_physical_device(vk::Instance instance) {
	Logger::get_logger().print("Choosing physical device...");
	vk::PhysicalDevice chosen = nullptr;
	for (const vk::PhysicalDevice device : instance.enumeratePhysicalDevices()) {
		vklog::log(device);
		if (!is_suitable(device)) {
			continue;
		}
		// Take the first suitable GPU, but let a discrete GPU (its own
		// memory, usually the fast one in a laptop with two) replace it.
		const bool discrete = device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		if (!chosen || discrete) {
			chosen = device;
		}
	}
	if (!chosen) {
		throw std::runtime_error("No GPU supports what this renderer needs");
	}
	return chosen;
}
