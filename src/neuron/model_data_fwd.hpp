#pragma once
#include <optional>
#include <string>
#include <vector>

namespace neuron {
struct Model;
inline Model& model();
namespace container {
template <typename>
struct data_handle;
namespace utils {
template <typename T>
[[nodiscard]] data_handle<T> find_data_handle(T*);

struct storage_info {
    std::string name{};
    std::size_t size{};
};

/** @brief Try and find a helpful name for a container.
 *
 *  In practice this can be expected to be work for the structures that can be
 *  discovered from neuron::model(), and not for anything else. If no
 *  information about the container can be found the returned std::optional will
 *  not contain a value.
 */
[[nodiscard]] inline std::optional<storage_info> find_container_info(void const*);
}  // namespace utils
}  // namespace container
}  // namespace neuron