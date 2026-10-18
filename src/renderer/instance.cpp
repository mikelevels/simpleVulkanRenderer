// instance.cpp: creating and destroying the Vulkan instance.
#include "instance.h"
#include "../logging/logger.h"
#include "../logging/vulkan_logging.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace {

// The Vulkan version this program is written for. Stage 10 uses features
// that became core in Vulkan 1.3 (dynamic rendering, synchronization2).
// The author instead asks the loader for ITS version and requests that
// (1.4 on your PC); see bugsExplained.md, entry 10.
constexpr std::uint32_t requiredApiVersion = VK_API_VERSION_1_3;

// Fill the dispatcher's first level and check that the loader is new enough.
void check_loader_version() {
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
	// vkEnumerateInstanceVersion exists since Vulkan 1.1; the loader on any
	// current driver has it.
	std::uint32_t loaderVersion = vk::enumerateInstanceVersion();
	vklog::report_version_number(loaderVersion);
	if (loaderVersion < requiredApiVersion) {
		throw std::runtime_error("The Vulkan loader is older than 1.3");
	}
}

// The instance extensions we need. For now only GLFW's: the ones that let
// Vulkan draw into a window on this operating system.
std::vector<const char*> required_extensions() {
	std::uint32_t count = 0;
	const char** names = glfwGetRequiredInstanceExtensions(&count);
	if (names == nullptr) {
		// GLFW returns NULL when it found no usable Vulkan on this machine.
		throw std::runtime_error("GLFW found no Vulkan support for windows");
	}
	// Copy the C array into a vector: (first pointer, one-past-last pointer).
	return std::vector<const char*>(names, names + count);
}

}

vk::Instance make_instance(const char* applicationName, DeletionQueue& deletionQueue) {
	Logger& logger = Logger::get_logger();
	logger.print("Making an instance...");
	check_loader_version();

	// ApplicationInfo(appName, appVersion, engineName, engineVersion, apiVersion)
	// Only apiVersion matters to Vulkan; the rest is for driver statistics.
	vk::ApplicationInfo appInfo(applicationName, VK_MAKE_API_VERSION(0, 0, 1, 0),
		"Simple Vulkan Renderer", VK_MAKE_API_VERSION(0, 0, 1, 0), requiredApiVersion);

	std::vector<const char*> extensions = required_extensions();
	logger.print("Instance extensions to be requested:");
	vklog::print_list(extensions);

	vk::InstanceCreateInfo createInfo({}, &appInfo, {}, extensions);
	// Without VULKAN_HPP_NO_EXCEPTIONS, createInstance throws vk::SystemError
	// on failure instead of returning a result code we could forget to check.
	vk::Instance instance = vk::createInstance(createInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);   // dispatcher level 2

	deletionQueue.push([instance] {
		instance.destroy();
		Logger::get_logger().print("Deleted instance");
	});
	return instance;
}
