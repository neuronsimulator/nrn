#pragma once
#include "membfunc.h"
#include "nrn_ansi.h"
#include "nrnoc_ml.h"

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
        // See https://github.com/neuronsimulator/nrn/issues/2312 for discussion of possible
        // extensions to caching.
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
 * @warning It is the responsibility of the caller to ensure that the model remains sorted beyond
 * the lifetime of the MechanismRange instance.
 */
template <std::size_t NumFloatingPointFields, std::size_t NumDatumFields>
struct MechanismRange {
    /**
     * @brief Construct a MechanismRange from sorted model data.
     * @param cache_token Token showing the model data are sorted.
     * @param nt          Thread that this MechanismRange corresponds to.
     * @param ml          Range of mechanisms this MechanismRange refers to.
     * @param type        The type of this mechanism.
     *
     * This mirrors the signature of the functions (nrn_state, nrn_cur, nrn_init...) that are
     * generated in C++ from MOD files. Typically those generated functions immediately create an
     * instance of MechanismRange using this constructor.
     */
    MechanismRange(neuron::model_sorted_token const& cache_token,
                   NrnThread&,
                   Memb_list& ml,
                   int type)
        : MechanismRange{type, ml.get_storage_offset()} {
        auto const& ptr_cache = mechanism::_get::_pdata_ptr_cache_data(cache_token, type);
        m_pdata_ptrs = ptr_cache.data();
        assert(ptr_cache.size() <= NumDatumFields);
    }

  protected:
    /**
     * @brief Hidden helper constructor used by MechanismRange and MechanismInstance.
     */
    MechanismRange(int mech_type, std::size_t offset)
        : m_data_ptrs{mechanism::get_data_ptrs<double>(mech_type)}
        , m_data_array_dims{mechanism::get_array_dims<double>(mech_type)}
        , m_offset{offset} {
        assert((mech_type < 0) ||
               (mechanism::get_field_count<double>(mech_type) == NumFloatingPointFields));
    }

  public:
    /**
     * @brief Get the range of values for an array RANGE variable.
     * @tparam variable   The index of the RANGE variable in the mechanism.
     * @tparam array_size The array dimension of the RANGE variable.
     * @param instance    Which mechanism instance to access inside this mechanism range.
     */
    template <int variable, int array_size>
    [[nodiscard]] double* data_array(std::size_t instance) {
        static_assert(variable < NumFloatingPointFields);
        // assert(array_size == m_data_array_dims[variable]);
        return std::next(m_data_ptrs[variable], array_size * (m_offset + instance));
    }

    /**
     * @brief Get a RANGE variable value.
     * @tparam variable The index of the RANGE variable in the mechanism.
     * @param instance  Which mechanism instance to access inside this MechanismRange.
     *
     * This is only intended for use with non-array RANGE variables, otherwise use @ref data_array.
     */
    template <int variable>
    [[nodiscard]] double& fpfield(std::size_t instance) {
        return *data_array<variable, 1>(instance);
    }

    /**
     * @brief Get a RANGE variable value.
     * @param instance Which mechanism instance to access inside this MechanismRange.
     * @param ind      The index of the RANGE variable in the mechanism. This includes both the
     *                 index of the variable and the index into an array RANGE variable.
     */
    [[nodiscard]] double& data(std::size_t instance, container::field_index ind) {
        // assert(ind.field < NumFloatingPointFields);
        auto const array_dim = m_data_array_dims[ind.field];
        // assert(ind.array_index < array_dim);
        return m_data_ptrs[ind.field][array_dim * (m_offset + instance) + ind.array_index];
    }

    /**
     * @brief Get a POINTER variable.
     * @tparam variable The index of the POINTER variable in the pdata/dparam entries of the
     *                  mechanism.
     * @param instance  Which mechanism instance to access inside this mechanism range.
     */
    template <int variable>
    [[nodiscard]] double* dptr_field(std::size_t instance) {
        static_assert(variable < NumDatumFields);
        return m_pdata_ptrs[variable][m_offset + instance];
    }

