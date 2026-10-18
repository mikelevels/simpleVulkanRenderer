// device.h: choosing a GPU (the "physical device").
//
// A physical device is a GPU that Vulkan can see. You don't create it, you
// pick one from the list and ask it what it can do (properties, features,
// extensions, queue families). From Stage 5 we make a "logical device" from
// it: our own connection to that GPU, through which everything else is made.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#devsandqueues-physical-device-enumeration
#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>

// The device extensions this renderer needs; grows stage by stage.
std::vector<const char*> required_device_extensions();

// Pick a GPU that supports everything we need, preferring a discrete GPU.
// Throws if there is none.
vk::PhysicalDevice choose_physical_device(vk::Instance instance);
