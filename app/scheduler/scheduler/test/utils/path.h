#ifndef UTILS_PATH_H
#define UTILS_PATH_H

#include <filesystem>

namespace TestUtils {
class Path {
public:
    inline static const std::filesystem::path &getExecutablePath() {
        return s_executablePath;
    }

    inline static std::filesystem::path getExecutableDirectory() {
        return s_executablePath.parent_path();
    }

    static void parseExecutablePath(int argc, char **argv);

private:
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): This access is fine
    static std::filesystem::path s_executablePath;
};
} // namespace TestUtils

#endif // UTILS_PATH_H