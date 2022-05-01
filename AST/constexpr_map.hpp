#pragma once

#include <algorithm>
#include <array>
#include <exception>
#include <initializer_list>

template <typename Key, typename Value, std::size_t Size>
struct ConstexprMap {
    using itemT = std::pair<Key, Value>;
    std::array<itemT, Size> data;

    constexpr ConstexprMap() = default;
    constexpr ConstexprMap(std::initializer_list<itemT> arg_list) {
        int i = 0;
        for (auto it = arg_list.begin(); it != arg_list.end(); ++it) {
            data[i++] = *it;
        }
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