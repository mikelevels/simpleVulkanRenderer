/// @file swapchain.cpp
/// @brief Swapchain: asking the surface what it supports, choosing, creating.
#include "swapchain.h"
#include "../logging/logger.h"
#include "../logging/vulkan_logging.h"
#include <algorithm>
#include <stdexcept>

void Swapchain::build(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface, std::uint32_t width, std::uint32_t height,
	DeletionQueue& deletionQueue) {
	const SurfaceDetails support = query_surface_support(physicalDevice, surface);
	format = choose_surface_format(support.formats);
	extent = choose_extent(width, height, support.capabilities);
	imageCount = choose_image_count(support.capabilities);

	// SwapchainCreateInfoKHR(flags, surface, minImageCount, format,
	//     colorSpace, extent, arrayLayers, usage)
	// usage eColorAttachment: we will draw into these images (Stage 10).
	vk::SwapchainCreateInfoKHR createInfo({}, surface, imageCount, format.format,
		format.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment);
	// One queue family uses the images, so eExclusive (the default) is right.
	createInfo.preTransform = support.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;   // ignore alpha
	createInfo.presentMode = choose_present_mode(support.presentModes);
	createInfo.clipped = VK_TRUE;   // skip pixels hidden behind other windows

	chain = logicalDevice.createSwapchainKHR(createInfo);
	const vk::SwapchainKHR handle = chain;   // capture the handle, not `this`
	deletionQueue.push([logicalDevice, handle] {
		logicalDevice.destroySwapchainKHR(handle);
		Logger::get_logger().print("Deleted swapchain");
	});
}

SurfaceDetails Swapchain::query_surface_support(vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface) {
	SurfaceDetails support;
	support.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	support.formats = physicalDevice.getSurfaceFormatsKHR(surface);
	support.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	vklog::log(support.capabilities);
	vklog::log(support.formats);
	vklog::log(support.presentModes);
	return support;
}

vk::SurfaceFormatKHR Swapchain::choose_surface_format(
	const std::vector<vk::SurfaceFormatKHR>& formats) {
	// B8G8R8A8: 8 bits each of blue, green, red, alpha (Windows' native
	// order). UNORM: the bytes 0..255 map straight to 0.0..1.0. The author
	// picks UNORM, as the official tutorial's older versions did; SRGB would
	// make the GPU apply gamma correction for us. We keep his choice for now
	// so the colors match his videos (see stage07 notes, "UNORM vs SRGB").
	// std::find_if (from <algorithm>) returns an iterator to the first element
	// for which the lambda returns true, or formats.end() if there is none.
	// It replaces a hand-written search loop (Cppcheck's useStlAlgorithm).
	const auto wanted = std::find_if(formats.begin(), formats.end(),
		[](const vk::SurfaceFormatKHR& candidate) {
			return candidate.format == vk::Format::eB8G8R8A8Unorm
				&& candidate.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	if (wanted != formats.end()) {
		return *wanted;
	}
	return formats.at(0);   // .at() throws if the list is empty; [0] wouldn't
}

vk::PresentModeKHR Swapchain::choose_present_mode(
	const std::vector<vk::PresentModeKHR>& presentModes) {
	// Stage 10b: FIFO (wait for the monitor's refresh) is our default; the
	// author prefers Mailbox. See preferredPresentMode in swapchain.h and
	// MyNotes/stage10b-framePacing.md for the measurements behind the choice.
	// std::find (from <algorithm>) looks for one value; FIFO is the fallback
	// because it is the only mode every GPU must support.
	const bool offered = std::find(presentModes.begin(), presentModes.end(),
		preferredPresentMode) != presentModes.end();
	return offered ? preferredPresentMode : vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::choose_extent(std::uint32_t width, std::uint32_t height,
	const vk::SurfaceCapabilitiesKHR& capabilities) {
	// On Windows, currentExtent IS the window's size and must be used. The
	// special value 0xFFFFFFFF means "you choose" (some Linux systems).
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	// std::clamp(value, low, high) (C++17) keeps value inside [low, high].
	return vk::Extent2D(
		std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
}

std::uint32_t Swapchain::choose_image_count(const vk::SurfaceCapabilitiesKHR& capabilities) {
	// One more than the minimum, so we never wait on the driver for a free
	// image. maxImageCount == 0 means "no maximum"! The author's
	// std::min(max, min + 1) gives 0 in that case (bugsExplained.md, entry 18).
	std::uint32_t count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0) {
		count = std::min(count, capabilities.maxImageCount);
	}
	return count;
}
