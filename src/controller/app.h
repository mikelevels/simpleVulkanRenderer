// app.h: the top-level controller. It runs on the MAIN thread and handles the
// window's events. (GLFW requires event handling on the main thread:
// https://www.glfw.org/docs/3.4/intro_guide.html#thread_safety)
#pragma once
#include <GLFW/glfw3.h>

class App {
public:
	// Remembers the window. Does NOT start the loop (see bugsExplained.md, 6).
	explicit App(GLFWwindow* window);

	// Handle events until the user closes the window.
	void run();

private:
	// Not owned: main() owns the window, the app only uses it.
	GLFWwindow* window;
};
