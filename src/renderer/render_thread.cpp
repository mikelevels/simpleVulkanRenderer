// render_thread.cpp: what the render thread does, and how it is stopped.
#include "render_thread.h"
#include "renderer.h"
#include "../logging/logger.h"
#include <chrono>
#include <exception>

namespace {   // "anonymous namespace": visible in this .cpp file only

// The function the render thread runs, from its first line to its last.
// (The author's spawn_render_thread, from his main.cpp.)
void render_thread_main(GLFWwindow* window, const std::atomic<bool>* done) {
	// An exception that escapes a thread's function calls std::terminate()
	// and kills the whole program without a message, so catch and report.
	try {
		Engine engine(window);   // built and destroyed on THIS thread
		while (!done->load()) {
			// Later stages draw one frame here. For now there is nothing to
			// do, so sleep instead of spinning: the author's empty loop keeps
			// one CPU core at 100% (bugsExplained.md, entry 7).
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	catch (const std::exception& error) {
		Logger::get_logger().print(error.what());
	}
}

}

RenderThread::RenderThread(GLFWwindow* window)
	// std::thread's constructor STARTS the thread right away: from this line
	// on, render_thread_main runs at the same time as the code below.
	// `&done` passes a pointer: the thread must see the same flag, not a copy.
	: thread(render_thread_main, window, &done) {
}

RenderThread::~RenderThread() {
	done = true;     // 1. ask the thread to leave its loop
	thread.join();   // 2. wait until it has (the Engine is destroyed by then)
}
