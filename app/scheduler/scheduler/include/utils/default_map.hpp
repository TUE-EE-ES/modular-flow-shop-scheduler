#ifndef FORPFSSPSD_OPERATIONS_TIME_TABLE
#define FORPFSSPSD_OPERATIONS_TIME_TABLE

#include <algorithm>
#include <tuple>
#include <unordered_map>

namespace FMS {

template <typename Key, typename Value> class DefaultMap {
public:
    using Table = std::unordered_map<Key, Value>;

    DefaultMap(const DefaultMap &) = default;

    DefaultMap(DefaultMap &&) noexcept = default;

    constexpr explicit DefaultMap(const Value &defaultValue) : m_defaultValue(defaultValue) {}

    // NOLINTNEXTLINE: Allow implicit conversion to tuple for braced initialization
    constexpr DefaultMap(std::tuple<Table, Value> &&tableAndDefaultValue)
        : m_table(std::move(std::get<0>(tableAndDefaultValue))),
          m_defaultValue(std::move(std::get<1>(tableAndDefaultValue))) {}

    constexpr DefaultMap(Table table, Value defaultValue) :
        m_table(std::move(table)), m_defaultValue(defaultValue) {}

    constexpr DefaultMap &operator=(const DefaultMap &) = default;

    constexpr DefaultMap &operator=(DefaultMap &&) noexcept = default;

    constexpr ~DefaultMap() = default;

    Value operator()(const Key &first) const {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            return it->second;
        }

        return m_defaultValue;
    }

    std::optional<Value> getMaybe(const Key &first) const {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    [[nodiscard]] inline typename Table::const_iterator begin() const noexcept {
        return m_table.cbegin();
    }

    [[nodiscard]] inline typename Table::const_iterator end() const noexcept {
        return m_table.cend();
    }

    [[nodiscard]] inline typename Table::iterator begin() noexcept { return m_table.begin(); }

    [[nodiscard]] inline typename Table::iterator end() noexcept { return m_table.end(); }

    [[nodiscard]] inline Value getDefaultValue() const noexcept { return m_defaultValue; }

    void insert(const Key &first, const Value &second) { m_table.emplace(first, second); }

private:
    Table m_table;
    Value m_defaultValue;
};

template <typename Key, typename Value> class TwoKeyMap {
public:
    using Table = std::unordered_map<Key, std::unordered_map<Key, Value>>;
    using const_iterator = typename Table::const_iterator;
    using iterator = typename Table::iterator;

    TwoKeyMap() = default;

    TwoKeyMap(const TwoKeyMap &) = default;

    TwoKeyMap(TwoKeyMap &&) noexcept = default;

    // NOLINTNEXTLINE: Allow implicit conversion to tuple for braced initialization
    TwoKeyMap(Table table) : m_table(std::move(table)) {}

    TwoKeyMap &operator=(const TwoKeyMap &) = default;

    TwoKeyMap &operator=(TwoKeyMap &&) noexcept = default;

    ~TwoKeyMap() = default;

    [[nodiscard]] Value operator()(const Key &first, const Key &second) const {
        return m_table.at(first).at(second);
    }

    [[nodiscard]] std::optional<Value> getMaybe(const Key &first, const Key &second) const {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            const auto it2 = it->second.find(second);
            if (it2 != it->second.end()) {
                return it2->second;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] inline typename Table::const_iterator find(const Key &key) const {
        return m_table.find(key);
    }

    [[nodiscard]] bool contains(const Key &first, const Key &second) const {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            return it->second.find(second) != it->second.end();
        }

        return false;
    }

    [[nodiscard]] inline typename Table::iterator begin() noexcept { return m_table.begin(); }

    [[nodiscard]] inline typename Table::iterator end() noexcept { return m_table.end(); }

    [[nodiscard]] inline typename Table::const_iterator begin() const noexcept {
        return m_table.cbegin();
    }

    [[nodiscard]] inline typename Table::const_iterator end() const noexcept {
        return m_table.cend();
    }

    [[nodiscard]] inline typename Table::const_iterator cbegin() const noexcept {
        return m_table.cbegin();
    }

    [[nodiscard]] inline typename Table::const_iterator cend() const noexcept {
        return m_table.cend();
    }

    [[nodiscard]] inline std::size_t size() const noexcept { return m_table.size(); }

    [[nodiscard]] inline constexpr auto &table() const noexcept { return m_table; }

    /**
     * @brief Inserts the maximum among the current value and the given value.
     *
     * @param first First key
     * @param second Second key
     * @param value Value to be inserted if it is greater than the current value
     */
    void insertMax(const Key &first, const Key &second, const Value &value) {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            const auto it2 = it->second.find(second);
            if (it2 != it->second.end()) {
                if (it2->second < value) {
                    it2->second = value;
                }
            } else {
                it->second.insert({second, value});
            }
        } else {
            m_table.emplace(first, std::unordered_map<Key, Value>{{second, value}});
        }
    }

