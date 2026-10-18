// instance.h: the Vulkan instance, the connection between our program and
// the Vulkan library. Everything else (devices, surfaces, the debug messenger)
// is created from it.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#initialization-instances
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>

// Stage 3: turn the validation layer on in Debug builds, off in Release.
// NDEBUG ("no debug") is defined by the compiler in Release builds; the same
// macro switches off assert(). The author ties validation to the logger's
// on/off switch instead, so silencing the log also silently turns off error
// checking (bugsExplained.md, entry 13).
#ifdef NDEBUG
inline constexpr bool enableValidation = false;
#else
inline constexpr bool enableValidation = true;
#endif

// Create the instance and push its destruction onto `deletionQueue`.
// Throws if this computer's Vulkan is too old or a requirement is missing.
vk::Instance make_instance(const char* applicationName, DeletionQueue& deletionQueue);
