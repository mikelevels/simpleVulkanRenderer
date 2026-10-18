// frame.h: everything used to draw into ONE swapchain image.
//
// The author makes one Frame per swapchain image. Stage 7: the image.
// Stage 8: a view of it. Stage 10: a command buffer whose commands (clear,
// draw the triangle, get ready for the screen) are recorded once, in advance,
// and submitted again every time this image comes up.
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <vector>

class Frame {
public:
	// Make a view of `image` and push its destruction onto `deletionQueue`.
	Frame(vk::Image image, vk::Device logicalDevice, vk::Format format,
		DeletionQueue& deletionQueue);

	// Stage 10: take `newCommandBuffer` and record this frame's drawing
	// commands into it. The author's set_command_buffer.
	void set_command_buffer(vk::CommandBuffer newCommandBuffer,
		const std::vector<vk::ShaderEXT>& shaders, vk::Extent2D frameSize);

	// Owned by the swapchain: destroying the swapchain destroys it.
	vk::Image image;
	// Stage 8: owned by us.
	vk::ImageView imageView;
	// Stage 10: freed with its command pool.
	vk::CommandBuffer commandBuffer;

private:
	void record_rendering(const std::vector<vk::ShaderEXT>& shaders, vk::Extent2D frameSize);
};
