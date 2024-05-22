#include <memory>
#include <string>

namespace nmodl {
namespace utils {

class Blame {
  public:
    Blame(size_t blame_line)
        : blame_line(blame_line) {}

    template <class OStream>
    void operator()(OStream& os, size_t current_line) const {
        if (blame_line == current_line) {
            os << this->format();
        }
    }

  protected:
    virtual std::string format() const = 0;

  private:
    size_t blame_line = 0;
};

std::unique_ptr<Blame> make_blame(size_t blame_line);

}  // namespace utils
}  // namespace nmodl
