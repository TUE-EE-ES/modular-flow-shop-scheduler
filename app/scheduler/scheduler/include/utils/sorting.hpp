#ifndef UTILS_SORTING_HPP
#define UTILS_SORTING_HPP

#include <cstddef>
#include <utility>
#include <vector>

namespace algorithm {
// From: https://stackoverflow.com/a/17074810/4005637
template <typename RandomIt, typename RandomItPermutation>
void applyPermutationInPlace(RandomIt vecFirst, RandomIt vecLast, RandomItPermutation permFirst) {
    std::vector<bool> done(vecLast - vecFirst);

    for (std::size_t i = 0; vecFirst + i < vecLast; ++i) {
        if (done[i]) {
            continue;
        }

        done[i] = true;
        std::size_t prevJ = i;
        std::size_t j = permFirst[i];

        while (i != j) {
            std::swap(vecFirst[prevJ], vecFirst[j]);
            done[j] = true;
            prevJ = j;
            j = permFirst[j];
        }
    }
}

}; // namespace algorithm

#endif // UTILS_SORTING_HPP