#include <gtest/gtest.h>

#include <fms/problem/indices.hpp>

using namespace fms;
using namespace fms::problem;


TEST(Types, different) {
    JobId j1{1};
    JobId j2{2};
    JobId j3{1};
    JobId jMax{JobId::max()};

    EXPECT_EQ(j1, j3);
    EXPECT_NE(j1, j2);
    EXPECT_NE(j1, jMax);
    EXPECT_NE(j2, jMax);
    EXPECT_NE(j3, jMax);
    EXPECT_EQ(jMax, JobId::max());
    EXPECT_EQ(jMax, jMax);
    EXPECT_EQ(j1, j1);
    EXPECT_LT(j1, j2);
    EXPECT_GT(j2, j1);
}
