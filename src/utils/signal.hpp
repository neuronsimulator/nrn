#pragma

#include <list>

template <typename T>
class signal_ {
  public:
    template <typename F>
    void connect(F f) {
        functors.push_back(f);
    }

    void send(T wc) {
        for (auto& f: functors) {
            std::invoke(f, wc);
        }
    }

  private:
    std::list<std::function<void(T)>> functors;
};
