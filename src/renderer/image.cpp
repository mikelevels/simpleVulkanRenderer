// image.cpp: image views.
#include "image.h"

vk::ImageView create_image_view(vk::Device logicalDevice, vk::Image image, vk::Format format) {
	vk::ImageViewCreateInfo createInfo;
	createInfo.image = image;
	createInfo.viewType = vk::ImageViewType::e2D;
	createInfo.format = format;
	// Components: eIdentity = red reads red, green reads green... (a view
	// could swap channels; we don't). Vulkan-Hpp already defaults to this,
	// the author sets it explicitly to show it exists.
	createInfo.components = vk::ComponentMapping();
	// subresourceRange: WHICH part of the image the view covers.
	// eColor aspect (not depth/stencil), mip level 0 only, layer 0 only.
	createInfo.subresourceRange = vk::ImageSubresourceRange(
		vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
	return logicalDevice.createImageView(createInfo);
}

void transition_image_layout(vk::CommandBuffer commandBuffer, vk::Image image,
	const LayoutTransition& transition) {
	vk::ImageMemoryBarrier barrier;
	barrier.srcAccessMask = transition.srcAccess;
	barrier.dstAccessMask = transition.dstAccess;
	barrier.oldLayout = transition.oldLayout;
	barrier.newLayout = transition.newLayout;
	// IGNORED: the image stays with the same queue family.
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = vk::ImageSubresourceRange(
		vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
	// pipelineBarrier(srcStages, dstStages, flags, memory barriers,
	//     buffer barriers, image barriers)
	commandBuffer.pipelineBarrier(transition.srcStage, transition.dstStage, {},
		nullptr, nullptr, barrier);
}
