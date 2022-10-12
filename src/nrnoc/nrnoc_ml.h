#pragma once
#include "hocdec.h"
#include "options.h"  // for CACHEVEC

struct Node;
struct Prop;

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
     *  second index corresponds to the instance of the mechanism.
     */
    [[nodiscard]] std::vector<double*> data() {
        assert(m_storage);
        assert(m_storage->is_sorted());
        std::vector<double*> ret{};
        auto const num_fields = m_storage->num_floating_point_fields();
        ret.resize(num_fields);
        for (auto i = 0ul; i < num_fields; ++i) {
            ret[i] = m_storage
                         ->get_field_instance<
                             neuron::container::Mechanism::field::PerInstanceFloatingPointField>(i)
                         .data();
        }
        return ret;
    }
    [[nodiscard]] double& data(std::size_t instance, std::size_t variable) {
        assert(m_storage);
        assert(m_storage->is_sorted());
        return m_storage->get_field_instance<
            neuron::container::Mechanism::field::PerInstanceFloatingPointField>(variable, instance);
    }
    [[nodiscard]] double const& data(std::size_t instance, std::size_t variable) const {
        assert(m_storage);
        assert(m_storage->is_sorted());
        return m_storage->get_field_instance<
            neuron::container::Mechanism::field::PerInstanceFloatingPointField>(variable, instance);
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
            auto const& vec = m_storage->get_field_instance<
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
        return {m_storage, instance, zeroth_variable};
    }

  private:
    neuron::container::Mechanism::storage* m_storage{};
};
