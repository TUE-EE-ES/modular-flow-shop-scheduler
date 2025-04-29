#include "test_utils/path.hpp"

#include <gtest/gtest.h>

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    TestUtils::Path::parseExecutablePath(argc, argv);

    return RUN_ALL_TESTS();
}
