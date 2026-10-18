// device.h: choosing a GPU (the "physical device"), and opening our own
// connection to it (the "logical device").
//
// A physical device is a GPU that Vulkan can see. You don't create it, you
// pick one from the list and ask it what it can do (properties, features,
// extensions, queue families). From it we create a logical device: our
// connection to that GPU, with the extensions and features WE turned on.
// Every later object (swapchain, images, shaders, command pools) is created
// from the logical device.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#devsandqueues
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

// The device extensions this renderer needs; grows stage by stage.
std::vector<const char*> required_device_extensions();

// Pick a GPU that supports everything we need, preferring a discrete GPU.
// Throws if there is none.
vk::PhysicalDevice choose_physical_device(vk::Instance instance);

// Stage 5: the index of the first queue family that can do `queueType` work
// and (Stage 6), if `surface` isn't null, present images to that surface.
// Throws if there is none.
std::uint32_t find_queue_family_index(vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface, vk::QueueFlags queueType);

// Stage 5: create the logical device with one queue from `queueFamilyIndex`,
// and push its destruction onto `deletionQueue`.
vk::Device create_logical_device(vk::PhysicalDevice physicalDevice,
	std::uint32_t queueFamilyIndex, DeletionQueue& deletionQueue);
