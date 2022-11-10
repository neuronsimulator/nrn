#pragma once
#include "hocdec.h"  // Datum
#include "neuron/container/mechanism_data.hpp"
#include "options.h"  // CACHEVEC

#include <algorithm>    // std::max_element
#include <array>        // std::array
#include <cstddef>      // std::ptrdiff_t, std::size_t
#include <iterator>     // std::distance, std::next
#include <limits>       // std::numeric_limits
#include <type_traits>  // std::conjunction_v
#include <vector>       // std::vector

struct Node;
struct Prop;

/**
 * @brief A view into a set of mechanism instances.
 *
 * This type gets used in a few different ways, and the interface with generated
 * code from MOD files makes it convenient to wrap these different use cases in
 * the same type:
 *  - The view can be to a set of mechanism instances that are contiguous in the
 *    underlying storage. This is inherently something that only makes sense if
 *    the underlying data are sorted. In this case, Memb_list essentially
 *    contains a pointer to the underlying storage struct and a single offset
 *    into it. This covers use-cases like Memb_list inside NrnThread -- the data
 *    are partitioned by NrnThread in nrn_sort_mech_data so all the instances of
 *    a particular mechanism in a particular thread are next to each other in
 *    the storage.
 *  - The view can be a list of stable handles to mechanism instances. This is
 *    useful in CVode code which needs to handle sets of mechanism instances in
 *    more flexible ways, where it might be excessively difficult or convoluted
 *    to guarantee in nrn_sort_mech_data that all sets that are ever needed are
 *    contiguous in the underlying storage.
 */
struct Memb_list {
    /**
     * @brief Construct a null Memb_list that does not refer to any thread/type.
     */
    Memb_list() = default;

    /**
     * @brief Construct a Memb_list that knows its type + underlying storage.
     */
    Memb_list(int type)
        : m_storage{&neuron::model().mechanism_data(type)} {}

