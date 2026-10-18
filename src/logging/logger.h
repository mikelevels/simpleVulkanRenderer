// logger.h: prints debug messages, and can be switched off in one place.
//
// The logger is a "singleton": there is exactly one, reachable from anywhere
// with Logger::get_logger(). Two threads use it from Stage 1 on (the main
// thread and the render thread), so it must be thread-safe. See the notes,
// MyNotes/stage01-appStructure.md, "C++ refresher" sections 4 and 5.
#pragma once
#include <atomic>
#include <mutex>
#include <string_view>

class Logger {
public:
	// Returns THE logger. The first call creates it; C++ guarantees that this
	// creation happens exactly once even if two threads call at the same time.
	static Logger& get_logger();

	// true = print messages, false = stay silent.
	void set_mode(bool mode);
	bool is_enabled() const;

	// Print one line, if enabled. Safe to call from any thread.
	void print(std::string_view message);

	// A singleton must not be copied: a copy would be a second logger.
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

private:
	// Private: only get_logger() can make a Logger.
	Logger() = default;

	// std::atomic: one thread may switch the logger on or off while another
	// thread reads the flag. A plain bool would be a "data race" (undefined
	// behavior); an atomic bool is always read and written whole.
	std::atomic<bool> enabled{ false };

	// Only one thread at a time may write a line, so lines don't get mixed.
	std::mutex printMutex;
};
