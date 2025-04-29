#include <gtest/gtest.h>

#include <fms/math/interval.hpp>
#include <fms/problem/boundary.hpp>

using namespace fms;
using namespace fms::problem;

// NOLINTBEGIN(*-magic-numbers)

TEST(Boundary, Idempotent) {
    Boundary a(0, 0, 0, 0);

    math::Interval<> in(0, 1);
    auto out1 = a.translateToDestination(in);
    auto out2 = a.translateToSource(in);

    EXPECT_EQ(out1, in);
    EXPECT_EQ(out2, in);
}

TEST(Boundary, Translation) {
    Boundary a(10, 30, 10, 30);

    math::Interval<> in(100, 200);
    auto out1 = a.translateToDestination(in);
    auto out2 = a.translateToSource(in);

    EXPECT_EQ(out1, math::Interval<>(120, 220));
    EXPECT_EQ(out2, math::Interval<>(80, 180));
}

TEST(Boundary, NoLowerBound) {
    Boundary a(10, 30, std::nullopt, 30);

    math::Interval<> in(100, 200);
    auto out1 = a.translateToDestination(in);
    auto out2 = a.translateToSource(in);

    EXPECT_EQ(out1, math::Interval<>(std::nullopt, 220));
    EXPECT_EQ(out2, math::Interval<>(80, std::nullopt));
}

// NOLINTEND(*-magic-numbers)