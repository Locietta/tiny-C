#pragma once

// To silence the control reaches end problems
//
[[noreturn]] inline void unreachable() {
    // Uses compiler specific extensions if possible.
    // Even if no extension is used, undefined behavior is still raised by
    // an empty function body and the noreturn attribute.
#ifdef __GNUC__ // GCC, Clang, ICC
    __builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
    __assume(false);
#endif
}

#include <fmt/core.h>

template <typename ExcepT = std::logic_error, typename... T>
[[noreturn]] inline void throw_err(fmt::format_string<T...> fmt, T &&...args) {
    std::string err_msg;
    fmt::format_to(std::back_inserter(err_msg), fmt, std::forward<T>(args)...);
    throw ExcepT(err_msg);
}

template <typename... T>
inline std::string format(fmt::format_string<T...> fmt, T &&...args) {
    std::string ret;
    fmt::format_to(std::back_inserter(ret), fmt, std::forward<T>(args)...);
    return std::move(ret);
}
