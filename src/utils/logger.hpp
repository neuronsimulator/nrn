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

    // Unconditionnaly print non depending of the level
    template <typename... Args>
    void print(const char* fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(const char* fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(const char* fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(const char* fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const char* fmt, Args&&... args) const {
        output(2, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void output(int out, const char* fmt, Args&&... args) const {
        std::string message = fmt::format(fmt, std::forward<Args>(args)...);
        callback(out, message.c_str());
    }

  private:
    std::function<int(int, const char*)> callback = [](int out, const char* mess) {
        if (out == 1) {
            std::cout << mess;
        } else {
            std::cerr << mess;
        }
        return 0;
    };
};

extern Logger logger;
