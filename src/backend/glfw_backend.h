/// @file glfw_backend.h
/// @brief The window, made with GLFW.
///
/// The window has two jobs (from the Stage 1 video):
///   - the app reads its events (keys, mouse, the close button);
///   - the renderer draws into it (from Stage 6, through a Vulkan "surface").
/// So it is created first, in main(), and both of them get a pointer to it.
#pragma once
#include <GLFW/glfw3.h>

/// Owns GLFW itself and one window, and cleans both up in its destructor.
///
/// The author has a free function build_window() and calls glfwTerminate()
/// by hand at the end of main(); this class does the same work, but the
/// destructor guarantees the cleanup.
class GlfwWindow {
public:
	/// Start GLFW and open the window.
	/// @param width   width in screen coordinates
	/// @param height  height in screen coordinates
	/// @param name    the window's title
	/// @throws std::runtime_error if GLFW can't start or the window can't be made
	GlfwWindow(int width, int height, const char* name);
	/// Close the window and shut GLFW down.
	~GlfwWindow();

	// Rule of five: this class owns a raw pointer, so copying would destroy
	// the window twice. Moving is forbidden too: writing all four "= delete"s
	// says so explicitly (clang-tidy cppcoreguidelines-special-member-functions)
	// instead of relying on the rule that a deleted copy hides the moves.
	GlfwWindow(const GlfwWindow&) = delete;
	GlfwWindow& operator=(const GlfwWindow&) = delete;
	GlfwWindow(GlfwWindow&&) = delete;
	GlfwWindow& operator=(GlfwWindow&&) = delete;

	/// The raw pointer, for the GLFW calls made by the app and the renderer.
	GLFWwindow* get() const { return window; }

	/// The window's drawable width IN PIXELS, measured when it was made.
	///
	/// Screen coordinates (glfwGetWindowSize) and pixels (glfwGetFramebufferSize)
	/// differ on high-DPI screens; the swapchain needs pixels. Measured in the
	/// constructor because GLFW only allows these calls on the main thread, and
	/// the renderer runs on another one (bugsExplained.md, 19).
	int framebuffer_width() const { return framebufferWidth; }
	/// The window's drawable height in pixels; see framebuffer_width().
	int framebuffer_height() const { return framebufferHeight; }

private:
	GLFWwindow* window{ nullptr };   ///< The GLFW window; owned.
	int framebufferWidth{ 0 };       ///< Drawable width in pixels.
	int framebufferHeight{ 0 };      ///< Drawable height in pixels.
};
