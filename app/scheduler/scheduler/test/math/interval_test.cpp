#include <gtest/gtest.h>

#include <fms/pch/containers.hpp>

#include <fms/math/interval.hpp>

#include <cinttypes>
#include <sstream>
#include <stdexcept>

using namespace fms::math;
using Int = Interval<int64_t>;

TEST(MathInterval, Constructor) {
    EXPECT_NO_THROW(Int(std::nullopt, std::nullopt));
    EXPECT_NO_THROW(Int(0, std::nullopt));
    EXPECT_NO_THROW(Int(std::nullopt, 0));
    EXPECT_NO_THROW(Int(0, 10));
    EXPECT_NO_THROW(Int(0, 0));
    EXPECT_THROW(Int(10, 0), std::domain_error);
}

TEST(MathInterval, Addition) {
    // Input values
    Int i1(0, 10);
    Int i2(5, 15);
    Int i3(std::nullopt, 1);
    Int i4(1, std::nullopt);

    // Output values
    std::array output{i1 + i2,
                      i2 + i1,
                      i1 + i3,
                      i3 + i1,
                      i1 + i4,
                      i4 + i1,
                      i3 + i4,
                      i4 + i3,
                      i1 + Int(5, 15)};

    // Expected values
    std::array expected{
            Int(5, 25),
            Int(std::nullopt, 11),
            Int(1, std::nullopt),
            Int(std::nullopt, std::nullopt),
    };
    std::array expectedIndices{0, 0, 1, 1, 2, 2, 3, 3, 0};

    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_EQ(output.at(i), expected.at(expectedIndices.at(i)));
    }
}

TEST(MathInterval, Extend) {
    // Input values
    Int i1(0, 10);
    Int i2(5, 15);
    Int i3(std::nullopt, 1);
    Int i4(1, std::nullopt);

    // Output values
    std::array output{i1.extend(i2),
                      i1.extend(i3),
                      i3.extend(i1),
                      i1.extend(i4),
                      i4.extend(i1),
                      i3.extend(i4),
                      i4.extend(i3)};

    // Expected values
    std::array expected{Int(0, 15),
                        Int(std::nullopt, 10),
                        Int(0, std::nullopt),
                        Int(std::nullopt, std::nullopt)};
    std::array expectedIndices{0, 1, 1, 2, 2, 3, 3};

    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_EQ(output.at(i), expected.at(expectedIndices.at(i)));
    }
}

TEST(MathInterval, Shorten) {
    // Input values
    Int i1(0, 10);
    Int i2(5, 15);
    Int i3(std::nullopt, 1);
    Int i4(1, std::nullopt);

    // Output values
    std::array output{i1.shorten(i2),
                      i2.shorten(i1),
                      i1.shorten(i3),
                      i3.shorten(i1),
                      i1.shorten(i4),
                      i4.shorten(i1),
                      i3.shorten(i4),
                      i4.shorten(i3)};

    // Expected values
    std::array expected{Int(5, 10), Int(0, 1), Int(1, 10), Int(1, 1)};
    std::array expectedIndices{0, 0, 1, 1, 2, 2, 3, 3};

    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_EQ(output.at(i), expected.at(expectedIndices.at(i)));
    }
}

TEST(MathInterval, String) {
    // Input values
    std::array input{Int(0, 10), Int(5, 15), Int(std::nullopt, 1), Int(1, std::nullopt)};
    std::array expectSS{"[0, 10]", "[5, 15]", "[-∞, 1]", "[1, +∞]"};

    for (size_t i = 0; i < input.size(); ++i) {
        const auto ss = input.at(i).toString();
        EXPECT_EQ(ss, expectSS.at(i));
    }
}
