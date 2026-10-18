// instance.h: the Vulkan instance, the connection between our program and
// the Vulkan library. Everything else (devices, surfaces, the debug messenger)
// is created from it.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#initialization-instances
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>

// Create the instance and push its destruction onto `deletionQueue`.
// Throws if this computer's Vulkan is too old or a requirement is missing.
vk::Instance make_instance(const char* applicationName, DeletionQueue& deletionQueue);
