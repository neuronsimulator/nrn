#pragma once
#include "membfunc.h"
#include "nrn_ansi.h"
#include "nrnoc_ml.h"
#include "section.h"

#include <array>

namespace neuron::cache {
/**
 * @brief Call the given method with each dparam index that should be cached for a mechanism.
 *
 * The callable will be invoked with the largest index first. This is useful if you want to resize a
 * container and use the indices as offsets into it.
 */
template <typename Callable>
void indices_to_cache(short type, Callable callable) {
    auto const pdata_size = nrn_prop_dparam_size_[type];
    auto* const dparam_semantics = memb_func[type].dparam_semantics;
    for (int field = pdata_size - 1; field >= 0; --field) {
        // Check if the field-th dparam of this mechanism type is an ion variable. See
        // hoc_register_dparam_semantics.
        auto const sem = dparam_semantics[field];
        // TODO is area constant? could we cache the value instead of a pointer to it?
        // TODO ion type can be handled differently
        if ((sem > 0 && sem < 1000) || sem == -1 /* area */) {
            std::invoke(callable, field);
        }
    }
}

/**
 * @brief Version of Memb_list for use in performance-critical code.
 *
 * Unlike Memb_list, this requires that the number of fields is known at compile time.
 * This is typically only true in translated MOD file code. The idea is that an instance of this
 * class will be created outside a loop over the data vectors and then used inside the loop.
 *
 * It is the responsibility of the caller to ensure that the model remains sorted beyond the
 * lifetime of the MechanismRange instance.
 */
template <std::size_t NumFloatingPointFields, std::size_t NumDatumFields>
struct MechanismRange {
    MechanismRange(neuron::model_sorted_token const& cache_token,
                   NrnThread&,
                   Memb_list& ml,
                   int type)
        : MechanismRange{&neuron::model().mechanism_data(type), ml.get_storage_offset()} {
        auto const& ptr_cache = cache_token.mech_cache(type).pdata_ptr_cache;
        m_pdata_ptrs = ptr_cache.data();
        assert(ptr_cache.size() <= NumDatumFields);
    }

  protected:
    MechanismRange(container::Mechanism::storage* mech_data, std::size_t offset)
        : m_data_ptrs{mech_data
                          ? mech_data->get_data_ptrs<container::Mechanism::field::FloatingPoint>()
                          : nullptr}
        , m_data_array_dims{mech_data
                                ? mech_data
                                      ->get_array_dims<container::Mechanism::field::FloatingPoint>()
                                : nullptr}
        , m_offset{offset} {
        assert(!mech_data || (mech_data->num_floating_point_fields() == NumFloatingPointFields));
    }

  public:
    template <std::size_t variable>
    [[nodiscard]] double& fpfield(std::size_t instance) {
        static_assert(variable < NumFloatingPointFields);
        return m_data_ptrs[variable][m_offset + instance];
    }
    [[nodiscard]] double& data(std::size_t instance, int variable) {
        assert(variable < NumFloatingPointFields);
        return m_data_ptrs[variable][m_offset + instance];
    }
    [[nodiscard]] double& data(std::size_t instance, field_index ind) {
        assert(ind.field < NumFloatingPointFields);
        auto const array_dim = m_data_array_dims[ind.field];
        return m_data_ptrs[ind.field][array_dim * (m_offset + instance) + ind.array_index];
    }
    template <std::size_t variable>
    [[nodiscard]] double* dptr_field(std::size_t instance) {
        static_assert(variable < NumDatumFields);
        return m_pdata_ptrs[variable][m_offset + instance];
    }
    template <std::size_t zeroth_variable, std::size_t array_size>
    [[nodiscard]] double* data_array(std::size_t instance) {
        assert(array_size == m_data_array_dims[zeroth_variable]);
        return std::next(m_data_ptrs[zeroth_variable], array_size * (m_offset + instance));
    }

  protected:
    double* const* m_data_ptrs{};
    int const* m_data_array_dims{};
    double* const* const* m_pdata_ptrs{};
    std::size_t m_offset{};
};
/**
 * @brief Specialised version of MechanismRange for a single instance.
 *
 * This is used inside generated code that takes a single mechanism instance (Prop) instead of a
 * range of instances (Memb_list). A key feature of methods that take Prop is that they should
 * *not* require a call to nrn_ensure_model_data_are_sorted(). This is conceptually fine, as if
 * we are only concerned with a single mechanism instance then it doesn't matter where it lives
 * in the global storage vectors. In this case, m_token_or_cache contains an array of pointers
 * that m_dptr_datums can refer to.
 */
template <std::size_t NumFloatingPointFields, std::size_t NumDatumFields>
struct MechanismInstance: MechanismRange<NumFloatingPointFields, NumDatumFields> {
    using base_type = MechanismRange<NumFloatingPointFields, NumDatumFields>;
    MechanismInstance(Prop* prop)
        : base_type{prop ? &neuron::model().mechanism_data(prop->_type) : nullptr,
                    prop ? prop->current_row() : std::numeric_limits<std::size_t>::max()} {
        if (!prop) {
            // grrr...see cagkftab test where setdata is not called(?) and extcall_prop is null(?)
            return;
        }
        indices_to_cache(prop->_type, [this, prop](auto field) {
            assert(field < NumDatumFields);
            auto& datum = prop->dparam[field];
            m_dptr_cache[field] = datum.template get<double*>();
            this->m_dptr_datums[field] = &m_dptr_cache[field];
        });
        this->m_pdata_ptrs = m_dptr_datums.data();
    }
    MechanismInstance(MechanismInstance const& other) {
        *this = other;
    }
    MechanismInstance& operator=(MechanismInstance const& other) {
        if (this != &other) {
            this->m_data_ptrs = other.m_data_ptrs;
            this->m_data_array_dims = other.m_data_array_dims;
            this->m_offset = other.m_offset;
            m_dptr_cache = other.m_dptr_cache;
            for (auto i = 0; i < NumDatumFields; ++i) {
                m_dptr_datums[i] = &m_dptr_cache[i];
            }
            this->m_pdata_ptrs = m_dptr_datums.data();
        }
        return *this;
    }

  private:
    std::array<double*, NumDatumFields> m_dptr_cache{};
    std::array<double* const*, NumDatumFields> m_dptr_datums{};
};
}  // namespace neuron::cache

namespace neuron::legacy {
/**
 * @brief Helper for legacy MOD files that mess with _p in VERBATIM blocks.
 */
template <typename MechInstance, typename MechRange>
void set_globals_from_prop(Prop* p, MechInstance& ml, MechRange*& ml_ptr, std::size_t& iml) {
    ml = {p};
    ml_ptr = &ml;
    iml = 0;
}
}  // namespace neuron::legacy
