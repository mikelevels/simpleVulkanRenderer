/// @file instance.h
/// @brief The Vulkan instance: our program's connection to the Vulkan library.
///
/// Everything else (devices, surfaces, the debug messenger) is created from it.
/// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#initialization-instances
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>

/// Whether the validation layer is on: yes in Debug builds, no in Release. (Stage 3)
///
/// NDEBUG ("no debug") is defined by the compiler in Release builds; the same
/// macro switches off assert(). The author ties validation to the logger's
/// on/off switch instead, so silencing the log also silently turns off error
/// checking (bugsExplained.md, entry 13).
#ifdef NDEBUG
inline constexpr bool enableValidation = false;
#else
inline constexpr bool enableValidation = true;
#endif

/// Create the instance and push its destruction onto `deletionQueue`.
/// @throws std::runtime_error if the Vulkan loader is older than 1.3, or an
///         extension or layer we need is missing
/// @throws vk::SystemError if vkCreateInstance fails
vk::Instance make_instance(const char* applicationName, DeletionQueue& deletionQueue);
