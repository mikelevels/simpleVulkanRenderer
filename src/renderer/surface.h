// surface.h: the window surface, Vulkan's view of our GLFW window.
//
// Vulkan doesn't know what a window is (it also runs on servers with no
// screen). A VkSurfaceKHR wraps the operating system's window (a Win32 HWND
// here) so that a swapchain (Stage 7) can show images in it. It comes from
// the instance extensions GLFW asked for in Stage 2: VK_KHR_surface and
// VK_KHR_win32_surface.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#_wsi_surface
// GLFW: https://www.glfw.org/docs/3.4/vulkan_guide.html#vulkan_surface
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>

struct GLFWwindow;   // a "forward declaration": enough to use a pointer to it

// Create the surface for `window` and push its destruction onto
// `deletionQueue`. Throws on failure.
vk::SurfaceKHR make_surface(vk::Instance instance, GLFWwindow* window,
	DeletionQueue& deletionQueue);
