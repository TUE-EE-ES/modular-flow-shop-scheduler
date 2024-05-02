#include "path.h"

namespace fs = std::filesystem;

fs::path TestUtils::Path::s_executablePath;

void TestUtils::Path::parseExecutablePath(int argc, char **argv) {
    if (argc < 1) {
        throw std::runtime_error("No executable path provided");
    }
    s_executablePath = fs::path(argv[0]);
}
