/// @file swapchain_rebuild.cpp
/// @brief Engine::rebuild_swapchain: making a new swapchain after a minimize.
///
/// A swapchain is made for one window size. When the window is minimized
/// (its size becomes 0 x 0) or resized from outside, Vulkan reports the
/// swapchain as "out of date" and refuses to use it again: it must be
/// destroyed and made anew. Before this fix the render thread treated that
/// as a fatal error and stopped, so the window froze after any minimize
/// (bugsExplained.md, entry 30).
///
/// The author teaches the full version, including user resizing, in Stage 15
/// ("Swapchain Recreation"). This is the minimum needed to survive a minimize;
/// Stage 15 will build on it.
/// Spec (VK_ERROR_OUT_OF_DATE_KHR): https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#vkQueuePresentKHR
#include "renderer.h"
#include "../logging/logger.h"

bool Engine::rebuild_swapchain() {
	// On Windows the surface itself reports the window's current size in
	// pixels, and we can ask from this thread (unlike GLFW, main thread only).
	const vk::Extent2D size = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent;
	if (size.width == 0 || size.height == 0) {
		return false;   // minimized: a swapchain can't have zero-sized images
	}
	// Nothing may still be using the old swapchain's images or command buffers.
	logicalDevice.waitIdle();
	swapchainDeletionQueue.flush();   // command buffers, image views, swapchain
	frames.clear();
	make_swapchain_objects(size.width, size.height);
	Logger::get_logger().print("Rebuilt the swapchain (the window was minimized or resized)");
	return true;
}
