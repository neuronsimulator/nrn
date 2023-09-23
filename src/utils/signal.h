#pragma once

#include <functional>
#include <map>

template <typename F>
class signal {
  public:
    template <typename G>
    std::size_t attach(G&& f) {
        slots.emplace(id, std::forward<G>(f));
        return id++;
    }

    void detach(std::size_t i) {
        slots.erase(i);
    }

    template <typename... Args>
    void operator()(Args... args) {
        for (auto& [_, f]: slots) {
            f(args...);
        }
    }

  private:
    std::size_t id{};
    std::map<std::size_t, std::function<F>> slots{};
};
