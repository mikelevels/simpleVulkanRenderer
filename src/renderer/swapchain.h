// swapchain.h: the queue of images that are shown in the window.
//
// We never draw straight into the window. The swapchain owns a few images
// (3 on your PC); each frame we borrow one, draw into it, and hand it back
// to be shown ("presented") while we draw the next one. The monitor shows
// one image while another is being drawn, so you never see half a frame.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#_wsi_swapchain
// Official tutorial: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html
// WebGPU counterpart: the surface configuration in MyLearnWebGPU\MyNotes\FirstColor.md
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

// What the surface allows: image counts and sizes, pixel formats, present
// modes. The author's SurfaceDetails.
struct SurfaceDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

class Swapchain {
public:
	// Create the swapchain for `surface` and push its destruction onto
	// `deletionQueue`. width/height: the window's size IN PIXELS.
	void build(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
		vk::SurfaceKHR surface, std::uint32_t width, std::uint32_t height,
		DeletionQueue& deletionQueue);

	vk::SwapchainKHR chain;
	vk::SurfaceFormatKHR format;   // pixel layout + color space of the images
	vk::Extent2D extent;           // size of the images in pixels
	std::uint32_t imageCount{ 0 }; // how many images we ASKED for (minimum)

private:
	static SurfaceDetails query_surface_support(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
	static vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& formats);
	static vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& presentModes);
	static vk::Extent2D choose_extent(std::uint32_t width, std::uint32_t height,
		const vk::SurfaceCapabilitiesKHR& capabilities);
	static std::uint32_t choose_image_count(const vk::SurfaceCapabilitiesKHR& capabilities);
};
