#pragma once
#include <memory>
#include <string_view>

namespace neuron {
struct Model;
inline Model& model();
namespace container {
template <typename>
struct data_handle;
namespace utils {
template <typename T>
[[nodiscard]] data_handle<T> find_data_handle(T* ptr);

/**
 * @brief Interface for obtaining information about model data containers.
 *
 * This indirection via an abstract interface helps reduce the ABI surface between translated MOD
 * file code and the rest of the library.
 */
struct storage_info {
    virtual ~storage_info() = default;
    virtual std::string_view container() const = 0;
    virtual std::string_view field() const = 0;
    virtual std::size_t size() const = 0;
};

/** @brief Try and find a helpful name for a container.
 *
 *  In practice this can be expected to be work for the structures that can be
 *  discovered from neuron::model(), and not for anything else. If no
 *  information about the container can be found the returned std::optional will
 *  not contain a value.
 */
[[nodiscard]] std::unique_ptr<storage_info> find_container_info(void const*);
}  // namespace utils
}  // namespace container
}  // namespace neuron
