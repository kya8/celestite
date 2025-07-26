#ifndef CLST_TIMER_HPP
#define CLST_TIMER_HPP

#include <chrono>

namespace clst {

template<typename Duration = std::chrono::duration<double, std::ratio<1>>, typename Clock = std::chrono::steady_clock>
class Timer {
	typename Clock::time_point tic_;

public:
	Timer() : tic_(Clock::now()) {}

	void tic() {
		tic_ = Clock::now();
	}
	auto toc() const {
		return std::chrono::duration_cast<Duration>(Clock::now() - tic_).count();
	}
	static auto now() {
		return std::chrono::duration_cast<Duration>(Clock::now().time_since_epoch()).count();
	}

	using DurationType = Duration;
	using ClockType = Clock;
};

} // namespace clst

#endif // CLST_TIMER_HPP
