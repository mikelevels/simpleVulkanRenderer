// frame.h: everything used to draw into ONE swapchain image.
//
// The author makes one Frame per swapchain image. In Stage 7 a Frame only
// remembered its image; Stage 8 adds a view of it; Stage 10 adds a command
// buffer with the drawing commands recorded in advance.
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>

class Frame {
public:
	// Make a view of `image` and push its destruction onto `deletionQueue`.
	Frame(vk::Image image, vk::Device logicalDevice, vk::Format format,
		DeletionQueue& deletionQueue);

	// Owned by the swapchain: destroying the swapchain destroys it.
	vk::Image image;
	// Stage 8: owned by us.
	vk::ImageView imageView;
};
