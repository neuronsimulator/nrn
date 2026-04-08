#include <cstdio>
#include <utility>  // std::forward
#include <stdexcept>
#include <assert.h>

namespace neuron {
/**
 * @brief Type-safe sprintf replacement using snprintf with automatic buffer size deduction
 *
 * Wraps std::snprintf and deduces the buffer size when the destination is a fixed-size
 * array (e.g. `char buf[512]`). Prevents buffer overflows and avoids deprecation warnings
 * from using plain `sprintf`.
 *
 * When no format arguments are provided, uses a diagnostic pragma to suppress the common
 * -Wformat-security warning that would otherwise trigger on non-literal format strings
 * (even though string literals are overwhelmingly the typical use case here).
 *
 * @note Does **not** work with pointers (`char*`) or runtime-sized arrays (`char buf[]`).
 *       In those cases, call `snprintf` directly with an explicit size.
 *
 * @tparam N     Automatically deduced buffer size
 * @param  buf   Fixed-size character array to write into
 * @param  fmt   Format string (typically a string literal)
 * @param  args  Optional format arguments
 * @return       Number of characters written (excluding null terminator), or negative on error
 */
template <std::size_t N, typename... Args>
int Sprintf(char (&buf)[N], const char* fmt, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        // work around a false positive from -Wformat-security when there are no arguments
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
        auto i = std::snprintf(buf, N, fmt);
#pragma GCC diagnostic pop
        return i;
    } else {
        return std::snprintf(buf, N, fmt, std::forward<Args>(args)...);
    }
}

/**
 * @brief assert if the Sprintf format data does not fit into buf
 */
template <std::size_t N, typename... Args>
void SprintfAsrt(char (&buf)[N], const char* fmt, Args&&... args) {
    int sz = Sprintf(buf, fmt, std::forward<Args>(args)...);
#ifdef UNIT_TESTING
    if (sz < 0 || std::size_t(sz) >= N) {
        throw std::runtime_error("SprintfAsrt buffer too small or snprintf error");
    }
#else
    assert(sz >= 0 && std::size_t(sz) < N);
#endif
}

}  // namespace neuron
