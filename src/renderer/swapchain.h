/// @file swapchain.h
/// @brief The swapchain: the images that are shown in the window. (Stage 7)
///
/// We never draw straight into the window. The swapchain owns a few images
/// (3 on your PC); each frame we borrow one, draw into it, and hand it back
/// to be shown ("presented") while we draw the next one. The monitor shows
/// one image while another is being drawn, so you never see half a frame.
/// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#_wsi_swapchain
/// Official tutorial: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html
/// WebGPU counterpart: the surface configuration in MyLearnWebGPU\\MyNotes\\FirstColor.md
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

/// What a surface allows: image counts and sizes, pixel formats, present modes.
struct SurfaceDetails {
	vk::SurfaceCapabilitiesKHR capabilities;        ///< Image counts, sizes, transforms.
	std::vector<vk::SurfaceFormatKHR> formats;      ///< Pixel formats + color spaces.
	std::vector<vk::PresentModeKHR> presentModes;   ///< How images are queued for the screen.
};

/// The present mode we ask for first; FIFO is the fallback. (Stage 10b)
///
/// FIFO waits for the monitor's refresh, so the GPU draws only frames that
/// will be seen: 60 per second on a 60 Hz monitor, at 0.03 of a CPU core on
/// your PC. The author prefers Mailbox (later Immediate), which never waits:
/// 5,880 frames per second at 0.78 of a core, of which 60 reach the screen.
/// See MyNotes/stage10b-framePacing.md. The CMake option SVR_FORCE_PRESENT_MODE
/// (Fifo, Mailbox or Immediate) overrides it to repeat the measurement.
inline constexpr vk::PresentModeKHR preferredPresentMode =
#ifdef SVR_FORCE_PRESENT_MODE
	vk::PresentModeKHR::SVR_FORCE_PRESENT_MODE;
#else
	vk::PresentModeKHR::eFifo;
#endif

/// The swapchain handle plus the choices it was made with.
class Swapchain {
public:
	/// Create the swapchain for `surface` and push its destruction onto `deletionQueue`.
	/// @param logicalDevice   the device that will own the swapchain
	/// @param physicalDevice  the GPU, asked what the surface supports
	/// @param surface         the window surface to show images in
	/// @param width           the window's width IN PIXELS (used only where the
	///                        surface lets us choose; on Windows it decides)
	/// @param height          the window's height in pixels, likewise
	/// @param deletionQueue   where the swapchain's destruction is pushed
	void build(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
		vk::SurfaceKHR surface, std::uint32_t width, std::uint32_t height,
		DeletionQueue& deletionQueue);

	vk::SwapchainKHR chain;          ///< The swapchain itself.
	vk::SurfaceFormatKHR format;     ///< Pixel layout + color space of the images.
	vk::Extent2D extent;             ///< Size of the images in pixels.
	std::uint32_t imageCount{ 0 };   ///< How many images we ASKED for (a minimum).

private:
	/// Ask the GPU what `surface` supports, and log it.
	static SurfaceDetails query_surface_support(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
	/// B8G8R8A8 UNORM + sRGB color space if offered, else the first format.
	static vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& formats);
	/// preferredPresentMode if offered, else FIFO (always offered).
	static vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& presentModes);
	/// The surface's own size if it has one, else width x height clamped to the limits.
	static vk::Extent2D choose_extent(std::uint32_t width, std::uint32_t height,
		const vk::SurfaceCapabilitiesKHR& capabilities);
	/// One more than the minimum, capped by the maximum when there is one.
	static std::uint32_t choose_image_count(const vk::SurfaceCapabilitiesKHR& capabilities);
};
