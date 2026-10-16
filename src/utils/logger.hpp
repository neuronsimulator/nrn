#pragma once

#include <fmt/printf.h>

extern int (*nrnpy_pr_stdoe_callback)(int, char*);

template <typename... Args>
int Fprintf(FILE* stream, const char* fmt, Args... args) {
    if (nrnpy_pr_stdoe_callback && (stream == stdout || stream == stderr)) {
        std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
        nrnpy_pr_stdoe_callback(stream == stdout ? 1 : 2, message.data());
        return message.size();
    }
    return fmt::fprintf(stream, fmt, args...);
}

template <typename... Args>
int Printf(const char* fmt, Args... args) {
    if (nrnpy_pr_stdoe_callback) {
        std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
        nrnpy_pr_stdoe_callback(1, message.data());
        return message.size();
    }
    return fmt::printf(fmt, args...);
}
