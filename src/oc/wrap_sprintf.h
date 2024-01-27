#include <cstdio>
#include <utility>  // std::forward

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
}  // namespace neuron
