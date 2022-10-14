#pragma once
#include "neuron/model_data_fwd.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/node_data.hpp"

#include <optional>
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

    /** @brief Create a structure to hold the data of a new Mechanism.
     */
    template <typename... Args>
    container::Mechanism::storage& add_mechanism(int type, Args&&... args) {
        if (type >= m_mech_data.capacity()) {
            m_mech_data.reserve(2 * m_mech_data.capacity());
        }
        if (type >= m_mech_data.size()) {
            m_mech_data.resize(type + 1);
        }
        if (m_mech_data[type]) {
            throw std::runtime_error("add_mechanism(" + std::to_string(type) +
                                     "): storage already exists");
        }
        m_mech_data[type] =
            std::make_unique<container::Mechanism::storage>(type, std::forward<Args>(args)...);
        return *m_mech_data[type];
    }

    /** @brief Destroy the structure holding the data of a particular mechanism.
     */
    void delete_mechanism(int type) {
        if (type >= m_mech_data.size() || !m_mech_data[type]) {
            return;
        }
        if (auto const size = m_mech_data[type]->size(); size > 0) {
            throw std::runtime_error("delete_mechanism(" + std::to_string(type) +
                                     "): refusing to delete storage that still hosts " +
                                     std::to_string(size) + " instances");
        }
        m_mech_data[type].reset();
    }

    /** @brief Get the structure holding the data of a particular Mechanism.
     */
    container::Mechanism::storage& mechanism_data(int type) {
        if (type >= m_mech_data.size()) {
            throw std::runtime_error("mechanism_data(" + std::to_string(type) +
                                     "): type out of range");
        }
        auto& data_ptr = m_mech_data[type];
        if (!data_ptr) {
            throw std::runtime_error("mechanism_data(" + std::to_string(type) +
                                     "): data for type was null");
        }
        return *data_ptr;
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

    /** @brief Storage for mechanism-specific data.
     *
     *  Each element is allocated on the heap so that reallocating this vector
     *  does not invalidate pointers to container::Mechanism::storage.
     */
    std::vector<std::unique_ptr<container::Mechanism::storage>> m_mech_data{};
};

using model_sorted_token = container::Node::storage::sorted_token_type;

namespace detail {
// Defining this inline seems to lead to duplicate copies when we dlopen
// libnrnmech.so , so we define it explicitly in container.cpp as part of
// libnrniv.so
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
