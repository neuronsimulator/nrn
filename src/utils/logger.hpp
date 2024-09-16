#pragma once

#include <functional>
#include <iostream>

#include <fmt/printf.h>

class Logger {
  public:
    void setCallback(std::function<int(int, char*)> cb) {
        callback = std::move(cb);
    }

    const std::function<int(int, char*)>& getCallback() const {
        return callback;
    }

    template <typename... Args>
    void info(const char* fmt, Args... args) {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const char* fmt, Args... args) {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void output(int out, const char* fmt, Args... args) {
        std::string message = fmt::format(fmt, std::forward<Args>(args)...);
        callback(out, message.data());
    }


  private:
    std::function<int(int, char*)> callback = [](int out, char* mess) {
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
int Fprintf(FILE* stream, const char* fmt, Args... args) {
    if (logger.getCallback() && (stream == stdout || stream == stderr)) {
        std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
        logger.getCallback()(stream == stdout ? 1 : 2, message.data());
        return message.size();
    }
    return fmt::fprintf(stream, fmt, args...);
}

template <typename... Args>
int Printf(const char* fmt, Args... args) {
    if (logger.getCallback()) {
        std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
        logger.getCallback()(1, message.data());
        return message.size();
    }
    return fmt::printf(fmt, args...);
}
