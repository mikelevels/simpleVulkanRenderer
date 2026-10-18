// renderer.cpp: building and destroying the engine.
// Construction order = the order of the stages; destruction is the reverse,
// done by deletionQueue when the Engine is destroyed.
#include "renderer.h"
#include "instance.h"
#include "device.h"
#include "../logging/debug_messenger.h"
#include "../logging/logger.h"

Engine::Engine(GLFWwindow* window) : window(window) {
	Logger::get_logger().print("Made a graphics engine");
	instance = make_instance("Simple Vulkan Renderer", deletionQueue);
	if (enableValidation) {
		debugMessenger = make_debug_messenger(instance, deletionQueue);
	}
	physicalDevice = choose_physical_device(instance);
	graphicsQueueFamilyIndex = find_queue_family_index(physicalDevice, vk::QueueFlagBits::eGraphics);
	logicalDevice = create_logical_device(physicalDevice, graphicsQueueFamilyIndex, deletionQueue);
	graphicsQueue = logicalDevice.getQueue(graphicsQueueFamilyIndex, 0);
}

Engine::~Engine() {
	Logger::get_logger().print("Goodbye see you!");
	// deletionQueue's destructor runs after this body, destroying the
	// Vulkan objects newest first.
}
