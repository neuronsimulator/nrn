#include <cstdio>
#include <utility>  // std::forward
#include <stdexcept>
#include <assert.h>

namespace neuron {
/**
 * @brief Redirect sprintf to snprintf if the buffer size can be deduced.
 *
 * This is useful to avoid deprecation warnings for sprintf. In general it works if the buffer is
 * something like char buf[512] in the calling scope, but not if it is char* or char buf[].
 */
template <std::size_t N, typename... Args>
int Sprintf(char (&buf)[N], const char* fmt, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        // try and work around a false positive from -Wformat-security when there are no arguments
        return std::snprintf(buf, N, "%s", fmt);
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