  protected:
    /**
     * @brief Pointer to a range of pointers to the start of RANGE variable storage.
     *
     * @c m_data_ptrs[i] is a pointer to the start of the contiguous storage for the
     * @f$\texttt{i}^{th}@f$ RANGE variable.
     * @see container::detail::field_data<Tag, true>::data_ptrs()
     */
    double* const* m_data_ptrs{};

    /**
     * @brief Pointer to a range of array dimensions for the RANGE variables in this mechanism.
     *
     * @c m_data_array_dims[i] is the array dimension of the @f$\texttt{i}^{th}@f$ RANGE variable.
     */
    int const* m_data_array_dims{};

    /**
     * @brief Pointer to a range of pointers to the start of POINTER variable caches.
     *
     * @c m_pdata_ptrs[i][j] is the @c double* corresponding to the @f$\texttt{i}^{th}@f$ @c pdata /
     * @c dparam field and the @f$\texttt{j}^{th}@f$ instance of the mechanism in the program.
     * @see MechanismInstance::MechanismInstance(Prop*) and @ref nrn_sort_mech_data.
     */
    double* const* const* m_pdata_ptrs{};

    /**
     * @brief Offset of this contiguous range of mechanism instances into the global range.
     *
     * Typically if there is more than one thread in the process then the instances of a particular
     * mechanism type will be distributed across multiple NrnThread objects and processed by
     * different threads, and the mechanism data will be permuted so that the instances owned by a
     * given thread are contiguous. In that case the MechanismRange for the 0th thread would have an
     * @c m_offset of zero, and the MechanismRange for the next thread would have an @c m_offset of
     * the number of instances in the 0th thread.
     *
     * @see @ref nrn_sort_mech_data.
     */
    std::size_t m_offset{};
};
/**
 * @brief Specialised version of MechanismRange for a single instance.
 *
 * This is used inside generated code that takes a single mechanism instance (Prop) instead of a
 * range of instances (Memb_list). A key feature of methods that take Prop is that they should
 * *not* require a call to nrn_ensure_model_data_are_sorted(). This is conceptually fine, as if
 * we are only concerned with a single mechanism instance then it doesn't matter where it lives
 * in the global storage vectors. In this case, @ref m_dptr_cache contains an array of pointers
 * that @ref m_dptr_datums can refer to.
 */
template <std::size_t NumFloatingPointFields, std::size_t NumDatumFields>
struct MechanismInstance: MechanismRange<NumFloatingPointFields, NumDatumFields> {
    /**
     * @brief Shorthand for the MechanismRange base class.
     */
    using base_type = MechanismRange<NumFloatingPointFields, NumDatumFields>;

    /**
     * @brief Construct from a single mechanism instance.
     * @param prop Handle to a single mechanism instance.
     */
    MechanismInstance(Prop* prop)
        : base_type{_nrn_mechanism_get_type(prop), mechanism::_get::_current_row(prop)} {
        if (!prop) {
            // grrr...see cagkftab test where setdata is not called(?) and extcall_prop is null(?)
            return;
        }
        indices_to_cache(_nrn_mechanism_get_type(prop), [this, prop](auto field) {
            assert(field < NumDatumFields);
            auto& datum = _nrn_mechanism_access_dparam(prop)[field];
            m_dptr_cache[field] = datum.template get<double*>();
            this->m_dptr_datums[field] = &m_dptr_cache[field];
        });
        this->m_pdata_ptrs = m_dptr_datums.data();
    }

    /**
     * @brief Copy constructor.
     */
    MechanismInstance(MechanismInstance const& other) {
        *this = other;  // Implement using copy assignment.
    }

    /**
     * @brief Copy assignment
     *
     * This has to be implemented manually because the base class (MechanismInstance) member @ref
     * m_pdata_ptrs has to be updated to point at the derived class (MechanismInstance) member @ref
     * m_dptr_datums.
     */
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
    /**
     * @brief Cached @c double* values for this instance, calculated from @ref Datum.
     */
    std::array<double*, NumDatumFields> m_dptr_cache{};

    /**
     * @brief Pointers to m_dptr_cache needed to satisfy MechanismRange's requirements.
     * @invariant @ref m_dptr_datums[i] is equal to &@ref m_dptr_cache[i] for all @c i.
     * @invariant @c MechanismInstance::m_pdata_ptrs is equal to @ref m_dptr_datums.%data().
     */
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
