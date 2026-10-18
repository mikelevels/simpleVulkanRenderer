// logger.cpp: the one Logger, and how it prints.
#include "logger.h"
#include <iostream>

Logger& Logger::get_logger() {
	// A "function-local static" (sometimes called a Meyers singleton): the
	// object is built the first time this line runs and destroyed when the
	// program ends. Since C++11 its construction is thread-safe.
	// The author instead writes `if (!logger) logger = new Logger();`, which
	// is NOT thread-safe and never deletes the logger (bugsExplained.md, 4).
	static Logger logger;
	return logger;
}

void Logger::set_mode(bool mode) {
	enabled = mode;
}

bool Logger::is_enabled() const {
	return enabled;
}

void Logger::print(std::string_view message) {
	if (!enabled) {
		return;
	}
	// lock_guard locks the mutex here and unlocks it at the closing brace
	// (RAII again). A second thread calling print() waits on this line until
	// the first one has finished its whole line.
	std::lock_guard<std::mutex> lock(printMutex);
	// std::endl = '\n' + flush: the line reaches the console immediately,
	// which is what you want from a debug log if the program then crashes.
	std::cout << message << std::endl;
}
