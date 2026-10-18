/// @file app.h
/// @brief The top-level controller: the main thread's window-event loop.
///
/// GLFW requires event handling on the main thread:
/// https://www.glfw.org/docs/3.4/intro_guide.html#thread_safety
#pragma once
#include <GLFW/glfw3.h>

/// Handles the window's events on the main thread until the window closes.
class App {
public:
	/// Remember the window. Does NOT start the loop (bugsExplained.md, 6).
	explicit App(GLFWwindow* window);

	/// Handle events until the user closes the window.
	void run();

private:
	GLFWwindow* window;   ///< Not owned: main() owns the window, the app only uses it.
};
