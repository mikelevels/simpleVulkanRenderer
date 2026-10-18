/// @file render_thread.cpp
/// @brief What the render thread does, and how it is stopped.
#include "render_thread.h"
#include "renderer.h"
#include "frame_timer.h"
#include "../logging/logger.h"
#include <exception>

namespace {   // "anonymous namespace": visible in this .cpp file only

/// The function the render thread runs, from its first line to its last.
/// (The author's spawn_render_thread, from his main.cpp.)
void render_thread_main(GLFWwindow* window, int width, int height,
	const std::atomic<bool>* done) {
	// An exception that escapes a thread's function calls std::terminate()
	// and kills the whole program without a message, so catch and report.
	try {
		Engine engine(window, width, height);   // built and destroyed on THIS thread
		FrameTimer timer;                        // Stage 10b: measure before changing
		while (!done->load()) {
			// Stage 10: one frame per loop. (Stage 1 slept here while there
			// was nothing to draw; see bugsExplained.md, entry 7.)
			engine.draw();
			timer.tick();
		}
	}
	catch (const std::exception& error) {
		Logger::get_logger().print(error.what());
	}
}

}

RenderThread::RenderThread(const GlfwWindow& window)
	// std::thread's constructor STARTS the thread right away: from this line
	// on, render_thread_main runs at the same time as the code below.
	// The arguments are COPIED into the new thread. The window's pixel size
	// is read here, on the main thread (GLFW's rule), and copied over.
	// `&done` passes a pointer: the thread must see the same flag, not a copy.
	: thread(render_thread_main, window.get(), window.framebuffer_width(),
		window.framebuffer_height(), &done) {
}

RenderThread::~RenderThread() {
	done = true;     // 1. ask the thread to leave its loop
	thread.join();   // 2. wait until it has (the Engine is destroyed by then)
}
