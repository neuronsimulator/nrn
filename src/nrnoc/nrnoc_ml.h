#pragma once
#include <cstddef>  // std::ptrdiff_t, std::size_t
#include <limits>   // std::numeric_limits
#include <vector>   // std::vector

struct Node;
struct Prop;

namespace neuron::container {
struct generic_data_handle;
}
using Datum = neuron::container::generic_data_handle;

// Only include a forward declaration to help avoid translated MOD file code relying on its layout
namespace neuron::container::Mechanism {
struct storage;
}

/**
 * @brief A view into a set of mechanism instances.
 *
 * This is a view to a set of mechanism instances that are contiguous in the
 * underlying storage. This is inherently something that only makes sense if
 * the underlying data are sorted. In this case, Memb_list essentially
 * contains a pointer to the underlying storage struct and a single offset into
 * it. This covers use-cases like Memb_list inside NrnThread -- the data are
 * partitioned by NrnThread in nrn_sort_mech_data so all the instances of a
 * particular mechanism in a particular thread are next to each other in the
 * storage.
 *
 * Because this type is passed from NEURON library code into code generated from MOD files, it is
 * prone to ABI issues -- particularly when dealing with Python wheels.
 */
struct Memb_list {
    /**
     * @brief Construct a null Memb_list that does not refer to any thread/type.
     */
    Memb_list() = default;

    /**
     * @brief Construct a Memb_list that knows its type + underlying storage.
     *
     * Defined in .cpp to hide neuron::container::Mechanism::storage layout from translated MOD file
     * code.
     */
    Memb_list(int type);

    Node** nodelist{};
    /* nodeindices contains all nodes this extension is responsible for,
     * ordered according to the matrix. This allows to access the matrix
     * directly via the nrn_actual_* arrays instead of accessing it in the
     * order of insertion and via the node-structure, making it more
     * cache-efficient */
    int* nodeindices{};
    Datum** pdata{};
    Prop** prop{};
    Datum* _thread{}; /* thread specific data (when static is no good) */
    int nodecount{};
    /**
     * @brief Get a vector of double* representing the model data.
     *
     * Calling .data() on the return value yields a double** that is similar to
     * the old _data member, with the key difference that its indices are
     * transposed. Now, the first index corresponds to the variable and the
     * second index corresponds to the instance of the mechanism. This method
     * is useful for interfacing with CoreNEURON but should be deprecated and
     * removed along with the translation layer between NEURON and CoreNEURON.
     *
     * Defined in .cpp to hide neuron::container::Mechanism::storage layout from translated MOD file
     * code.
     */
    [[nodiscard]] std::vector<double*> data();

    template <std::size_t variable>
    [[nodiscard]] double& fpfield(std::size_t instance) {
        return data(instance, variable);
    }

    template <std::size_t variable>
    [[nodiscard]] double* dptr_field(std::size_t instance) {
        return dptr_field(instance, variable);
    }

    [[nodiscard]] neuron::container::data_handle<double> data_handle(
        std::size_t instance,
        neuron::container::field_index field) const;

    /**
     * @brief Get the `variable`-th floating point value in `instance` of the mechanism.
     *
     * Defined in .cpp to hide neuron::container::Mechanism::storage layout from translated MOD file
     * code.
     */
    [[nodiscard]] __attribute__((pure)) double& data(std::size_t instance,
                                                     int variable,
                                                     int array_index = 0);

    /**
     * @brief Get the `variable`-th pointer-to-double in `instance` of the mechanism.
     *
     * Defined in .cpp to hide the full definition of Datum from translated MOD file code.
     */
    [[nodiscard]] __attribute__((pure)) double* dptr_field(std::size_t instance, int variable);

    /**
     * @brief Get the `variable`-th floating point value in `instance` of the mechanism.
     *
     * Defined in .cpp to hide neuron::container::Mechanism::storage layout from translated MOD file
     * code.
     */
    [[nodiscard]] __attribute__((pure)) double const& data(std::size_t instance,
                                                           int variable,
                                                           int array_index = 0) const;

    /**
     * @brief Calculate a legacy index of the given pointer in this mechanism data.
     *
     * This used to be defined as ptr - ml->_data[0] if ptr belonged to the
     * given mechanism, i.e. an offset from the zeroth element of the zeroth
     * mechanism. This is useful when interfacing with CoreNEURON and for
     * parameter exchange with other MPI ranks.
     *
     * Defined in .cpp to hide neuron::container::Mechanism::storage layout from translated MOD file
     * code.
     */
    [[nodiscard]] std::ptrdiff_t legacy_index(double const* ptr) const;

    /**
     * @brief Calculate a legacy index from a data handle.
     */
    [[nodiscard]] std::ptrdiff_t legacy_index(
        neuron::container::data_handle<double> const& dh) const {
        return legacy_index(static_cast<double const*>(dh));
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
        assert(m_storage_offset != neuron::container::invalid_row);
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

    /**
     * @brief Get the mechanism type.
     *
     * Defined in .cpp to hide neuron::container::Mechanism::storage layout from translated MOD file
     * code.
     */
    [[nodiscard]] int type() const;

    [[nodiscard]] int _type() const {
        return type();
    }

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
    std::size_t m_storage_offset{neuron::container::invalid_row};
};
