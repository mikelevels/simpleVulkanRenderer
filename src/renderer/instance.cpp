// instance.cpp: creating and destroying the Vulkan instance.
#include "instance.h"
#include "../logging/debug_messenger.h"
#include "../logging/logger.h"
#include "../logging/vulkan_logging.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <stdexcept>
#include <string>

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

// The instance extensions we need: GLFW's (drawing into a window on this
// operating system) and, with validation, the debug messenger's.
std::vector<const char*> required_extensions() {
	std::uint32_t count = 0;
	const char** names = glfwGetRequiredInstanceExtensions(&count);
	if (names == nullptr) {
		// GLFW returns NULL when it found no usable Vulkan on this machine.
		throw std::runtime_error("GLFW found no Vulkan support for windows");
	}
	// Copy the C array into a vector: (first pointer, one-past-last pointer).
	std::vector<const char*> extensions(names, names + count);
	if (enableValidation) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

std::vector<const char*> required_layers() {
	if (enableValidation) {
		return { "VK_LAYER_KHRONOS_validation" };
	}
	return {};
}

// Does `available` (what Vulkan reports) contain `wanted`? Works for both
// ExtensionProperties and LayerProperties through `nameOf`.
template <typename Properties, typename NameOf>
bool contains(const std::vector<Properties>& available, const char* wanted, NameOf nameOf) {
	for (const Properties& item : available) {
		if (std::strcmp(nameOf(item), wanted) == 0) {
			return true;
		}
	}
	return false;
}

// Stage 3: throw, naming the culprit, if an extension or layer is missing.
void check_support(const std::vector<const char*>& extensions,
	const std::vector<const char*>& layers) {
	auto supportedExtensions = vk::enumerateInstanceExtensionProperties();
	auto supportedLayers = vk::enumerateInstanceLayerProperties();
	Logger::get_logger().print("Instance can support these layers:");
	vklog::print_layers(supportedLayers);
	for (const char* name : extensions) {
		if (!contains(supportedExtensions, name, [](const auto& e) { return e.extensionName.data(); })) {
			throw std::runtime_error(std::string("Instance extension not supported: ") + name);
		}
	}
	for (const char* name : layers) {
		if (!contains(supportedLayers, name, [](const auto& l) { return l.layerName.data(); })) {
			throw std::runtime_error(std::string("Layer not supported: ") + name);
		}
	}
}

// Print what we are about to ask Vulkan for.
void log_requests(const std::vector<const char*>& extensions,
	const std::vector<const char*>& layers) {
	Logger& logger = Logger::get_logger();
	logger.print("Instance extensions to be requested:");
	vklog::print_list(extensions);
	logger.print("Layers to be requested:");
	vklog::print_list(layers);
}

// Create the instance from `createInfo` and fill the dispatcher's level 2.
vk::Instance create_instance(vk::InstanceCreateInfo createInfo) {
	// Chaining the messenger's settings into pNext lets the layer report
	// problems DURING vkCreateInstance / vkDestroyInstance too, which the real
	// messenger (created after the instance) cannot see.
	vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = make_debug_messenger_info();
	if (enableValidation) {
		createInfo.pNext = &messengerInfo;
	}
	// Without VULKAN_HPP_NO_EXCEPTIONS, createInstance throws vk::SystemError
	// on failure instead of returning a result code we could forget to check.
	vk::Instance instance = vk::createInstance(createInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);   // dispatcher level 2
	return instance;
}

}

vk::Instance make_instance(const char* applicationName, DeletionQueue& deletionQueue) {
	Logger::get_logger().print("Making an instance...");
	check_loader_version();

	// ApplicationInfo(appName, appVersion, engineName, engineVersion, apiVersion)
	// Only apiVersion matters to Vulkan; the rest is for driver statistics.
	vk::ApplicationInfo appInfo(applicationName, VK_MAKE_API_VERSION(0, 0, 1, 0),
		"Simple Vulkan Renderer", VK_MAKE_API_VERSION(0, 0, 1, 0), requiredApiVersion);

	std::vector<const char*> extensions = required_extensions();
	std::vector<const char*> layers = required_layers();
	log_requests(extensions, layers);
	check_support(extensions, layers);

	vk::Instance instance = create_instance(
		vk::InstanceCreateInfo({}, &appInfo, layers, extensions));
	deletionQueue.push([instance] {
		instance.destroy();
		Logger::get_logger().print("Deleted instance");
	});
	return instance;
}
