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
	vk::QueueFlags queueType) {
	std::vector<vk::QueueFamilyProperties> families = physicalDevice.getQueueFamilyProperties();
	vklog::log(families);
	for (std::uint32_t i = 0; i < families.size(); ++i) {
		// `flags & queueType` is non-zero when the family has the bit.
		if (families[i].queueFlags & queueType) {
			return i;
		}
	}
	throw std::runtime_error("No queue family can do the requested work");
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

	vk::Device device = physicalDevice.createDevice(deviceInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);   // dispatcher level 3
	deletionQueue.push([device] {
		device.destroy();
		Logger::get_logger().print("Deleted logical device");
	});
	Logger::get_logger().print("GPU has been successfully abstracted!");
	return device;
}
