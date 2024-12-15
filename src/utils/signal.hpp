#pragma once

#include <unordered_map>

template <typename... Args>
class signal_ {
  public:
    using key_type = unsigned;

    template <typename F>
    key_type connect(F f) {
        ++counter;
        functors[counter] = f;
        return counter;
    }

    void disconnect(unsigned i) {
        auto it = functors.find(i);
        if (it != functors.end()) {
            functors.erase(it);
        }
    }

    void send(Args... args) const {
        for (const auto& [i, f]: functors) {
            std::invoke(f, args...);
        }
    }

  private:
    key_type counter = 0;
    std::unordered_map<key_type, std::function<void(Args...)>> functors;
};
