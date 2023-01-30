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
                   NrnThread& nt,
                   Memb_list& ml,
                   int type)
        : m_data_ptrs{cache_token.mech_cache(type).data_ptr_cache.data()}
        , m_pdata_ptrs{cache_token.mech_cache(type).pdata_ptr_cache.data()}
        , m_offset{ml.get_storage_offset()} {
        assert(cache_token.mech_cache(type).data_ptr_cache.size() == NumFloatingPointFields);
        assert(cache_token.mech_cache(type).pdata_ptr_cache.size() <= NumDatumFields);
    }
    template <std::size_t variable>
    [[nodiscard]] double& fpfield(std::size_t instance) {
        static_assert(variable < NumFloatingPointFields);
        return m_data_ptrs[variable][m_offset + instance];
    }
    [[nodiscard]] double& data(std::size_t instance, std::size_t variable) {
        assert(variable < NumFloatingPointFields);
        return m_data_ptrs[variable][m_offset + instance];
    }
    template <std::size_t variable>
    [[nodiscard]] double* dptr_field(std::size_t instance) {
        static_assert(variable < NumDatumFields);
        return m_pdata_ptrs[variable][m_offset + instance];
    }
    /**
     * @brief Proxy type used in data_array.
     *
     * Note that this takes a pointer to an element of MechanismRange::m_ptrs, so any instance of
     * data_array must have a lifetime strictly shorter than that of the MechanismRange that
     * produced it.
     */
    template <std::size_t N>
    struct array_view {
        array_view(std::size_t offset, double* const* ptrs)
            : m_ptrs_in_mech_range_cache{ptrs}
            , m_offset{offset} {}
        [[nodiscard]] double& operator[](std::size_t array_entry) {
            return m_ptrs_in_mech_range_cache[array_entry][m_offset];
        }
        [[nodiscard]] double const& operator[](std::size_t array_entry) const {
            return m_ptrs_in_mech_range_cache[array_entry][m_offset];
        }

      private:
        double* const* m_ptrs_in_mech_range_cache{};
        std::size_t m_offset{};
    };
    template <std::size_t zeroth_variable, std::size_t array_size>
    [[nodiscard]] array_view<array_size> data_array(std::size_t instance) {
        static_assert(zeroth_variable + array_size < NumFloatingPointFields);
        return {m_offset + instance, m_data_ptrs + zeroth_variable};
    }

  protected:
    MechanismRange() = default;
    double* const* m_data_ptrs{};
    double* const* const* m_pdata_ptrs{};

  private:
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
        : m_ptrs{fill_ptrs(prop, std::make_index_sequence<NumFloatingPointFields>{})} {
        if (!prop) {
            // grrr...see cagkftab test where setdata is not called(?) and extcall_prop is null(?)
            return;
        }
        this->m_data_ptrs = m_ptrs.data();
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
            m_dptr_cache = other.m_dptr_cache;
            for (auto i = 0; i < NumDatumFields; ++i) {
                m_dptr_datums[i] = &m_dptr_cache[i];
            }
            m_ptrs = other.m_ptrs;
            this->m_data_ptrs = m_ptrs.data();
            this->m_pdata_ptrs = m_dptr_datums.data();
        }
        return *this;
    }

  private:
    template <std::size_t... Is>
    static std::array<double*, sizeof...(Is)> fill_ptrs(Prop* prop, std::index_sequence<Is...>) {
        if (!prop) {
            // see above
            return {};
        }
        assert(prop->param_size() == sizeof...(Is));
        return {&prop->param(Is)...};
    }
    std::array<double*, NumDatumFields> m_dptr_cache{};
    std::array<double* const*, NumDatumFields> m_dptr_datums{};
    std::array<double*, NumFloatingPointFields> m_ptrs{};
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
