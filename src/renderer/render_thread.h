/// @file render_thread.h
/// @brief Runs the Engine on a second thread.
///
/// Why a second thread? The main thread is busy with window events (App). The
/// author's plan for the series is that all drawing happens on this background
/// thread, so slow event handling never delays a frame, and vice versa.
///
/// This is only the skeleton: a thread that starts, draws until a "done" flag
/// is set, and stops. The real multithreading topics (sharing work between
/// threads, synchronizing them) come in Stage 13, where we stop for your
/// review first.
#pragma once
#include "../backend/glfw_backend.h"
#include <atomic>
#include <thread>

/// Starts the render thread in its constructor and stops it in its destructor.
///
/// The author does the same with a std::thread and an std::atomic<bool> written
/// out in main(); wrapping them in a class guarantees the stop + join even when
/// an exception leaves main() early (bugsExplained.md, entry 5).
class RenderThread {
public:
	/// Start the render thread, which builds an Engine for `window`.
	explicit RenderThread(const GlfwWindow& window);
	/// Set `done`, then wait for the thread to finish.
	~RenderThread();

	// Rule of five: a running thread can be neither copied nor moved here
	// (std::thread can move, std::atomic can't; saying so documents intent).
	RenderThread(const RenderThread&) = delete;
	RenderThread& operator=(const RenderThread&) = delete;
	RenderThread(RenderThread&&) = delete;
	RenderThread& operator=(RenderThread&&) = delete;

private:
	// ORDER MATTERS: members are constructed in the order they are DECLARED
	// here, and destroyed in reverse. `done` must exist before the thread
	// starts reading it, and must outlive the thread. So `done` comes first.
	std::atomic<bool> done{ false };   ///< Set by the destructor to stop the loop.
	std::thread thread;                ///< The render thread itself.
};
