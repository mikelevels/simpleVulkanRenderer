// app.cpp: the main thread's event loop.
#include "app.h"
#include "../logging/logger.h"

App::App(GLFWwindow* window) : window(window) {
}

void App::run() {
	// The window's events must be read, or Windows marks the program "Not
	// responding" and the close button never sets the flag that
	// glfwWindowShouldClose() reads. GLFW offers two ways:
	//   glfwPollEvents()  handle waiting events and return at once;
	//   glfwWaitEvents()  SLEEP until at least one event arrives.
	// The author polls. In a single-threaded program that also draws, polling
	// is right: the loop must come back around to draw the next frame. Here
	// the drawing happens on the render thread, so this loop has nothing to do
	// between events, and polling would spin one CPU core at 100%
	// (bugsExplained.md, entry 7). Waiting costs nothing.
	while (!glfwWindowShouldClose(window)) {
		glfwWaitEvents();
	}
	Logger::get_logger().print("Window closed.");
}
