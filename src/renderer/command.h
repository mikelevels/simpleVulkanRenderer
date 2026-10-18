// command.h: command pools and command buffers.
//
// The GPU doesn't run functions we call one by one. We RECORD commands
// ("clear this image", "draw 3 vertices") into a command buffer, then
// SUBMIT the whole buffer to a queue. Command buffers come from a command
// pool, which owns their memory.
// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#commandbuffers
// WebGPU counterpart: wgpu::CommandEncoder / CommandBuffer (MyLearnWebGPU\MyNotes\TheCommandQueue.md)
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>

// Make a pool for command buffers that will be submitted to queues of
// family `queueFamilyIndex`, and push its destruction onto `deletionQueue`.
vk::CommandPool make_command_pool(vk::Device logicalDevice, std::uint32_t queueFamilyIndex,
	DeletionQueue& deletionQueue);

// Allocate one primary command buffer from `commandPool`. It is freed when
// the pool is destroyed.
vk::CommandBuffer allocate_command_buffer(vk::Device logicalDevice, vk::CommandPool commandPool);
