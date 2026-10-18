// renderer.cpp: building and destroying the engine.
// Construction order = the order of the stages; destruction is the reverse,
// done by deletionQueue when the Engine is destroyed.
#include "renderer.h"
#include "device.h"
#include "instance.h"
#include "shader.h"
#include "command.h"
#include "surface.h"
#include "../logging/debug_messenger.h"
#include "../logging/logger.h"
#include <string>

Engine::Engine(GLFWwindow* window, int width, int height) : window(window) {
	Logger::get_logger().print("Made a graphics engine");
	make_instance_objects();
	make_device_objects();
	make_swapchain_objects(width, height);
	make_drawing_objects();
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
	// deletionQueue's destructor runs after this body, destroying the
	// Vulkan objects newest first.
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

void Engine::make_swapchain_objects(int width, int height) {
	swapchain.build(logicalDevice, physicalDevice, surface,
		static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), deletionQueue);
	// The swapchain may make MORE images than we asked for: always ask it.
	std::vector<vk::Image> images = logicalDevice.getSwapchainImagesKHR(swapchain.chain);
	frames.reserve(images.size());   // one allocation instead of several
	for (vk::Image image : images) {
		// emplace_back builds the Frame in place, calling
		// Frame(image, logicalDevice, format, deletionQueue).
		frames.emplace_back(image, logicalDevice, swapchain.format.format, deletionQueue);
	}
	Logger::get_logger().print("Swapchain has " + std::to_string(frames.size()) + " images");
}

void Engine::make_drawing_objects() {
	// SHADER_DIR is an absolute path set by CMakeLists.txt (the build
	// folder's shaders/), so the program finds its shaders whatever folder
	// it's started from. The author's "shaders/vertex.spv" only works when
	// the working directory happens to be the project folder (entry 21).
	shaders = make_shader_objects(logicalDevice,
		SHADER_DIR "/vertex.spv", SHADER_DIR "/fragment.spv", deletionQueue);
	// Stage 10: one pre-recorded command buffer per swapchain image.
	commandPool = make_command_pool(logicalDevice, graphicsQueueFamilyIndex, deletionQueue);
	for (Frame& frame : frames) {
		frame.set_command_buffer(allocate_command_buffer(logicalDevice, commandPool),
			shaders, swapchain.extent);
	}
	imageAcquiredFence = logicalDevice.createFence(vk::FenceCreateInfo());
	vk::Device device = logicalDevice;
	vk::Fence fence = imageAcquiredFence;
	deletionQueue.push([device, fence] { device.destroyFence(fence); });
}
