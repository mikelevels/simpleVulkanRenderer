/// @file renderer.cpp
/// @brief Building and destroying the Engine.
///
/// Construction order = the order of the stages; destruction is the reverse,
/// done by the deletion queues when the Engine is destroyed.
#include "renderer.h"
#include "command.h"
#include "device.h"
#include "instance.h"
#include "shader.h"
#include "surface.h"
#include "../logging/debug_messenger.h"
#include "../logging/logger.h"
#include <string>

Engine::Engine(GLFWwindow* window, int width, int height) : window(window) {
	Logger::get_logger().print("Made a graphics engine");
	make_instance_objects();
	make_device_objects();
	make_drawing_objects();
	make_swapchain_objects(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
}

Engine::~Engine() {
	Logger::get_logger().print("Goodbye see you!");
	// Destroying objects the GPU is still using is an error, so wait until
	// it has finished everything we submitted. A destructor must not throw
	// (that would call std::terminate during stack unwinding), hence the try.
	try {
		if (logicalDevice) {
			logicalDevice.waitIdle();
		}
	}
	catch (const std::exception& error) {
		Logger::get_logger().print(error.what());
	}
	// The members' destructors run after this body: swapchainDeletionQueue
	// first (declared last), then deletionQueue, each newest first.
}

void Engine::make_instance_objects() {
	instance = make_instance("Simple Vulkan Renderer", deletionQueue);
	if (enableValidation) {
		debugMessenger = make_debug_messenger(instance, deletionQueue);
	}
	surface = make_surface(instance, window, deletionQueue);
}

void Engine::make_device_objects() {
	physicalDevice = choose_physical_device(instance);
	graphicsQueueFamilyIndex = find_queue_family_index(
		physicalDevice, surface, vk::QueueFlagBits::eGraphics);
	logicalDevice = create_logical_device(physicalDevice, graphicsQueueFamilyIndex, deletionQueue);
	graphicsQueue = logicalDevice.getQueue(graphicsQueueFamilyIndex, 0);
}

void Engine::make_drawing_objects() {
	// SHADER_DIR is an absolute path set by CMakeLists.txt (the build
	// folder's shaders/), so the program finds its shaders whatever folder
	// it's started from. The author's "shaders/vertex.spv" only works when
	// the working directory happens to be the project folder (entry 21).
	shaders = make_shader_objects(logicalDevice,
		SHADER_DIR "/vertex.spv", SHADER_DIR "/fragment.spv", deletionQueue);
	commandPool = make_command_pool(logicalDevice, graphicsQueueFamilyIndex, deletionQueue);
	imageAcquiredFence = logicalDevice.createFence(vk::FenceCreateInfo());
	const vk::Device device = logicalDevice;
	const vk::Fence fence = imageAcquiredFence;
	deletionQueue.push([device, fence] { device.destroyFence(fence); });
}

void Engine::make_swapchain_objects(std::uint32_t width, std::uint32_t height) {
	// Everything here goes on swapchainDeletionQueue, so a rebuild
	// (swapchain_rebuild.cpp) can destroy exactly this group and make it again.
	swapchain.build(logicalDevice, physicalDevice, surface, width, height, swapchainDeletionQueue);
	// The swapchain may make MORE images than we asked for: always ask it.
	const std::vector<vk::Image> images = logicalDevice.getSwapchainImagesKHR(swapchain.chain);
	frames.reserve(images.size());   // one allocation instead of several
	for (const vk::Image image : images) {
		// emplace_back builds the Frame in place, calling
		// Frame(image, logicalDevice, format, swapchainDeletionQueue).
		// Cppcheck suggests std::transform here; a plain loop that builds
		// objects with side effects (deleters pushed) reads more honestly.
		// cppcheck-suppress useStlAlgorithm
		frames.emplace_back(image, logicalDevice, swapchain.format.format, swapchainDeletionQueue);
	}
	Logger::get_logger().print("Swapchain has " + std::to_string(frames.size()) + " images");
	record_frames();
}

void Engine::record_frames() {
	// One pre-recorded command buffer per swapchain image. Each is freed with
	// the swapchain group, so a rebuild doesn't pile up unused buffers.
	for (Frame& frame : frames) {
		const vk::CommandBuffer commandBuffer = allocate_command_buffer(logicalDevice, commandPool);
		const vk::Device device = logicalDevice;
		const vk::CommandPool pool = commandPool;
		swapchainDeletionQueue.push([device, pool, commandBuffer] {
			device.freeCommandBuffers(pool, commandBuffer);
		});
		frame.set_command_buffer(commandBuffer, shaders, swapchain.extent);
	}
}
