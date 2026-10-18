/// @file frame.cpp
/// @brief Building a Frame and recording its commands.
#include "frame.h"
#include "dynamic_state.h"
#include "image.h"

namespace {

/// Record "clear `imageView`, then draw the triangle into it". (Stage 10)
///
/// A free function rather than a Frame method: it only needs these four
/// values. (clang-tidy pointed out that the method could be const; a const
/// method that records GPU commands would read as "changes nothing", which
/// is misleading, so we moved it out of the class instead.)
void record_rendering(vk::CommandBuffer commandBuffer, vk::ImageView imageView,
	const std::vector<vk::ShaderEXT>& shaders, vk::Extent2D frameSize) {
	// DYNAMIC RENDERING (core in Vulkan 1.3): begin drawing straight into
	// image views, described right here. Before it, you had to create
	// VkRenderPass and VkFramebuffer objects in advance.
	vk::RenderingAttachmentInfo colorAttachment;
	colorAttachment.imageView = imageView;
	colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;    // start from the clear color
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;  // keep the result
	colorAttachment.clearValue = vk::ClearColorValue(0.5f, 0.0f, 0.25f, 1.0f);   // dark purple
	// RenderingInfo(flags, renderArea, layerCount, viewMask, colorAttachments)
	const vk::RenderingInfo renderingInfo({}, vk::Rect2D({ 0, 0 }, frameSize), 1, 0, colorAttachment);

	commandBuffer.beginRendering(renderingInfo);
	set_default_dynamic_state(commandBuffer, frameSize);
	const vk::ShaderStageFlagBits stages[2] = { vk::ShaderStageFlagBits::eVertex,
		vk::ShaderStageFlagBits::eFragment };
	commandBuffer.bindShadersEXT(stages, shaders);
	commandBuffer.draw(3, 1, 0, 0);   // 3 vertices, 1 instance, starting at 0
	commandBuffer.endRendering();
}

}

Frame::Frame(vk::Image image, vk::Device logicalDevice, vk::Format format,
	DeletionQueue& deletionQueue)
	// Both members are set in the initializer list, before the body runs
	// (Cppcheck's useInitializationList: assigning in the body would first
	// default-construct imageView, then overwrite it).
	: image(image), imageView(create_image_view(logicalDevice, image, format)) {
	// Capture copies of the two handles. The author converts the view to the
	// C type VkImageView first; vk:: handles copy just as cheaply (they hold
	// one pointer-sized value), so there's no need.
	const vk::ImageView view = imageView;
	deletionQueue.push([logicalDevice, view] {
		logicalDevice.destroyImageView(view);
	});
}

void Frame::set_command_buffer(vk::CommandBuffer newCommandBuffer,
	const std::vector<vk::ShaderEXT>& shaders, vk::Extent2D frameSize) {
	using Stage = vk::PipelineStageFlagBits;
	using Access = vk::AccessFlagBits;
	commandBuffer = newCommandBuffer;
	commandBuffer.begin(vk::CommandBufferBeginInfo());

	// 1. Undefined -> attachment: "we don't care what was in it before" (it
	//    will be cleared), ready to be drawn into. Waits for nothing earlier
	//    (top of pipe); color writes wait for it.
	transition_image_layout(commandBuffer, image, { vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal, Access::eNone,
		Access::eColorAttachmentWrite, Stage::eTopOfPipe, Stage::eColorAttachmentOutput });

	record_rendering(commandBuffer, imageView, shaders, frameSize);   // 2. clear + draw

	// 3. Attachment -> present source: the layout the screen reads from.
	//    The color writes must finish first; nothing later in this buffer
	//    waits (bottom of pipe).
	transition_image_layout(commandBuffer, image, { vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR, Access::eColorAttachmentWrite,
		Access::eNone, Stage::eColorAttachmentOutput, Stage::eBottomOfPipe });
	commandBuffer.end();
}
