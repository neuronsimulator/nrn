#pragma once

#include <fmt/format.h>

#include "oc_ansi.h"

template <typename... T>
[[noreturn]] void hoc_execerror_fmt(const char* fmt, T&&... args) {
    auto s = fmt::format(fmt::runtime(fmt), std::forward<T>(args)...);
    hoc_execerror(s.c_str(), nullptr);
}