    Node** nodelist;
#if CACHEVEC != 0
    /* nodeindices contains all nodes this extension is responsible for,
     * ordered according to the matrix. This allows to access the matrix
     * directly via the nrn_actual_* arrays instead of accessing it in the
     * order of insertion and via the node-structure, making it more
     * cache-efficient */
    int* nodeindices;
#endif /* CACHEVEC */
    Datum** pdata;
    Prop** prop;
    Datum* _thread; /* thread specific data (when static is no good) */
    int nodecount;
    /** @brief Get a vector of double* representing the model data.
     *
     *  Calling .data() on the return value yields a double** that is similar to
     *  the old _data member, with the key difference that its indices are
     *  transposed. Now, the first index corresponds to the variable and the
     *  second index corresponds to the instance of the mechanism. This method
     *  is useful for interfacing with CoreNEURON but should be deprecated and
     *  removed along with the translation layer between NEURON and CoreNEURON.
     */
    [[nodiscard]] std::vector<double*> data() {
        assert(m_storage);
        assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
        auto const num_fields = m_storage->num_floating_point_fields();
        std::vector<double*> ret(num_fields, nullptr);
        for (auto i = 0ul; i < num_fields; ++i) {
            ret[i] =
                &m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                    i, m_storage_offset);
        }
        return ret;
    }

    /**
     * @brief Get the `variable`-th floating point value in `instance` of the mechanism.
     */
    [[nodiscard]] double& data(std::size_t instance, std::size_t variable) {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // non-contiguous mode
            return instances.at(instance).fpfield(variable);
        } else {
            // contiguous mode
            assert(m_storage);
            assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
            return m_storage
                ->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                    variable, m_storage_offset + instance);
        }
    }

    /**
     * @brief Get the `variable`-th floating point value in `instance` of the mechanism.
     */
    [[nodiscard]] double const& data(std::size_t instance, std::size_t variable) const {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // non-contiguous mode
            return instances.at(instance).fpfield(variable);
        } else {
            // contiguous mode
            assert(m_storage);
            return m_storage
                ->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                    variable, m_storage_offset + instance);
        }
    }

    /** @brief Calculate a legacy index of the given pointer in this mechanism data.
     *
     *  This used to be defined as ptr - ml->_data[0] if ptr belonged to the
     *  given mechanism, i.e. an offset from the zeroth element of the zeroth
     *  mechanism. This is useful when interfacing with CoreNEURON and for
     *  parameter exchange with other MPI ranks.
     */
    [[nodiscard]] std::ptrdiff_t legacy_index(double const* ptr) const {
        assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
        auto const size = m_storage->size();
        auto const num_fields = m_storage->num_floating_point_fields();
        for (auto field = 0ul; field < num_fields; ++field) {
            auto const* const vec_data =
                &m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                    field, 0);
            auto const index = std::distance(vec_data, ptr);
            if (index >= 0 && index < size) {
                // ptr lives in the field-th data column
                return (index - m_storage_offset) * num_fields + field;
            }
        }
        // ptr doesn't live in this mechanism data, cannot compute a legacy index
        return -1;
    }

    /**
     * @brief Calculate a legacy index from a data handle.
     */
    [[nodiscard]] std::ptrdiff_t legacy_index(
        neuron::container::data_handle<double> const& dh) const {
        return legacy_index(static_cast<double const*>(dh));
    }

    /**
     * @brief Proxy type used in data_array.
     */
    struct array_view {
        array_view(neuron::container::Mechanism::storage* storage,
                   std::size_t instance,
                   std::size_t zeroth_column)
            : m_storage{storage}
            , m_instance{instance}
            , m_zeroth_column{zeroth_column} {}
        [[nodiscard]] double& operator[](std::size_t array_entry) {
            return m_storage
                ->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                    m_zeroth_column + array_entry, m_instance);
        }
        [[nodiscard]] double const& operator[](std::size_t array_entry) const {
            return m_storage
                ->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                    m_zeroth_column + array_entry, m_instance);
        }

      private:
        neuron::container::Mechanism::storage* m_storage{};
        std::size_t m_instance{}, m_zeroth_column{};
    };

    /**
     * @brief Get a proxy to a range of variables in a mechanism instance.
     *
     * Concretely
     *   auto proxy = ml.data_array(instance, zeroth_variable)
     *   proxy[variable_index]
     * is equivalent to
     *   ml.data(instance, zeroth_variable + variable_index)
     * this is used in translated MOD file code for array variables. Ideally
     * it would be deprecated and removed, with data(i, j) used instead.
     */
    [[nodiscard]] array_view data_array(std::size_t instance, std::size_t zeroth_variable) {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // not in contiguous mode
            auto& handle = instances.at(instance);
            return {&handle.underlying_storage(), handle.current_row(), zeroth_variable};
        } else {
            // contiguous mode
            assert(m_storage);
            return {m_storage, m_storage_offset + instance, zeroth_variable};
        }
    }

    /** @brief Helper for compatibility with legacy code.
     *
     * The scopmath solvers have a complicated relationship with NEURON data
     * structures. The simplest solution for now seems to be to modify the
     * scopmath code to follow an extra layer of indirection, and then when
     * calling into it to generate a temporary vector of pointers to the right
     * elements in SOA format. In the old AOS data then ml->data[i] could be
     * used directly, but no more... If the scopmath code is removed or updated
     * to be compiled as C++ then this layer could go away again.
     */
    template <typename... ListTypes>
    [[nodiscard]] std::vector<double*> vector_of_pointers_for_scopmath(std::size_t instance,
                                                                       std::size_t num_eqns,
                                                                       ListTypes... lists) {
        static_assert(sizeof...(ListTypes) > 0);
        static_assert(std::conjunction_v<std::is_pointer<ListTypes>...>);
        assert(num_eqns);
        assert((lists && ...));  // all lists should be non-null
        std::array const max_elements{*std::max_element(lists, lists + num_eqns)...};
        std::vector<double*> p(1 + *std::max_element(max_elements.begin(), max_elements.end()));
        for (auto i = 0; i < num_eqns; ++i) {
            ((p[lists[i]] = &data(instance, lists[i])), ...);
        }
        return p;
    }

    /**
     * @brief Get the offset of this Memb_list into global storage for this type.
     *
     * In the simplest case then this Memb_list represents all instances of a
     * particular type in a particular thread, which are contiguous because the
     * data have been sorted by a call like
     *   auto const cache_token = nrn_ensure_model_data_are_sorted()
     * and this offset has been set to be
     *   cache_token.thread_cache(thread_id).mechanism_offset.at(mechanism_type)
     * so that the first argument to data(i, j) is an offset inside this thread,
     * not the global structure.
     */
    [[nodiscard]] std::size_t get_storage_offset() const {
        assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
        return m_storage_offset;
    }

    /**
     * @brief Set the offset of this Memb_list into global storage for this type.
     *
     * See the documentation for @ref get_storage_offset.
     *
     * @todo At the moment this is set as part of sorting/permuting data, but it
     *       is not automatically invalidated when the cache / sorted status is
     *       reset. Consider if these offsets can be more explicitly tied to the
     *       lifetime of the cache data.
     */
    void set_storage_offset(std::size_t offset) {
        m_storage_offset = offset;
    }

    /**
     * @brief Set the pointer to the underlying data container.
     *
     * This is a quasi-private method that you should think twice before
     * calling. Normally m_storage would automatically be set by the constructor
     * taking an integer mechanism type.
     */
    void set_storage_pointer(neuron::container::Mechanism::storage* storage) {
        m_storage = storage;
    }

    /** @brief Get a stable handle to the current instance-th handle.
     *
     *  The returned value refers to the same instance of this mechanism
     *  regardless of how the underlying storage are permuted.
     */
    [[nodiscard]] neuron::container::Mechanism::handle instance_handle(std::size_t instance) {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // not in contiguous mode
            return instances.at(instance);
        } else {
            assert(m_storage);
            return m_storage->at(m_storage_offset + instance);
        }
    }

    /**
     * @brief List of stable handles to mechanism instances.
     *
     * See the struct-level documentation. This is the list of handles that is
     * used from CVode code.
     */
    std::vector<neuron::container::Mechanism::handle> instances{};

  private:
    /**
     * @brief Pointer to the global mechanism data structure for this mech type.
     */
    neuron::container::Mechanism::storage* m_storage{};

    /**
     * @brief Offset of this thread+mechanism into the global mechanism data.
     *
     * This is locally a piece of "cache" information like node_data_offset --
     * the question is whether Memb_list itself is also considered a
     * transient/cache type, in which case this is fine, or if it's considered
     * permanent...in which case this value should probably not live here.
     */
    std::size_t m_storage_offset{std::numeric_limits<std::size_t>::max()};
};
