#pragma once

#include <functional>
#include <iostream>

#include <fmt/format.h>

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
    void print(std::string_view fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::string_view fmt, Args&&... args) const {
        output(1, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) const {
        output(2, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void output(int out, std::string_view fmt, Args&&... args) const {
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
