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
    throw ExcepT(fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T>
inline void dbg_print(fmt::format_string<T...> fmt, T &&...args) {
#ifndef NDEBUG
    fmt::print(stderr, fmt, std::forward<T>(args)...);
#endif
}
