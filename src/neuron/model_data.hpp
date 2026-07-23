#pragma once
#include "neuron/cache/model_data.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/memory_usage.hpp"
#include "neuron/container/network/netcon.hpp"
#include "neuron/container/network/point_process.hpp"
#include "neuron/container/network/presyn.hpp"
#include "neuron/container/network/weights.hpp"
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

    /** @brief Access SoA storage for Point_process integration fields.
     *  @see doc/network-soa-phase0.md
     */
    container::network::PointProcess::storage& point_processes() {
        return m_point_processes;
    }
    container::network::PointProcess::storage const& point_processes() const {
        return m_point_processes;
    }

    /** @brief Access SoA storage for the flat NetCon weight pool.
     *  @see doc/network-soa-phase0.md
     */
    container::network::Weight::storage& weights() {
        return m_weights;
    }
    container::network::Weight::storage const& weights() const {
        return m_weights;
    }

    /** @brief Access SoA storage for NetCon integration fields (Phase 2).
     *  @see doc/network-soa-phase0.md §5.4
     */
    container::network::NetCon::storage& netcons() {
        return m_netcons;
    }
    container::network::NetCon::storage const& netcons() const {
        return m_netcons;
    }

    /** @brief Access SoA storage for PreSyn integration fields (Phase 3).
     *  @see doc/network-soa-phase0.md §5.5
     */
    container::network::PreSyn::storage& presyns() {
        return m_presyns;
    }
    container::network::PreSyn::storage const& presyns() const {
        return m_presyns;
    }

    /** @brief Apply a function to each non-null Mechanism.
     */
    template <typename Callable>
    void apply_to_mechanisms(Callable const& callable) {
        for (std::size_t type = 0; type < m_mech_data.size(); ++type) {
            if (!m_mech_data[type]) {
                continue;
            }
            callable(*m_mech_data[type]);
        }
    }

    template <typename Callable>
    void apply_to_mechanisms(Callable const& callable) const {
        for (std::size_t type = 0; type < m_mech_data.size(); ++type) {
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
        if (type < 0 || std::size_t(type) >= m_mech_data.size() || !m_mech_data[type]) {
            return;
        }
        if (std::size_t const size = m_mech_data[type]->size(); size > 0) {
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
        return 0 <= type && std::size_t(type) < mechanism_storage_size() && m_mech_data[type];
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
        m_point_processes.shrink_to_fit();
        m_weights.shrink_to_fit();
        m_netcons.shrink_to_fit();
        m_presyns.shrink_to_fit();
    }

  private:
    container::Mechanism::storage& mechanism_data_impl(int type) const {
        if (type < 0 || std::size_t(type) >= m_mech_data.size()) {
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

    /** @brief Network SoA: Point_process integration rows (Phase 1). */
    container::network::PointProcess::storage m_point_processes{};

    /** @brief Network SoA: flat weight pool (Phase 1). */
    container::network::Weight::storage m_weights{};

    /** @brief Network SoA: NetCon integration rows (Phase 2). */
    container::network::NetCon::storage m_netcons{};

    /** @brief Network SoA: PreSyn integration rows (Phase 3). */
    container::network::PreSyn::storage m_presyns{};

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
    /**
     * @brief Frozen tokens for network SoA containers (sorted with the model).
     *
     * Held as vectors (0 or 1 element) because frozen_token_type is not default-
     * constructible; same pattern as mech_data_tokens.
     */
    std::vector<container::network::PointProcess::storage::frozen_token_type>
        point_process_tokens{};
    std::vector<container::network::Weight::storage::frozen_token_type> weight_tokens{};
    std::vector<container::network::NetCon::storage::frozen_token_type> netcon_tokens{};
    std::vector<container::network::PreSyn::storage::frozen_token_type> presyn_tokens{};

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
