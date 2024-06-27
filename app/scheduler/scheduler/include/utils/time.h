#ifndef UTILS_TIME_H
#define UTILS_TIME_H

#include <algorithm>
#include <chrono>

namespace FMS {

/**
 * @brief Get the amount of CPU time in milliseconds
 *
 * @return std::chrono::duration Milliseconds of CPU time
 */
std::chrono::milliseconds getCpuTime();

/**
 * @brief Timer that counts remaining time. Starting at construction.
 * @details The @ref StaticTimer class is used to count the remaining time from the moment of its
 * construction. It takes a @ref std::chrono::milliseconds parameter that specifies the maximum time
 * allowed. The @ref isTimeUp() method returns `true` if the elapsed time since construction is
 * greater than the maximum time allowed.
 */
class StaticTimer {
public:
    using Milliseconds = std::chrono::milliseconds;
    StaticTimer() : m_timeMax(0), m_timeStart(0) {}
    explicit StaticTimer(Milliseconds maxTime) : m_timeMax(maxTime), m_timeStart(getCpuTime()) {}

    [[nodiscard]] inline bool isTimeUp() const { return getCpuTime() - m_timeStart > m_timeMax; }

    [[nodiscard]] inline bool isRunning() const { return !isTimeUp(); }

    [[nodiscard]] Milliseconds remainingTime() const {
        return std::max(m_timeMax - (getCpuTime() - m_timeStart), Milliseconds(0));
    }

private:
    Milliseconds m_timeMax;
    Milliseconds m_timeStart;
};

} // namespace FMS

#endif