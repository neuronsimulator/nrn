#pragma once

#include <fmt/format.h>

#include "oc_ansi.h"

template <typename... T>
[[noreturn]] void hoc_execerror_fmt(const std::string& fmt, T&&... args) {
    auto s = fmt::format(fmt, std::forward<T>(args)...);
    hoc_execerror(s.c_str(), nullptr);
}
