#pragma

#include <map>

template <typename... Args>
class signal_ {
  public:
    template <typename F>
    unsigned connect(F f) {
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

    void send(Args... args) {
        for (const auto& [i, f]: functors) {
            std::invoke(f, args...);
        }
    }

  private:
    unsigned counter = 0;
    std::map<int, std::function<void(Args...)>> functors;
};
