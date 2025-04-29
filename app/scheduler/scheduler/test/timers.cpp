#include <gtest/gtest.h>

#include <fms/utils/time.hpp>

#include <thread>

using namespace fms;
using namespace fms::utils::time;
using namespace std::chrono_literals;

TEST(Timers, oneSecond) {
    StaticTimer timer(500ms);
    EXPECT_FALSE(timer.isTimeUp());
    EXPECT_TRUE(timer.isRunning());

    std::this_thread::sleep_for(50ms);
    auto remaining = timer.remainingTime();
    EXPECT_GE(remaining, 0ms);
}
