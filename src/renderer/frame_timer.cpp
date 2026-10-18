/// @file frame_timer.cpp
/// @brief FrameTimer: counting frames and logging frames per second.
#include "frame_timer.h"
#include "../logging/logger.h"
#include <sstream>

void FrameTimer::tick() {
	++framesInWindow;
	const Clock::time_point now = Clock::now();
	// duration<double> = seconds as a floating-point number.
	const std::chrono::duration<double> elapsed = now - windowStart;
	if (elapsed.count() < 2.0) {
		return;
	}
	const double fps = static_cast<double>(framesInWindow) / elapsed.count();
	std::ostringstream line;
	line.precision(3);   // 3 significant digits are plenty for a rate
	line << fps << " frames per second, " << 1000.0 / fps << " ms per frame";
	Logger::get_logger().print(line.str());
	windowStart = now;
	framesInWindow = 0;
}
