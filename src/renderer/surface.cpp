/// @file surface.cpp
/// @brief Creating and destroying the window surface.
///
/// Vulkan must be included BEFORE GLFW: glfw3.h only declares
/// glfwCreateWindowSurface when it sees that Vulkan's header came first.
#include "surface.h"
#include "../logging/logger.h"
#include <GLFW/glfw3.h>

vk::SurfaceKHR make_surface(vk::Instance instance, GLFWwindow* window,
	DeletionQueue& deletionQueue) {
	// GLFW's function is plain C, so it uses the C types: VkInstance in,
	// VkSurfaceKHR out. vk::Instance converts to VkInstance automatically.
	// glfwCreateWindowSurface may be called from any thread (GLFW docs),
	// which matters because we're on the render thread.
	VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
	const VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &rawSurface);
	// The author ignores this result (bugsExplained.md, entry 17). We throw
	// the same exception type Vulkan-Hpp throws: vk::SystemError, whose
	// message includes the error's name (e.g. ErrorInitializationFailed).
	if (result != VK_SUCCESS) {
		throw vk::SystemError(vk::make_error_code(static_cast<vk::Result>(result)),
			"glfwCreateWindowSurface");
	}
	const vk::SurfaceKHR surface = rawSurface;   // wrap the C handle
	deletionQueue.push([instance, surface] {
		instance.destroySurfaceKHR(surface);
		Logger::get_logger().print("Deleted surface");
	});
	return surface;
}
