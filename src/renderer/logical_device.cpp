// logical_device.cpp: queue families and the logical device.
// (Declared in device.h; device.cpp holds the physical-device half.)
#include "device.h"
#include "../logging/logger.h"
#include "../logging/vulkan_logging.h"
#include <stdexcept>

// A GPU runs work from QUEUES. Queues come in FAMILIES: all queues of one
// family accept the same kinds of work (graphics, compute, transfer...).
// Your TITAN Xp has 5 families (see the log); family 0 does graphics,
// compute and transfer; 3 and 4 are video decode/encode.
std::uint32_t find_queue_family_index(vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface, vk::QueueFlags queueType) {
	std::vector<vk::QueueFamilyProperties> families = physicalDevice.getQueueFamilyProperties();
	vklog::log(families);
	for (std::uint32_t i = 0; i < families.size(); ++i) {
		// `flags & queueType` is non-zero when the family has the bit.
		bool canDoWork = static_cast<bool>(families[i].queueFlags & queueType);
		// Stage 6: getSurfaceSupportKHR returns a Bool32 that ANSWERS the
		// question. The author tests whether the CALL succeeded instead,
		// which is true for every family (bugsExplained.md, entry 15).
		bool canPresent = !surface || physicalDevice.getSurfaceSupportKHR(i, surface);
		if (canDoWork && canPresent) {
			return i;
		}
	}
	throw std::runtime_error("No queue family can do the requested work");
}

namespace {

// Create the device with the features we use turned on.
vk::Device create_with_features(vk::PhysicalDevice physicalDevice,
	const vk::DeviceCreateInfo& deviceInfo) {
	// Stage 9: feature structs are chained through pNext; vk::StructureChain
	// links them for us and checks at compile time that each struct is
	// allowed in this chain.
	// Stage 10: dynamic rendering, a Vulkan 1.3 core feature. The author
	// also enables the VK_KHR_dynamic_rendering and VK_KHR_synchronization2
	// EXTENSIONS, which are unnecessary on a 1.3 device (they were promoted
	// to core), and synchronization2, which Stage 10 doesn't use yet.
	vk::PhysicalDeviceVulkan13Features vulkan13;
	vulkan13.dynamicRendering = VK_TRUE;
	vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceShaderObjectFeaturesEXT> chain(
		deviceInfo, vulkan13, vk::PhysicalDeviceShaderObjectFeaturesEXT(VK_TRUE));
	return physicalDevice.createDevice(chain.get<vk::DeviceCreateInfo>());
}

}

vk::Device create_logical_device(vk::PhysicalDevice physicalDevice,
	std::uint32_t queueFamilyIndex, DeletionQueue& deletionQueue) {
	// One queue from that family. Priority (0..1) only matters between
	// queues of the same device; with one queue, any value works.
	float queuePriority = 1.0f;
	vk::DeviceQueueCreateInfo queueInfo({}, queueFamilyIndex, 1, &queuePriority);

	std::vector<const char*> extensions = required_device_extensions();
	// No layers here: "device layers" were deprecated in Vulkan 1.0.13; the
	// instance's layers apply to devices automatically. The author still
	// passes them, with a count of 1 even when the list is null (entry 16).
	vk::DeviceCreateInfo deviceInfo({}, queueInfo, {}, extensions);

	vk::Device device = create_with_features(physicalDevice, deviceInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);   // dispatcher level 3
	deletionQueue.push([device] {
		device.destroy();
		Logger::get_logger().print("Deleted logical device");
	});
	Logger::get_logger().print("GPU has been successfully abstracted!");
	return device;
}
