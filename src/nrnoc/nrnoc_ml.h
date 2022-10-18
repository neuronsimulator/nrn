#pragma once
#include "hocdec.h"
#include "neuron/container/mechanism.hpp"
#include "options.h"  // for CACHEVEC

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

struct Node;
struct Prop;

/** @brief A view into a set of mechanism instances.
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
    Memb_list() = default;
    // Bit of a hack for codegen
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
    // double** _data; // historically this was _data[i_instance][j_variable] but now it is
    // transposed
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
     *  is useful for CoreNEURON interface
     */
    [[nodiscard]] std::vector<double*> data() {
        assert(m_storage);
        assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
        std::vector<double*> ret{};
        auto const num_fields = m_storage->num_floating_point_fields();
        ret.resize(num_fields);
        // use const-qualified get_field_instance so that it's OK to call this
        // method when the structure is frozen
        auto const* const const_storage = m_storage;
        for (auto i = 0ul; i < num_fields; ++i) {
            ret[i] = std::next(
                const_cast<double*>(
                    const_storage
                        ->get_field_instance<
                            neuron::container::Mechanism::field::PerInstanceFloatingPointField>(i)
                        .data()),
                m_storage_offset);
        }
        return ret;
    }
    [[nodiscard]] double& data(std::size_t instance, std::size_t variable) {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // non-contiguous mode
            return instances.at(instance).fpfield_ref(variable);
        } else {
            // contiguous mode
            assert(m_storage);
            assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
            return m_storage->get_field_instance<
                neuron::container::Mechanism::field::PerInstanceFloatingPointField>(
                variable, m_storage_offset + instance);
        }
    }
    [[nodiscard]] double const& data(std::size_t instance, std::size_t variable) const {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // non-contiguous mode
            return instances.at(instance).fpfield_ref(variable);
        } else {
            // contiguous mode
            assert(m_storage);
            assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
            return m_storage->get_field_instance<
                neuron::container::Mechanism::field::PerInstanceFloatingPointField>(
                variable, m_storage_offset + instance);
        }
    }
    /** @brief Calculate a legacy index of the given pointer in this mechanism data.
     *
     *  This used to be defined as ptr - ml->_data[0] if ptr belonged to the
     *  given mechanism, i.e. an offset from the zeroth element of the zeroth
     *  mechanism.
     */
    [[nodiscard]] std::ptrdiff_t legacy_index(double const* ptr) const {
        auto const num_fields = m_storage->num_floating_point_fields();
        for (auto field = 0ul; field < num_fields; ++field) {
            auto const* const const_storage = m_storage;
            auto const& vec = const_storage->get_field_instance<
                neuron::container::Mechanism::field::PerInstanceFloatingPointField>(field);
            auto const index = std::distance(vec.data(), ptr);
            if (index >= 0 && index < vec.size()) {
                // ptr lives in the field-th data column
                return index * num_fields + field;
            }
        }
        // ptr doesn't live in this mechanism data, cannot compute a legacy index
        return -1;
    }
    [[nodiscard]] std::size_t num_data_fields() const {
        assert(m_storage);
        return m_storage->num_floating_point_fields();
    }
    struct array_view {
        array_view(neuron::container::Mechanism::storage* storage,
                   std::size_t instance,
                   std::size_t zeroth_column)
            : m_storage{storage}
            , m_instance{instance}
            , m_zeroth_column{zeroth_column} {}
        [[nodiscard]] double& operator[](std::size_t array_entry) {
            return m_storage->get_field_instance<
                neuron::container::Mechanism::field::PerInstanceFloatingPointField>(
                m_zeroth_column + array_entry, m_instance);
        }

      private:
        neuron::container::Mechanism::storage* m_storage{};
        std::size_t m_instance{}, m_zeroth_column{};
    };
    [[nodiscard]] array_view data_array(std::size_t instance, std::size_t zeroth_variable) {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // not in contiguous mode
            auto& handle = instances.at(instance);
            return {&handle.underlying_storage(), handle.id().current_row(), zeroth_variable};
        } else {
            // contiguous mode
            assert(m_storage);
            return {m_storage, m_storage_offset + instance, zeroth_variable};
        }
    }
    /** @brief Helper for compatibility with legacy code.
     *
     *  Some methods assume that we can get a double* that points to the
     *  properties of a particular mechanism. As the underlying storage has now
     *  been transposed, this is not possible without creating a copy.
     *
     *  @todo (optionally) return a type whose destructor propagates
     *  modifications back into the NEURON data structures.
     */
    // [[nodiscard]] std::vector<double> contiguous_row(std::size_t instance) {
    //     std::vector<double> data;
    //     if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
    //         // not in contiguous mode
    //         auto const& handle = instances.at(instance);
    //         auto const num_fields = handle.num_fpfields();
    //         data.reserve(num_fields);
    //         for (auto i_field = 0; i_field < num_fields; ++i_field) {
    //             data.push_back(handle.fpfield(i_field));
    //         }
    //     } else {
    //         // contiguous mode
    //         assert(m_storage);
    //         assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
    //         auto const num_fields = m_storage->num_floating_point_fields();
    //         data.reserve(num_fields);
    //         for (auto i_field = 0; i_field < num_fields; ++i_field) {
    //             data.push_back(m_storage->get_field_instance<
    //                            neuron::container::Mechanism::field::PerInstanceFloatingPointField>(
    //                 i_field, m_storage_offset + instance));
    //         }
    //     }
    //     return data;
    // }

    /** @brief Helper for compatibility with legacy code.
     *
     * The scopmath solvers have a complicated relationship with NEURON data
     * structures. The simplest solution for now seems to be to modify the
     * scopmath code to follow an extra layer of indirection, and then when
     * calling into it to generate a temporary vector of pointers to the right
     * elements in SOA format. In the old AOS data then ml->data[i] could be
     * used directly, but no more...
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

    [[nodiscard]] std::size_t get_storage_offset() const {
        return m_storage_offset;
    }
    /** @todo how to invalidate this?
     */
    void set_storage_offset(std::size_t offset) {
        m_storage_offset = offset;
    }
    void set_storage_pointer(neuron::container::Mechanism::storage* storage) {
        m_storage = storage;
    }

    /** @brief Get a stable handle to the current instance-th handle.
     *
     *  The returned value refers to the same instance of this mechanism
     *  regardless of how the underlying storage are permuted.
     */
    [[nodiscard]] neuron::container::Mechanism::handle instance_handle(std::size_t instance) const {
        if (m_storage_offset == std::numeric_limits<std::size_t>::max()) {
            // not in contiguous mode
            return instances.at(instance);
        } else {
            assert(m_storage);
            return {m_storage->identifier(instance), *m_storage};
        }
    }

    std::vector<neuron::container::Mechanism::handle> instances{};

  private:
    neuron::container::Mechanism::storage* m_storage{};
    std::size_t m_storage_offset{std::numeric_limits<std::size_t>::max()};
};
