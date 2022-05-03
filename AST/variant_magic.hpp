#pragma once
#include <cassert>
#include <type_traits>
#include <variant>

namespace impl {

template <typename T>
constexpr T quiet_declval() {
    return *static_cast<T *>(nullptr);
}

template <>
constexpr void quiet_declval<void>() {}

template <typename R, typename... Ts, typename U, typename... Fs>
R match_impl(U &&u, Fs... arms) {
    struct overloaded : public Fs... {
        using Fs::operator()...;
    };
    assert(!u.valueless_by_exception() && "empty variant!");
    using CheckArgs = std::conjunction<std::is_invocable<overloaded, Ts>...>;
    static_assert(CheckArgs::value, "Missing matching cases");
    if constexpr (!CheckArgs::value) {
        return quiet_declval<R>();
    } else {
        using CheckRet =
            std::conjunction<std::is_convertible<std::invoke_result_t<overloaded, Ts>, R>...>;
        static_assert(CheckRet::value, "Return type not compatible");
        if constexpr (!CheckRet::value) {
            return quiet_declval<R>();
        } else {
            overloaded dispatcher{arms...};
            return std::visit(
                [&](auto &&var) -> R { return dispatcher(std::forward<decltype(var)>(var)); },
                std::forward<U>(u));
        }
    }
}

} // namespace impl

template <typename R = void, typename... Ts, typename... Fs>
R match(const std::variant<Ts...> &u, Fs... arms) {
    return impl::match_impl<R, const Ts &...>(u, arms...);
}

template <typename R = void, typename... Ts, typename... Fs>
R match(std::variant<Ts...> &&u, Fs... arms) {
    return impl::match_impl<R, Ts &&...>(std::move(u), arms...);
}