/// @file logger.h
/// @brief Prints debug messages, and can be switched off in one place.
///
/// The logger is a "singleton": there is exactly one, reachable from anywhere
/// with Logger::get_logger(). Two threads use it from Stage 1 on (the main
/// thread and the render thread), so it must be thread-safe. See the notes,
/// MyNotes/stage01-appStructure.md, "C++ refresher" sections 5 to 7.
#pragma once
#include <atomic>
#include <mutex>
#include <string_view>

/// The program's one thread-safe message printer.
class Logger {
public:
	/// Return THE logger.
	///
	/// The first call creates it; C++ guarantees that this creation happens
	/// exactly once even if two threads call at the same time.
	static Logger& get_logger();

	/// Switch printing on (true) or off (false).
	void set_mode(bool mode);
	/// Whether printing is on.
	bool is_enabled() const;

	/// Print one line, if enabled. Safe to call from any thread.
	void print(std::string_view message);

	// Rule of five: a singleton must be neither copied nor moved (either
	// would make a second logger). The destructor is the default one.
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger&&) = delete;
	~Logger() = default;

private:
	/// Private: only get_logger() can make a Logger.
	Logger() = default;

	/// Whether print() prints.
	///
	/// std::atomic: one thread may switch the logger on or off while another
	/// thread reads the flag. A plain bool would be a "data race" (undefined
	/// behavior); an atomic bool is always read and written whole.
	std::atomic<bool> enabled{ false };

	std::mutex printMutex;   ///< Only one thread at a time may write a line.
};
