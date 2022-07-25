#pragma once
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

/** @brief Try and find a helpful name for a container.
 *
 *  In practice this can be expected to be work for the structures that can be
 *  discovered from neuron::model(), and not for anything else. If a
 *  human-readable name cannot be found then a string representation of the
 *  container address will be used instead.
 */
template <typename T>
[[nodiscard]] std::string find_container_name(std::vector<T> const&);
}  // namespace utils
}  // namespace container
}  // namespace neuron