    /**
     * @brief Inserts the minimum among the current value and the given value.
     *
     * @param first First key
     * @param second Second key
     * @param value Value to be inserted if it is less than the current value
     */
    void insertMin(const Key &first, const Key &second, const Value &value) {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            const auto it2 = it->second.find(second);
            if (it2 != it->second.end()) {
                if (it2->second > value) {
                    it2->second = value;
                }
            } else {
                it->second.insert({second, value});
            }
        } else {
            m_table.emplace(first, std::unordered_map<Key, Value>{{second, value}});
        }
    }

    void insert(const Key &first, const Key &second, const Value &value) {
        auto &map = m_table[first];
        map.emplace(second, value);
    }

    void insert(const Key &first, const Key &second, Value &&value) {
        auto &map = m_table[first];
        map.emplace(second, std::move(value));
    }

private:
    Table m_table;
};

template <typename Key, typename Value> class DefaultTwoKeyMap {
public:
    using Table = TwoKeyMap<Key, Value>;

    DefaultTwoKeyMap(const DefaultTwoKeyMap &) = default;

    DefaultTwoKeyMap(DefaultTwoKeyMap &&) noexcept = default;

    explicit DefaultTwoKeyMap(Value defaultValue) : m_defaultValue(defaultValue) {}

    DefaultTwoKeyMap(Table table, Value defaultValue) :
        m_table(std::move(table)), m_defaultValue(defaultValue) {}

    DefaultTwoKeyMap &operator=(const DefaultTwoKeyMap &) = default;

    DefaultTwoKeyMap &operator=(DefaultTwoKeyMap &&) noexcept = default;

    ~DefaultTwoKeyMap() = default;

    [[nodiscard]] Value operator()(const Key &first, const Key &second) const {
        const auto it = m_table.find(first);
        if (it != m_table.end()) {
            const auto it2 = it->second.find(second);
            if (it2 != it->second.end()) {
                return it2->second;
            }
        }

        return m_defaultValue;
    }

    [[nodiscard]] inline typename Table::const_iterator find(const Key &first) const {
        return m_table.find(first);
    }

    [[nodiscard]] inline bool contains(const Key &first, const Key &second) const {
        return m_table.contains(first, second);
    }

    [[nodiscard]] inline typename Table::const_iterator begin() const noexcept {
        return m_table.cbegin();
    }

    [[nodiscard]] inline typename Table::const_iterator end() const noexcept {
        return m_table.cend();
    }

    [[nodiscard]] inline typename Table::iterator begin() noexcept { return m_table.begin(); }

    [[nodiscard]] inline typename Table::iterator end() noexcept { return m_table.end(); }

    [[nodiscard]] inline Table &table() noexcept { return m_table; }

    [[nodiscard]] inline constexpr const Table &table() const noexcept { return m_table; }

    [[nodiscard]] inline std::size_t size() const noexcept { return m_table.size(); }

    [[nodiscard]] inline bool isEmpty() const noexcept { return m_table.empty(); }

    [[nodiscard]] Value getDefaultValue() const noexcept { return m_defaultValue; }

    void insert(const Key &first, const Key &second, const Value &value) {
        m_table.insert(first, second, value);
    }

    void insert(const Key &first, const Key &second, Value &&value) {
        m_table.insert(first, second, std::move(value));
    }

private:
    Table m_table;
    Value m_defaultValue;
};

} // namespace FMS

#endif // FORPFSSPSD_OPERATIONS_TIME_TABLE