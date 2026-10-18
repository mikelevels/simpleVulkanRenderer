// glfw_backend.h: the window, made with GLFW.
//
// The window has two jobs (from the Stage 1 video):
//   - the app reads its events (keys, mouse, the close button);
//   - the renderer draws into it (from Stage 6, through a Vulkan "surface").
// So it is created first, in main(), and both of them get a pointer to it.
#pragma once
#include <GLFW/glfw3.h>

// GlfwWindow owns GLFW itself and one window. The author has a free function
// build_window() and calls glfwTerminate() by hand at the end of main(); this
// class does the same work, but the destructor guarantees the cleanup.
class GlfwWindow {
public:
	// Starts GLFW and opens the window. Throws std::runtime_error on failure.
	GlfwWindow(int width, int height, const char* name);
	// Closes the window and shuts GLFW down.
	~GlfwWindow();

	// Owns a raw pointer: copying would destroy the window twice (rule of five).
	GlfwWindow(const GlfwWindow&) = delete;
	GlfwWindow& operator=(const GlfwWindow&) = delete;

	// The raw pointer, for the GLFW calls made by the app and the renderer.
	GLFWwindow* get() const { return window; }

private:
	GLFWwindow* window{ nullptr };
};
