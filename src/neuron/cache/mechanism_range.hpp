#pragma once
#include "membfunc.h"
#include "nrn_ansi.h"
#include "nrnoc_ml.h"
#include "section.h"

#include <array>
#include <variant>

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
 */
template <std::size_t NumFloatingPointFields, std::size_t NumDatumFields>
struct MechanismRange {
    /**
     * @brief Construct a MechanismRange that refers to a single instance.
     *
     * This is used inside generated code that takes a single mechanism instance (Prop) instead of a
     * range of instances (Memb_list). A key feature of methods that take Prop is that they should
     * *not* require a call to nrn_ensure_model_data_are_sorted(). This is conceptually fine, as if
     * we are only concerned with a single mechanism instance then it doesn't matter where it lives
     * in the global storage vectors. In this case, m_token_or_cache contains an array of pointers
     * that m_dptr_datums can refer to.
     */
    MechanismRange(Prop* prop)
        : m_token_or_cache{dptr_cache_t{}} {
        if (!prop) {
            // grrr...see cagkftab test where setdata is not called(?) and extcall_prop is null(?)
            return;
        }
        for (auto i = 0; i < NumFloatingPointFields; ++i) {
            m_ptrs[i] = &prop->param(i);
            assert(m_ptrs[i]);
        }
        auto& cache = std::get<dptr_cache_t>(m_token_or_cache);
        indices_to_cache(prop->_type, [this, &cache, prop](auto field) {
            assert(field < NumDatumFields);
            auto& datum = prop->dparam[field];
            cache[field] = datum.template get<double*>();
            m_dptr_datums[field] = &cache[field];
        });
    }
    /**
     * @brief ...
     *
     * @todo Avoid calling nrn_ensure_model_data_are_sorted() every time.
     */
    MechanismRange(Memb_list& ml)
        : m_token_or_cache{nrn_ensure_model_data_are_sorted()} {
        auto const offset = ml.get_storage_offset();
        assert(offset != std::numeric_limits<std::size_t>::max());
        assert(NumFloatingPointFields == ml.num_floating_point_fields());
        for (auto i = 0; i < NumFloatingPointFields; ++i) {
            m_ptrs[i] = &ml.data(0, i);  // includes offset already
            assert(m_ptrs[i]);
        }
        auto& token = std::get<neuron::model_sorted_token>(m_token_or_cache);
        auto& mech_cache = token.mech_cache(ml.type());
        assert(mech_cache.pdata.size() <= NumDatumFields);
        for (auto i = 0; i < mech_cache.pdata.size(); ++i) {
            if (!mech_cache.pdata[i].empty()) {
                m_dptr_datums[i] = std::next(mech_cache.pdata[i].data(), offset);
            }
        }
    }
    template <std::size_t variable>
    [[nodiscard]] double& fpfield(std::size_t instance) {
        static_assert(variable < NumFloatingPointFields);
        return m_ptrs[variable][instance];
    }
    [[nodiscard]] double& data(std::size_t instance, std::size_t variable) {
        assert(variable < NumFloatingPointFields);
        return m_ptrs[variable][instance];
    }
    template <std::size_t variable>
    [[nodiscard]] double* dptr_field(std::size_t instance) {
        static_assert(variable < NumDatumFields);
        assert(m_dptr_datums[variable]);
        return m_dptr_datums[variable][instance];
    }
    /**
     * @brief Proxy type used in data_array.
     */
    template <std::size_t N>
    struct array_view {
        array_view(std::size_t offset, double** ptrs)
            : m_offset{offset} {
            for (auto i = 0; i < N; ++i) {
                m_ptrs[i] = ptrs[i];
            }
        }
        [[nodiscard]] double& operator[](std::size_t array_entry) {
            return m_ptrs[array_entry][m_offset];
        }
        [[nodiscard]] double const& operator[](std::size_t array_entry) const {
            return m_ptrs[array_entry][m_offset];
        }

      private:
        std::array<double*, N> m_ptrs{};
        std::size_t m_offset{};
    };
    template <std::size_t zeroth_variable, std::size_t array_size>
    [[nodiscard]] array_view<array_size> data_array(std::size_t instance) {
        static_assert(zeroth_variable + array_size < NumFloatingPointFields);
        return {instance, m_ptrs.data() + zeroth_variable};
    }

  private:
    std::array<double*, NumFloatingPointFields> m_ptrs{};
    std::array<double**, NumDatumFields> m_dptr_datums{};
    using dptr_cache_t = std::array<double*, NumDatumFields>;
    std::variant<neuron::model_sorted_token, dptr_cache_t> m_token_or_cache;
};

template <typename MechanismRange>
std::tuple<MechanismRange, MechanismRange*, std::size_t> make_nocmodl_macros_work(Prop* prop) {
    std::tuple<MechanismRange, MechanismRange*, std::size_t> ret{prop, nullptr, 0};
    std::get<1>(ret) = &std::get<0>(ret);
    return ret;
}

}  // namespace neuron::cache

namespace neuron::legacy {
/**
 * @brief Helper for legacy MOD files that mess with _p in VERBATIM blocks.
 */
template <typename MechRange>
inline void set_globals_from_prop(Prop* p, MechRange& ml, MechRange*& ml_ptr, std::size_t& iml) {
    auto [new_ml, new_mlptr, new_iml] = cache::make_nocmodl_macros_work<MechRange>(p);
    ml = std::move(new_ml);
    ml_ptr = &ml;
    iml = new_iml;
}
}  // namespace neuron::legacy
