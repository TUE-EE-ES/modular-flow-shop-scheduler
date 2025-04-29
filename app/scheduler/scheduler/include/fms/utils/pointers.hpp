#ifndef UTILS_POINTERS_HPP
#define UTILS_POINTERS_HPP

#include <memory>

namespace fms::utils::memory {
// NOLINTBEGIN(*-param-not-moved): We require an rvalue because we invalidate the previous pointer
// From https://stackoverflow.com/a/21174979/4005637
template <typename Derived, typename Base>
std::unique_ptr<Derived> static_unique_ptr_cast(std::unique_ptr<Base> &&p) {
    return std::unique_ptr<Derived>(static_cast<Derived *>(p.release()));
}

template <typename Derived, typename Base>
std::unique_ptr<Derived> dynamic_unique_ptr_cast(std::unique_ptr<Base> &p) {
    if (auto *result = dynamic_cast<Derived *>(p.get())) {
        p.release();
        return std::unique_ptr<Derived>(result);
    }
    return std::unique_ptr<Derived>(nullptr);
}
// NOLINTEND(*-param-not-moved)

} // namespace fms::utils::memory

#endif // UTILS_POINTERS_HPP