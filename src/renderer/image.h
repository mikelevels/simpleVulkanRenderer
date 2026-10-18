/// @file image.h
/// @brief Helpers for Vulkan images: views (Stage 8) and layout transitions (Stage 10).
///
/// A vk::Image is raw memory plus a description (size, format). Shaders and
/// rendering never use an image directly: they use an IMAGE VIEW, which says
/// which part of the image to use and how to interpret it (2D, which mip
/// levels, which array layers, which color channels).
/// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#resources-image-views
/// WebGPU counterpart: wgpu::TextureView (MyLearnWebGPU\\MyNotes\\FirstColor.md)
#pragma once
#include <vulkan/vulkan.hpp>

/// Make a 2D color view of the whole of `image`. The caller destroys it.
vk::ImageView create_image_view(vk::Device logicalDevice, vk::Image image, vk::Format format);

/// What an image layout transition needs to know. (Stage 10)
///
/// A LAYOUT is how the GPU arranges an image's pixels in memory; each use
/// (being drawn into, being shown on screen, being read by a shader) has its
/// own best layout, and using an image in the wrong layout is an error.
/// A transition is recorded as a "pipeline barrier" that also orders the work.
/// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#resources-image-layouts
struct LayoutTransition {
	vk::ImageLayout oldLayout;          ///< The layout the image is in now.
	vk::ImageLayout newLayout;          ///< The layout it must be in next.
	vk::AccessFlags srcAccess;          ///< Earlier memory writes to make visible.
	vk::AccessFlags dstAccess;          ///< Later memory accesses that need them.
	vk::PipelineStageFlags srcStage;    ///< Earlier work to wait for.
	vk::PipelineStageFlags dstStage;    ///< Later work that must wait.
};

/// Record `transition` of the whole color image `image` into `commandBuffer`.
void transition_image_layout(vk::CommandBuffer commandBuffer, vk::Image image,
	const LayoutTransition& transition);
