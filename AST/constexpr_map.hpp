#pragma once

#include <algorithm>
#include <array>
#include <exception>
#include <initializer_list>

template <typename Key, std::size_t Size>
struct SerializeMap {
    using itemT = std::pair<Key, std::string_view>;
    std::array<itemT, Size> data;

    constexpr SerializeMap() = default;
    constexpr SerializeMap(std::initializer_list<itemT> arg_list) {
        int i = 0;
        for (auto it = arg_list.begin(); it != arg_list.end(); ++it) {
            data[i++] = *it;
        }
    }

    [[nodiscard]] constexpr std::string_view operator[](const Key &key) const {
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