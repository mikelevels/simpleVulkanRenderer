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
