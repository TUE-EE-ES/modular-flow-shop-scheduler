#include "pch/containers.hpp"

#include "utils/time.h"

#include <chrono>

static constexpr uint64_t kNanosecondsPerMillisecond = 1000000;
static constexpr uint64_t kMillisecondsPerSecond = 1000;

#if defined(_WIN32) || defined(_WIN64)

#include <Windows.h>

static constexpr uint64_t k100NanosecondsPerMillisecond = 10000;

std::chrono::milliseconds FMS::getCpuTime() {
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime)
        == 0) {
        return std::chrono::milliseconds(0);
    }
    ULARGE_INTEGER liUserTime;
    liUserTime.LowPart = userTime.dwLowDateTime;
    liUserTime.HighPart = userTime.dwHighDateTime;
    ULARGE_INTEGER liKernelTime;
    liKernelTime.LowPart = kernelTime.dwLowDateTime;
    liKernelTime.HighPart = kernelTime.dwHighDateTime;
    uint64_t totalTime = liUserTime.QuadPart + liKernelTime.QuadPart;
    return std::chrono::milliseconds(totalTime / k100NanosecondsPerMillisecond);
}
#else
#include <ctime>
std::chrono::milliseconds FMS::getCpuTime() {
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
        return std::chrono::milliseconds(0);
    }

    return std::chrono::milliseconds(ts.tv_sec * kMillisecondsPerSecond
                                     + ts.tv_nsec / kNanosecondsPerMillisecond);
}
#endif