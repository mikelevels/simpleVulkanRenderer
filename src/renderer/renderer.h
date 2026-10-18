// renderer.h: the graphics engine. It owns every Vulkan object, and is
// created, used and destroyed ON THE RENDER THREAD (see render_thread.cpp).
//
// The class is called Engine, as in the author's code, so you can compare
// our files with his videos side by side.
//
// Map of the renderer files (one file per topic):
//   renderer.cpp        the Engine: build everything, destroy everything
//   deletion_queue.h    remembers how to destroy each Vulkan object
//   dispatch.cpp        storage for Vulkan-Hpp's function pointers
//   instance.cpp        the Vulkan instance                       (Stage 2)
//   ../logging/debug_messenger.cpp  validation messages           (Stage 3)
//   device.cpp          choosing the GPU                          (Stage 4)
//   logical_device.cpp  queue families, the logical device        (Stage 5)
//   surface.cpp         Vulkan's view of the window               (Stage 6)
//   render_thread.cpp   the thread the Engine lives on            (Stage 1)
#pragma once
#include "deletion_queue.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

class Engine {
public:
	explicit Engine(GLFWwindow* window);
	~Engine();

	// Owns Vulkan objects: never copy it (rule of five).
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

private:
	// Not owned: the window belongs to main(). The engine will draw into it.
	GLFWwindow* window;

	// Declared FIRST so it is destroyed LAST: by the time it runs its
	// deleters, nothing else in the Engine still uses the objects.
	DeletionQueue deletionQueue;

	vk::Instance instance;
	// Stage 3: only created when validation is on (Debug builds).
	vk::DebugUtilsMessengerEXT debugMessenger;
	// Stage 6: the window, as Vulkan sees it.
	vk::SurfaceKHR surface;
	// Stage 4: the GPU. Not created by us, so nothing to destroy.
	vk::PhysicalDevice physicalDevice;
	// Stage 5: our connection to the GPU, and the queue we send work to.
	// Queues belong to the device: they are never destroyed separately.
	vk::Device logicalDevice;
	std::uint32_t graphicsQueueFamilyIndex{ 0 };
	vk::Queue graphicsQueue;
};
