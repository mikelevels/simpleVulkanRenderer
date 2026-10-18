/// @file device.h
/// @brief Choosing a GPU (the physical device) and connecting to it (the logical device).
///
/// A physical device is a GPU that Vulkan can see. You don't create it, you
/// pick one from the list and ask it what it can do (properties, features,
/// extensions, queue families). From it we create a logical device: our
/// connection to that GPU, with the extensions and features WE turned on.
/// Every later object (swapchain, images, shaders, command pools) is created
/// from the logical device.
/// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#devsandqueues
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

/// The device extensions this renderer needs; the list grows stage by stage.
std::vector<const char*> required_device_extensions();

/// Pick a GPU that supports everything we need, preferring a discrete GPU. (Stage 4)
/// @throws std::runtime_error if no GPU qualifies
vk::PhysicalDevice choose_physical_device(vk::Instance instance);

/// Find the first queue family that can do `queueType` work. (Stages 5, 6)
/// @param physicalDevice  the GPU to look at
/// @param surface         if not null, the family must also be able to present to it
/// @param queueType       the kind of work, e.g. vk::QueueFlagBits::eGraphics
/// @throws std::runtime_error if no family qualifies
std::uint32_t find_queue_family_index(vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface, vk::QueueFlags queueType);

/// Create the logical device with one queue from `queueFamilyIndex`. (Stage 5)
///
/// Also fills the dispatcher's device level and pushes the device's
/// destruction onto `deletionQueue`.
vk::Device create_logical_device(vk::PhysicalDevice physicalDevice,
	std::uint32_t queueFamilyIndex, DeletionQueue& deletionQueue);
