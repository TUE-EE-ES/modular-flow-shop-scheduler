//
// Created by jmarc on 11/6/2021.
//

#ifndef MATH_INTERVAL_HPP
#define MATH_INTERVAL_HPP

#include "pch/containers.hpp"

#include <fmt/compile.h>
#include <stdexcept>

namespace Math {

/**
 * @brief Arithmetic intervals class @f$[a, b]@f$ that supports infinite endpoints
 * @f$(-\infty, b]@f$, @f$[a, +\infty)@f$, or @f$(-\infty, +\infty)@f$.
 *
 * @tparam T Underlying arithmetic type to use for the interval.
 */
template <typename T = int64_t> class Interval {
public:
    using MaybeT = std::optional<T>;

    Interval(MaybeT valueMin, MaybeT valueMax) : m_min(valueMin), m_max(valueMax) { checkMinMax(); }
    ~Interval() = default;

    Interval(Interval &&other) noexcept = default;
    Interval(const Interval &other) noexcept = default;
    Interval &operator=(const Interval &other) = default;
    Interval &operator=(Interval &&other) noexcept = default;

    inline static const Interval<T>& Empty() {
        static const Interval<T> empty(std::nullopt, std::nullopt);
        return empty;
    }

    bool operator==(const Interval &other) const {
        return m_min == other.m_min && m_max == other.m_max;
    }

    /**
     * @brief Returns the minimum optional value.
     *
     * @return const MaybeT& Minimum optional value.
     */
    inline const MaybeT &min() const { return m_min; }

    /**
     * @brief Returns the maximum optional value.
     *
     * @return const MaybeT& Maximum optional value.
     */
    inline const MaybeT &max() const { return m_max; }

    /**
     * @brief Returns the minimum value and throws an exception if there is no minimum value.
     *
     * @return const T& Minimum value.
     */
    inline const T &minValue() const { return m_min.value(); }

    /**
     * @brief Returns the maximum value and throws an exception if there is no maximum value.
     *
     * @return const T& Maximum value.
     */
    inline const T &maxValue() const { return m_max.value(); }

    Interval &operator+=(const Interval &other) {
        if (m_min && other.m_min) {
            m_min.value() += other.m_min.value();
        } else {
            // x + (-∞) = (-∞)
            m_min.reset();
        }

        if (m_max && other.m_max) {
            m_max.value() += other.m_max.value();
        } else {
            // x + (+∞) = (+∞)
            m_max.reset();
        }

        return *this;
    }

    [[nodiscard]] Interval operator+(const Interval &other) const {
        return Interval(*this) += other;
    }

    [[nodiscard]] inline Interval operator+(Interval &&other) const { return other += *this; }

    Interval &operator+=(T value) {
        if (m_min) {
            m_min.value() += value;
        }

        if (m_max) {
            m_max.value() += value;
        }

        return *this;
    }

    [[nodiscard]] Interval operator+(const T &value) const { return Interval(*this) += value; }

    Interval &operator+=(const std::tuple<MaybeT, MaybeT> &other) {
        const auto &[o_min, o_max] = other;

        if (m_min && o_min) {
            m_min.value() += o_min.value();
        } else {
            // x + (-∞) = (-∞)
            m_min.reset();
        }

        if (m_max && o_max) {
            m_max.value() += o_max.value();
        } else {
            // x + (+∞) = (+∞)
            m_max.reset();
        }

        checkMinMax();
        return *this;
    }

    [[nodiscard]] Interval operator+(const std::tuple<MaybeT, MaybeT> &other) const {
        return Interval(*this) += other;
    }

    Interval &operator-=(T value) {
        if (m_min) {
            m_min.value() -= value;
        }

        if (m_max) {
            m_max.value() -= value;
        }

        return *this;
    }

    [[nodiscard]] Interval operator-(const T &value) const { return Interval(*this) -= value; }

    Interval &replace(MaybeT min, MaybeT max) {
        m_min = min ? min : m_min;
        m_max = max ? max : m_max;
        checkMinMax();
        return *this;
    }

    /**
     * @brief Returns an interval that is a wider version of the current one and @p other . That is,
     * it takes the maximum value among the upper bound and the minimum value among the lower bound.
     *
     * @param other Other interval to compare to
     * @return Interval Externded interval
     */
    [[nodiscard]] Interval extend(const Interval &other) const {
        MaybeT newMin;
        MaybeT newMax;

        if (m_min && other.m_min) {
            newMin = std::min(m_min.value(), other.m_min.value());
        }

        if (m_max && other.m_max) {
            newMax = std::max(m_max.value(), other.m_max.value());
        }

        return Interval(newMin, newMax);
    }

    /**
     * @brief Returns an interval that is a shorter version of combining the current one and
     * @p other. That is, it takes the minimum value among the upper bound and the maximum value
     * among the lower bound.
     *
     * @param other
     * @return Interval Shortened interval
     */
    [[nodiscard]] Interval shorten(const Interval &other) const {
        MaybeT newMin;
        MaybeT newMax;

        if (m_min && other.m_min) {
            newMin = std::max(m_min.value(), other.m_min.value());
        } else {
            newMin = m_min ? m_min : other.m_min;
        }

        if (m_max && other.m_max) {
            newMax = std::min(m_max.value(), other.m_max.value());
        } else {
            newMax = m_max ? m_max : other.m_max;
        }

        return Interval(newMin, newMax);
    }

    // Allen's interval algebra

    bool operator<(const Interval &other) const {
        // Either max or other.min has no value
        if (!(m_max && other.m_min)) {
            return false;
        }

        return m_max < other.m_min;
    }

    bool operator>(const Interval &other) const {
        if (!(m_min && other.m_max)) {
            return false;
        }

        return m_min > other.m_max;
    }

    bool meets(const Interval &other) const {
        return m_max && other.m_min && (m_max == other.m_min);
    }

    bool overlaps(const Interval &other) const {
        bool overlapsMaxMin = false;
        if (!(m_max && other.m_min)) {
            overlapsMaxMin = true;
        } else {
            overlapsMaxMin = (m_max > other.m_min);
        }

        bool passesMinMax = true;
        if (!(m_min && other.m_max)) {
            passesMinMax = false;
        } else {
            passesMinMax = (m_min > other.m_max);
        }

        return overlapsMaxMin && !passesMinMax;
    }

    [[nodiscard]] std::string toString() const {
        return fmt::format(FMT_COMPILE("[{}, {}]"),
                           m_min ? fmt::to_string(*m_min) : "-∞",
                           m_max ? fmt::to_string(*m_max) : "+∞");
    }

    [[nodiscard]] bool converged(const Interval &other) const {
        if (m_min && other.m_min && m_min != other.m_min) {
            return false;
        }

        if (m_max && other.m_max && m_max != other.m_max) {
            return false;
        }

        return true;
    }

private:
    MaybeT m_min, m_max;

    void checkMinMax() {
        if (m_min && m_max && m_max < m_min) {
            throw std::domain_error("Value of min must be smaller than value of max");
        }
    }
};
} // namespace Math

#endif // MATH_INTERVAL_HPP
