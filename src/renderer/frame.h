// frame.h: everything used to draw into ONE swapchain image.
//
// The author makes one Frame per swapchain image. In Stage 7 a Frame only
// remembers its image; Stage 8 adds a view of it, Stage 10 a command buffer
// with the drawing commands recorded in advance.
#pragma once
#include <vulkan/vulkan.hpp>

class Frame {
public:
	explicit Frame(vk::Image image);

	// Owned by the swapchain: destroying the swapchain destroys it.
	vk::Image image;
};
