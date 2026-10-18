// image.h: helpers for Vulkan images.
//
// A vk::Image is raw memory plus a description (size, format). Shaders and
// rendering never use an image directly: they use an IMAGE VIEW, which says
// which part of the image to use and how to interpret it (2D, which mip
// levels, which array layers, which color channels).
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#resources-image-views
// WebGPU counterpart: wgpu::TextureView (MyLearnWebGPU\MyNotes\FirstColor.md)
#pragma once
#include <vulkan/vulkan.hpp>

// Make a 2D color view of the whole of `image`. The caller destroys it.
vk::ImageView create_image_view(vk::Device logicalDevice, vk::Image image, vk::Format format);

// Stage 10: what an image layout transition needs to know.
// A LAYOUT is how the GPU arranges an image's pixels in memory; each use
// (being drawn into, being shown on screen, being read by a shader) has its
// own best layout, and using an image in the wrong layout is an error.
// A transition is recorded as a "pipeline barrier" that also orders the work:
//   src stage/access: the earlier work (and its memory writes) to wait for;
//   dst stage/access: the later work that must wait.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#resources-image-layouts
struct LayoutTransition {
	vk::ImageLayout oldLayout;
	vk::ImageLayout newLayout;
	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;
	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;
};

// Record `transition` of the whole color image `image` into `commandBuffer`.
void transition_image_layout(vk::CommandBuffer commandBuffer, vk::Image image,
	const LayoutTransition& transition);
