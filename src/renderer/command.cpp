/// @file command.cpp
/// @brief Making a command pool and allocating command buffers.
#include "command.h"
#include "../logging/logger.h"

vk::CommandPool make_command_pool(vk::Device logicalDevice, std::uint32_t queueFamilyIndex,
	DeletionQueue& deletionQueue) {
	// eResetCommandBuffer: each buffer from this pool may be reset and
	// re-recorded on its own. We record once for now; later stages re-record
	// every frame.
	const vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queueFamilyIndex);
	const vk::CommandPool pool = logicalDevice.createCommandPool(poolInfo);
	deletionQueue.push([logicalDevice, pool] {
		logicalDevice.destroyCommandPool(pool);   // also frees its buffers
		Logger::get_logger().print("Deleted command pool");
	});
	return pool;
}

vk::CommandBuffer allocate_command_buffer(vk::Device logicalDevice, vk::CommandPool commandPool) {
	// ePrimary: can be submitted to a queue. (Secondary buffers are run
	// from primary ones with vkCmdExecuteCommands.)
	const vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
	return logicalDevice.allocateCommandBuffers(allocInfo).at(0);
}
