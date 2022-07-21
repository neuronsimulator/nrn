#pragma once
#include "neuron/model_data_fwd.hpp"
#include "neuron/container/node_data.hpp"

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

    /** @brief T* -> generic_handle<T> if ptr is in model data.
     */
    template <typename T>
    container::generic_handle<T> find_generic_handle(T* ptr) {
        // For now it could only be in m_node_data.
        return m_node_data.find_generic_handle(ptr);
    }

  private:
    /** @brief One structure for all Nodes.
     */
    container::Node::storage m_node_data{};
};

namespace detail {
inline Model model_data{};
}

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
generic_handle<T> find_generic_handle(T* ptr) {
    return model().find_generic_handle(ptr);
}
}  // namespace container::utils


}  // namespace neuron
