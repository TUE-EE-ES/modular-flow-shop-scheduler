#include "fmt/compile.h"
#include <gtest/gtest.h>

#include <fms/pch/utils.hpp>

using namespace fms;

TEST(Logger, runtime) {
    LOG("Hello, world!");
    LOG("Hello, {}!", "world");
    LOG_C("Hello, world");
    LOG_E("Hello, world");
    LOG_W("Hello, world");
    LOG_I("Hello, world");
    LOG_D("Hello, world");
    LOG_T("Hello, world");
    LOG_C("Hello, {}!", "world");
    LOG_E("Hello, {}!", "world");
    LOG_W("Hello, {}!", "world");
    LOG_I("Hello, {}!", "world");
    LOG_D("Hello, {}!", "world");
    LOG_T("Hello, {}!", "world");
}

TEST(Logger, compiled) {
    LOG(FMT_COMPILE("Hello, {}!"), "world");
    LOG_C(FMT_COMPILE("Hello, {}!"), "world");
    LOG_E(FMT_COMPILE("Hello, {}!"), "world");
    LOG_W(FMT_COMPILE("Hello, {}!"), "world");
    LOG_I(FMT_COMPILE("Hello, {}!"), "world");
    LOG_D(FMT_COMPILE("Hello, {}!"), "world");
    LOG_T(FMT_COMPILE("Hello, {}!"), "world");
}
