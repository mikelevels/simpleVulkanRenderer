// renderer.cpp: building and destroying the engine.
// Construction order = the order of the stages; destruction is the reverse,
// done by deletionQueue when the Engine is destroyed.
#include "renderer.h"
#include "device.h"
#include "instance.h"
#include "surface.h"
#include "../logging/debug_messenger.h"
#include "../logging/logger.h"
#include <string>

Engine::Engine(GLFWwindow* window, int width, int height) : window(window) {
	Logger::get_logger().print("Made a graphics engine");
	make_instance_objects();
	make_device_objects();
	make_swapchain_objects(width, height);
}

Engine::~Engine() {
	Logger::get_logger().print("Goodbye see you!");
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
		frames.emplace_back(image);  // builds the Frame in place: Frame(image)
	}
	Logger::get_logger().print("Swapchain has " + std::to_string(frames.size()) + " images");
}
