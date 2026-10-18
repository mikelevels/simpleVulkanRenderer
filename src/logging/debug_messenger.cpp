// debug_messenger.cpp: the validation layer's messages go to our Logger.
#include "debug_messenger.h"
#include "logger.h"
#include <string>

namespace {

// Vulkan calls this function, possibly from ANY thread that makes Vulkan
// calls, so it must be thread-safe: Logger::print is.
// VKAPI_ATTR / VKAPI_CALL make it use the calling convention Vulkan expects.
// The parameter types are the C API's, exactly as PFN_vkDebugUtilsMessengerCallbackEXT
// declares them, so the function can be passed without a cast. The author
// casts with reinterpret_cast, which would also silence the compiler if the
// signature were WRONG, turning a compile error into a crash (entry 12).
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void*) {
	auto severityName = vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity));
	std::string line = "validation layer [" + severityName + "]: ";
	line += data->pMessage;
	Logger::get_logger().print(line);
	// VK_FALSE = don't abort the Vulkan call. The spec says applications
	// should always return VK_FALSE.
	return VK_FALSE;
}

}

vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_info() {
	using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
	using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
	// The author also asks for eVerbose, which floods the console with the
	// loader's internal chatter; warnings and errors are what we act on.
	return vk::DebugUtilsMessengerCreateInfoEXT(
		{},
		Severity::eWarning | Severity::eError,
		Type::eGeneral | Type::eValidation | Type::ePerformance,
		&debug_callback);
}

vk::DebugUtilsMessengerEXT make_debug_messenger(
	vk::Instance instance, DeletionQueue& deletionQueue) {
	vk::DebugUtilsMessengerEXT messenger =
		instance.createDebugUtilsMessengerEXT(make_debug_messenger_info());
	deletionQueue.push([instance, messenger] {
		instance.destroyDebugUtilsMessengerEXT(messenger);
		Logger::get_logger().print("Deleted debug messenger");
	});
	return messenger;
}
