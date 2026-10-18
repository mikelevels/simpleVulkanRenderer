/// @file frame_timer.h
/// @brief Counts frames and reports frames per second (Stage 10b, frame pacing).
///
/// Before you change how fast the renderer runs, measure it. FrameTimer is
/// the measuring tape: the render loop calls tick() once per frame, and every
/// two seconds it logs how many frames were drawn and how long one took.
#pragma once
#include <chrono>
#include <cstdint>

/// Measures frames per second over two-second windows and logs the result.
class FrameTimer {
public:
	/// Count one finished frame; log a report when two seconds have passed.
	void tick();

private:
	/// steady_clock never jumps (unlike the wall clock, which can be changed
	/// by the user or by time synchronization), so it's the one for timing.
	using Clock = std::chrono::steady_clock;

	Clock::time_point windowStart{ Clock::now() };   ///< When the current window began.
	std::uint64_t framesInWindow{ 0 };               ///< Frames counted since windowStart.
};
