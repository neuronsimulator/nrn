#pragma once

#include <functional>
#include <iostream>

#include <fmt/printf.h>

class Logger {
  public:
    void setCallback(std::function<int(int, const char*)> cb) {
        callback = std::move(cb);
    }

    const std::function<int(int, const char*)>& getCallback() const {
        return callback;
    }

    template <typename... Args>
    void debug(const char* fmt, Args... args) {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(const char* fmt, Args... args) {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(const char* fmt, Args... args) {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const char* fmt, Args... args) {
        output(2, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void output(int out, const char* fmt, Args... args) {
        std::string message = fmt::format(fmt, std::forward<Args>(args)...);
        callback(out, message.c_str());
    }

  private:
    std::function<int(int, const char*)> callback = [](int out, const char* mess) {
        if (out == 1)
            std::cout << mess;
        else
            std::cerr << mess;
        return 0;
    };
};

extern Logger logger;

// Deprecated but there is ~700 to change so let it go a bit more
template <typename... Args>
int Printf(const char* fmt, Args... args) {
    if (logger.getCallback()) {
        std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
        logger.getCallback()(1, message.c_str());
        return message.size();
    }
    return fmt::printf(fmt, args...);
}
