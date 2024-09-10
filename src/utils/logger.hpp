#pragma once

#include <fmt/printf.h>

using LoggerCallback = int(int, const char*);
using PassCallback = int(void);

extern LoggerCallback* nrnpy_pr_stdoe_callback;
extern PassCallback* nrnpy_pass_callback;

template <typename ...T>
int Fprintf(FILE* stream, const char* fmt, T ...args) {
    if (nrnpy_pr_stdoe_callback && (stream == stdout || stream == stderr)) {
        std::string message = fmt::sprintf(fmt, args...);
        nrnpy_pr_stdoe_callback(stream == stdout ? 1 : 2, message.c_str());
        return message.size();
    }
    return fmt::fprintf(stream, fmt, args...);
}

template <typename ...T>
int Printf(const char* fmt, T ...args) {
    if (nrnpy_pr_stdoe_callback) {
        std::string message = fmt::sprintf(fmt, args...);
        nrnpy_pr_stdoe_callback(1, message.c_str());
        return message.size();
    }
    return fmt::printf(fmt, args...);
}
