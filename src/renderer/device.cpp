// device.cpp: choosing a GPU.
#include "device.h"
#include "../logging/logger.h"
#include "../logging/vulkan_logging.h"
#include <cstring>
#include <stdexcept>
#include <string>

std::vector<const char*> required_device_extensions() {
	return {
		// Stage 4: presenting images to a window is an extension, because
		// a GPU can be used without any screen at all.
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
}

namespace {

// Does this GPU offer every extension in `wanted`?
bool supports_extensions(vk::PhysicalDevice device, const std::vector<const char*>& wanted) {
	std::vector<vk::ExtensionProperties> available = device.enumerateDeviceExtensionProperties();
	for (const char* name : wanted) {
		bool found = false;
		for (const auto& extension : available) {
			found = found || std::strcmp(extension.extensionName.data(), name) == 0;
		}
		if (!found) {
			Logger::get_logger().print(std::string("\tmissing extension ") + name);
			return false;
		}
	}
	return true;
}

// Can we use this GPU at all? The author only checks extensions; a GPU must
// also support Vulkan 1.3, which we ask for in the instance (entry 14).
bool is_suitable(vk::PhysicalDevice device) {
	if (device.getProperties().apiVersion < VK_API_VERSION_1_3) {
		Logger::get_logger().print("\tsupports Vulkan older than 1.3");
		return false;
	}
	return supports_extensions(device, required_device_extensions());
}

}

vk::PhysicalDevice choose_physical_device(vk::Instance instance) {
	Logger::get_logger().print("Choosing physical device...");
	vk::PhysicalDevice chosen = nullptr;
	for (vk::PhysicalDevice device : instance.enumeratePhysicalDevices()) {
		vklog::log(device);
		if (!is_suitable(device)) {
			continue;
		}
		// Take the first suitable GPU, but let a discrete GPU (its own
		// memory, usually the fast one in a laptop with two) replace it.
		bool discrete = device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		if (!chosen || discrete) {
			chosen = device;
		}
	}
	if (!chosen) {
		throw std::runtime_error("No GPU supports what this renderer needs");
	}
	return chosen;
}
