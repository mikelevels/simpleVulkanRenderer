// renderer.h: the graphics engine. Empty for now; from Stage 2 on it owns
// every Vulkan object. It is created and destroyed ON THE RENDER THREAD
// (see render_thread.cpp), so all Vulkan work happens on that thread.
//
// The class is called Engine, as in the author's code, so you can compare
// our files with his videos side by side.
#pragma once
#include <GLFW/glfw3.h>

class Engine {
public:
	explicit Engine(GLFWwindow* window);
	~Engine();

	// Will own Vulkan objects: never copy it (rule of five).
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

private:
	// Not owned: the window belongs to main(). The engine will draw into it.
	GLFWwindow* window;
};
