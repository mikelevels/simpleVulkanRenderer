// main.cpp holds only main(). It builds the program's four parts, in order:
//   1. the logger    (prints debug messages; src/logging)
//   2. the window    (GLFW; src/backend)
//   3. the renderer  (runs on its OWN thread; src/renderer)
//   4. the app       (handles window events on the main thread; src/controller)
//
// Each part is a local variable. C++ destroys local variables in the REVERSE
// order of their creation when main() ends, so the shutdown order is
// automatically: app -> render thread (stopped and joined) -> window.
// That is the same order the author writes out by hand at the end of his
// main(), but here it also happens if something throws an exception.
#include "logging/logger.h"
#include "backend/glfw_backend.h"
#include "renderer/render_thread.h"
#include "controller/app.h"

int main() {
	Logger::get_logger().set_mode(true);

	GlfwWindow window(800, 600, "Simple Vulkan Renderer");
	RenderThread renderThread(window);
	App app(window.get());

	app.run();
	return 0;
}
