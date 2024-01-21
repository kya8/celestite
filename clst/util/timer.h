#ifndef CLST_UTIL_TIMER_H
#define CLST_UTIL_TIMER_H


#include <chrono>

namespace clst::util {

template<typename Duration = std::chrono::duration<double, std::ratio<1>>, typename Clock = std::chrono::steady_clock>
class Timer {
	typename Clock::time_point m_tic;

public:
	Timer() : m_tic(Clock::now()) {}

	void tic() {
		m_tic = Clock::now();
	}
	auto toc() const {
		return std::chrono::duration_cast<Duration>(Clock::now() - m_tic).count();
	}
	static auto now() {
		return std::chrono::duration_cast<Duration>(Clock::now().time_since_epoch()).count();
	}

	using DurationType = Duration;
	using ClockType = Clock;
};

} // namespace util


#endif /* CLST_UTIL_TIMER_H */
