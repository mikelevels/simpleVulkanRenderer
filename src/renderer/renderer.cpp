// renderer.cpp: building and destroying the engine.
#include "renderer.h"
#include "../logging/logger.h"

Engine::Engine(GLFWwindow* window) : window(window) {
	Logger::get_logger().print("Made a graphics engine");
}

Engine::~Engine() {
	Logger::get_logger().print("Goodbye see you!");
}
