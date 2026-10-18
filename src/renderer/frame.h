/// @file frame.h
/// @brief Frame: everything used to draw into ONE swapchain image.
///
/// The author makes one Frame per swapchain image. Stage 7: the image.
/// Stage 8: a view of it. Stage 10: a command buffer whose commands (clear,
/// draw the triangle, get ready for the screen) are recorded once, in advance,
/// and submitted again every time this image comes up.
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <vector>

/// One swapchain image, its view, and its pre-recorded drawing commands.
class Frame {
public:
	/// Make a view of `image` and push its destruction onto `deletionQueue`.
	Frame(vk::Image image, vk::Device logicalDevice, vk::Format format,
		DeletionQueue& deletionQueue);

	/// Take `newCommandBuffer` and record this frame's drawing commands into it.
	///
	/// The author's set_command_buffer. (Stage 10)
	void set_command_buffer(vk::CommandBuffer newCommandBuffer,
		const std::vector<vk::ShaderEXT>& shaders, vk::Extent2D frameSize);

	vk::Image image;                   ///< Owned by the swapchain: destroyed with it.
	vk::ImageView imageView;           ///< Owned by us. (Stage 8)
	vk::CommandBuffer commandBuffer;   ///< Recorded once, submitted every time. (Stage 10)
};
