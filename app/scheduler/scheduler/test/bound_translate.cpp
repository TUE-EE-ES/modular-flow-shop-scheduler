#include <gtest/gtest.h>

#include <FORPFSSPSD/boundary.hpp>
#include <math/interval.hpp>

using namespace FORPFSSPSD;

// NOLINTBEGIN(*-magic-numbers)

TEST(Boundary, Idempotent) {
    Boundary a(0, 0, 0, 0);

    Math::Interval<> in(0, 1);
    auto out1 = a.translateToDestination(in);
    auto out2 = a.translateToSource(in);

    EXPECT_EQ(out1, in);
    EXPECT_EQ(out2, in);    
}

TEST(Boundary, Translation) {
    Boundary a(10, 30, 10, 30);

    Math::Interval<> in(100, 200);
    auto out1 = a.translateToDestination(in);
    auto out2 = a.translateToSource(in);

    EXPECT_EQ(out1, Math::Interval<>(120, 220));
    EXPECT_EQ(out2, Math::Interval<>(80, 180));
}

TEST(Boundary, NoLowerBound) {
    Boundary a(10, 30, std::nullopt, 30);

    Math::Interval<> in(100, 200);
    auto out1 = a.translateToDestination(in);
    auto out2 = a.translateToSource(in);

    EXPECT_EQ(out1, Math::Interval<>(std::nullopt, 220));
    EXPECT_EQ(out2, Math::Interval<>(80, std::nullopt));
}

// NOLINTEND(*-magic-numbers)