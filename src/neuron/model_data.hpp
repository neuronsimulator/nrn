#pragma once
#include "neuron/model_data_fwd.hpp"
#include "neuron/container/node_data.hpp"

#include <sstream>

namespace neuron {
/** @brief Top-level structure.
 *
 * This level of indirection (as opposed to, for example, the Node data being a
 * global variable in its own right) will give us more control over
 * construction/destruction/... of different parts of the model data.
 */
struct Model {
    /** @brief Access the structure containing the data of all Nodes.
     */
    container::Node::storage& node_data() {
        return m_node_data;
    }

    /** @brief T* -> data_handle<T> if ptr is in model data.
     */
    template <typename T>
    [[nodiscard]] container::data_handle<T> find_data_handle(T* ptr) {
        // For now it could only be in m_node_data.
        return m_node_data.find_data_handle(ptr);
    }

    [[nodiscard]] std::optional<container::utils::storage_info> find_container_info(
        void const* cont) const {
        // For now it could only be m_node_data
        auto maybe_info = m_node_data.find_container_info(cont);
        if (maybe_info) {
            return maybe_info;
        }
        return {std::nullopt};
    }

  private:
    /** @brief One structure for all Nodes.
     */
    container::Node::storage m_node_data{};
};

namespace detail {
// Defining this inline seems to lead to duplicate copies when we
// dlopen(libnrnmech.so) -- need to investigate that more later.
extern Model model_data;
}  // namespace detail

/** @brief Access the global Model instance.
 *
 *  Just to be going on with. Needs more thought about who actually holds/owns
 *  the structures that own the SOA data. Could use a static local if we need to
 *  control/defer when this is constructed.
 */
inline Model& model() {
    return detail::model_data;
}

namespace container::utils {
template <typename T>
data_handle<T> find_data_handle(T* ptr) {
    return model().find_data_handle(ptr);
}

[[nodiscard]] inline std::optional<storage_info> find_container_info(void const* c) {
    return model().find_container_info(c);
}

}  // namespace container::utils


}  // namespace neuron
