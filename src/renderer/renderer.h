/// @file renderer.h
/// @brief Engine: the graphics engine, owner of every Vulkan object.
///
/// The Engine is created, used and destroyed ON THE RENDER THREAD (see
/// render_thread.cpp). The class is called Engine, as in the author's code,
/// so you can compare our files with his videos side by side.
///
/// Map of the renderer files (one file per topic):
///   - renderer.cpp        the Engine: build everything, destroy everything
///   - render_thread.cpp   the thread the Engine lives on             (Stage 1)
///   - deletion_queue.h    remembers how to destroy each Vulkan object
///   - dispatch.cpp        storage for Vulkan-Hpp's function pointers
///   - instance.cpp        the Vulkan instance                        (Stage 2)
///   - ../logging/debug_messenger.cpp  validation messages            (Stage 3)
///   - device.cpp          choosing the GPU                           (Stage 4)
///   - logical_device.cpp  queue families, the logical device         (Stage 5)
///   - surface.cpp         Vulkan's view of the window                (Stage 6)
///   - swapchain.cpp       the images shown in the window             (Stage 7)
///   - frame.cpp           what's needed to draw into one image       (Stage 7)
///   - image.cpp           image views, layout transitions            (Stages 8, 10)
///   - shader.cpp          shader objects                             (Stage 9)
///   - command.cpp         command pool and buffers                   (Stage 10)
///   - dynamic_state.cpp   drawing state for shader objects           (Stage 10)
///   - draw.cpp            Engine::draw: one frame on screen          (Stage 10)
///   - frame_timer.cpp     frames per second                          (Stage 10b)
///   - swapchain_rebuild.cpp  surviving a minimize                    (Stage 10 review)
#pragma once
#include "deletion_queue.h"
#include "frame.h"
#include "swapchain.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

struct GLFWwindow;

/// Builds every Vulkan object in its constructor and draws one frame per draw() call.
class Engine {
public:
	/// Build the whole renderer for `window`.
	/// @param window         the window to draw into; must outlive the Engine
	/// @param width          the window's drawable width in pixels
	/// @param height         the window's drawable height in pixels
	Engine(GLFWwindow* window, int width, int height);
	/// Wait for the GPU to finish, then destroy everything (via the deletion queues).
	~Engine();

	// Rule of five: the Engine owns Vulkan objects, so it is neither
	// copyable nor movable.
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine(Engine&&) = delete;
	Engine& operator=(Engine&&) = delete;

	/// Put one frame on screen (draw.cpp). Called in a loop by the render thread.
	void draw();

private:
	// The constructor's phases, in order.
	void make_instance_objects();                       ///< Stages 2, 3, 6
	void make_device_objects();                         ///< Stages 4, 5
	void make_drawing_objects();                        ///< Stages 9, 10
	void make_swapchain_objects(std::uint32_t width, std::uint32_t height);   ///< Stages 7, 8
	void record_frames();                               ///< Stage 10
	// One frame (draw.cpp) and recovering from a minimize (swapchain_rebuild.cpp).
	void draw_frame();                                  ///< acquire, submit, present
	std::uint32_t acquire_image();                      ///< Stage 10
	void present_image(std::uint32_t imageIndex);       ///< Stage 10
	bool rebuild_swapchain();                           ///< Stage 10 review

	GLFWwindow* window;   ///< Not owned: the window belongs to main().

	/// Destroys everything except the swapchain's objects, newest first.
	///
	/// Declared FIRST so it is destroyed LAST: by the time it runs its
	/// deleters, nothing else in the Engine still uses the objects.
	DeletionQueue deletionQueue;

	vk::Instance instance;                         ///< Stage 2
	vk::DebugUtilsMessengerEXT debugMessenger;     ///< Stage 3; only in Debug builds.
	vk::SurfaceKHR surface;                        ///< Stage 6: the window, as Vulkan sees it.
	vk::PhysicalDevice physicalDevice;             ///< Stage 4: the GPU; nothing to destroy.
	vk::Device logicalDevice;                      ///< Stage 5: our connection to the GPU.
	std::uint32_t graphicsQueueFamilyIndex{ 0 };   ///< Stage 5
	vk::Queue graphicsQueue;                       ///< Stage 5: destroyed with the device.
	std::vector<vk::ShaderEXT> shaders;            ///< Stage 9: { vertex, fragment }
	vk::CommandPool commandPool;                   ///< Stage 10: where command buffers come from.
	vk::Fence imageAcquiredFence;                  ///< Stage 10: "the acquired image is really free".

	/// Destroys the swapchain and everything made for it; flushed on rebuild.
	///
	/// Declared after deletionQueue so it is destroyed BEFORE it: the
	/// swapchain's objects must go before the device they were made from.
	DeletionQueue swapchainDeletionQueue;
	Swapchain swapchain;                           ///< Stage 7
	std::vector<Frame> frames;                     ///< Stage 7: one per swapchain image.
	bool swapchainOutOfDate{ false };              ///< Set when present/acquire say so.
};
