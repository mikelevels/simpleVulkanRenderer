// debug_messenger.h: receiving the validation layer's messages.
//
// The validation layer (VK_LAYER_KHRONOS_validation, from the Vulkan SDK)
// sits between our calls and the driver and checks every call against the
// spec's rules ("Valid Usage", the VUID-... codes in its messages). Vulkan
// itself does almost no error checking, for speed; the layer is how you find
// your mistakes. A "debug messenger" is the object that hands its messages to
// a function of ours.
// Docs: https://docs.vulkan.org/guide/latest/validation_overview.html
//       https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html
#pragma once
#include "../renderer/deletion_queue.h"
#include <vulkan/vulkan.hpp>

// What we ask for: warnings and errors, about anything. Shared by the
// messenger below and by instance creation (see instance.cpp).
vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_info();

// Create the messenger and push its destruction onto `deletionQueue`.
vk::DebugUtilsMessengerEXT make_debug_messenger(
	vk::Instance instance, DeletionQueue& deletionQueue);
