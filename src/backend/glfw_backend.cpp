// glfw_backend.cpp: opening and closing the GLFW window.
// GLFW docs: https://www.glfw.org/docs/3.4/window_guide.html
#include "glfw_backend.h"
#include "../logging/logger.h"
#include <sstream>
#include <stdexcept>

GlfwWindow::GlfwWindow(int width, int height, const char* name) {
	// glfwInit() returns GLFW_FALSE on failure; the author ignores it.
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("glfwInit failed");
	}
	// GLFW makes an OpenGL context by default. Vulkan doesn't use one, and a
	// window that has one cannot get a Vulkan surface later (Stage 6).
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// Resizing changes the size of the swapchain images (Stage 7), which we
	// only learn to rebuild in Stage 15. Until then: a fixed size.
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, name, nullptr, nullptr);
	if (window == nullptr) {
		// The destructor will NOT run (the constructor didn't finish), so
		// undo glfwInit() here before leaving.
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow failed");
	}
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
	std::ostringstream line;
	line << "Made a window called \"" << name << "\", " << width << "x" << height;
	Logger::get_logger().print(line.str());
}

GlfwWindow::~GlfwWindow() {
	// Reverse order of the constructor: window first, then GLFW itself.
	glfwDestroyWindow(window);
	glfwTerminate();
}
