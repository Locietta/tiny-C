#pragma once

#include <algorithm>
#include <array>
#include <initializer_list>
#include <stdexcept>

template <typename Key, typename Value, std::size_t Size>
struct ConstexprMap {
    using itemT = std::pair<Key, Value>;
    std::array<itemT, Size> data;
    std::size_t sz;

    constexpr ConstexprMap() = default;
    constexpr ConstexprMap(std::initializer_list<itemT> const &arg_list) {
        int i = 0;
        for (auto it = arg_list.begin(); it != arg_list.end(); ++it) {
            data[i++] = *it;
        }
        sz = i;
    }

    [[nodiscard]] constexpr Value operator[](const Key &key) const {
        const auto it = std::find_if(std::begin(data), std::end(data), [&key](const auto &v) {
            return v.first == key;
        });
        if (it != std::end(data)) {
            return it->second;
        } else {
            throw std::range_error("Not Found");
        }
    }
};