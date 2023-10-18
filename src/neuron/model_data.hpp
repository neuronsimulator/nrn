#pragma once
#include "neuron/cache/model_data.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/memory_usage.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/model_data_fwd.hpp"

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
    Model();   // defined in container.cpp
    ~Model();  // defined in container.cpp

    /** @brief Access the structure containing the data of all Nodes.
     */
    container::Node::storage& node_data() {
        return m_node_data;
    }

    /** @brief Access the structure containing the data of all Nodes.
     */
    container::Node::storage const& node_data() const {
        return m_node_data;
    }

    /** @brief Apply a function to each non-null Mechanism.
     */
    template <typename Callable>
    void apply_to_mechanisms(Callable const& callable) {
        for (auto type = 0; type < m_mech_data.size(); ++type) {
            if (!m_mech_data[type]) {
                continue;
            }
            callable(*m_mech_data[type]);
        }
    }

    template <typename Callable>
    void apply_to_mechanisms(Callable const& callable) const {
        for (auto type = 0; type < m_mech_data.size(); ++type) {
            if (!m_mech_data[type]) {
                continue;
            }
            callable(*m_mech_data[type]);
        }
    }

    /**
     * @brief Create a structure to hold the data of a new Mechanism.
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
        set_unsorted_callback(*m_mech_data[type]);
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
        return mechanism_data_impl(type);
    }

    /** @brief Get the structure holding the data of a particular Mechanism.
     */
    container::Mechanism::storage const& mechanism_data(int type) const {
        return mechanism_data_impl(type);
    }

    [[nodiscard]] std::size_t mechanism_storage_size() const {
        return m_mech_data.size();
    }

    [[nodiscard]] bool is_valid_mechanism(int type) const {
        return 0 <= type && type < mechanism_storage_size() && m_mech_data[type];
    }

    /**
     * @brief Find some metadata about the given container.
     *
     * The argument type will typically be a T* that contains the result of calling .data() on some
     * vector in the global model data structure.
     */
    [[nodiscard]] std::unique_ptr<container::utils::storage_info> find_container_info(
        void const* cont) const;

    void shrink_to_fit() {
        m_node_data.shrink_to_fit();
        apply_to_mechanisms([](auto& mech_data) { mech_data.shrink_to_fit(); });
    }

  private:
    container::Mechanism::storage& mechanism_data_impl(int type) const {
        if (0 <= type && type >= m_mech_data.size()) {
            throw std::runtime_error("mechanism_data(" + std::to_string(type) +
                                     "): type out of range");
        }
        const auto& data_ptr = m_mech_data[type];
        if (!data_ptr) {
            throw std::runtime_error("mechanism_data(" + std::to_string(type) +
                                     "): data for type was null");
        }

        return *data_ptr;
    }


    void set_unsorted_callback(container::Mechanism::storage& mech_data);

    /** @brief One structure for all Nodes.
     */
    container::Node::storage m_node_data;

    /** @brief Storage for mechanism-specific data.
     *
     *  Each element is allocated on the heap so that reallocating this vector
     *  does not invalidate pointers to container::Mechanism::storage.
     */
    std::vector<std::unique_ptr<container::Mechanism::storage>> m_mech_data{};

    /**
     * @brief Backing storage for defer_delete helper.
     */
    std::vector<void*> m_ptrs_for_deferred_deletion{};
};

struct model_sorted_token {
    model_sorted_token(cache::Model& cache,
                       container::Node::storage::frozen_token_type node_data_token_)
        : node_data_token{std::move(node_data_token_)}
        , m_cache{cache} {}
    [[nodiscard]] cache::Model& cache() {
        return m_cache;
    }
    [[nodiscard]] cache::Model const& cache() const {
        return m_cache;
    }
    void set_cache(cache::Model&& cache) {
        m_cache = cache;
    }
    [[nodiscard]] cache::Mechanism& mech_cache(std::size_t i) {
        return cache().mechanism.at(i);
    }
    [[nodiscard]] cache::Mechanism const& mech_cache(std::size_t i) const {
        return cache().mechanism.at(i);
    }
    [[nodiscard]] cache::Thread& thread_cache(std::size_t i) {
        return cache().thread.at(i);
    }
    [[nodiscard]] cache::Thread const& thread_cache(std::size_t i) const {
        return cache().thread.at(i);
    }
    container::Node::storage::frozen_token_type node_data_token;
    std::vector<container::Mechanism::storage::frozen_token_type> mech_data_tokens{};

  private:
    std::reference_wrapper<cache::Model> m_cache;
};

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

namespace container {
neuron::container::ModelMemoryUsage memory_usage(const Model& model);
}

}  // namespace neuron
