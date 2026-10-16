#pragma once
#include "neuron/container/data_handle.hpp"
#include "neuron/container/view_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace neuron::container::Mechanism {
struct Variable {
    std::string name{};
    int array_size{1};
};
namespace field {
/** @brief Catch-all for floating point per-instance variables in the MOD file.
 *
 *  @todo Update the code generation so we get some hh_data = soa<hh_identifier,
 *  hh_a, hh_b, ...> type instead of fudging things this way.
 */
struct FloatingPoint {
    FloatingPoint(std::vector<Variable> var_info)
        : m_var_info{std::move(var_info)} {}
    /**
     * @brief How many copies of this column should be created?
     *
     * Typically this is the number of different RANGE variables in a mechanism.
     */
    [[nodiscard]] std::size_t num_variables() const {
        return m_var_info.size();
    }

    [[nodiscard]] Variable const& info(std::size_t i) const {
        return m_var_info.at(i);
    }

    /**
     * @brief What is the array dimension of the i-th copy of this column?
     *
     * Typically this is 1 for a normal RANGE variable and larger for an array RANGE variable.
     */
    [[nodiscard]] int array_dimension(int i) const {
        return info(i).array_size;
    }

    [[nodiscard]] const char* name(int i) const {
        return info(i).name.c_str();
    }

    using type = double;

  private:
    std::vector<Variable> m_var_info{};
};
}  // namespace field

/**
 * @brief Base class defining the public API of Mechanism handles.
 * @tparam Identifier The concrete owning/non-owning identifier type.
 *
 * This allows the same struct-like accessors (v(), ...) to be
 * used on all of the different types of objects that represent a single Node:
 * - owning_handle: stable over permutations of underlying data, manages
 *   lifetime of a row in the underlying storage. Only null when in moved-from
 *   state.
 * - handle: stable over permutations of underlying data, produces runtime error
 *   if it is dereferenced after the corresponding owning_handle has gone out of
 *   scope. Can be null.
 */
template <typename Identifier>
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    /**
     * @brief Return the number of floating point fields accessible via fpfield.
     */
    [[nodiscard]] int num_fpfields() const {
        return this->template get_tag<field::FloatingPoint>().num_variables();
    }

    /**
     * @brief Get the sum of array dimensions of floating point fields.
     */
    [[nodiscard]] int fpfields_size() const {
        auto* const prefix_sums =
            this->underlying_storage().template get_array_dim_prefix_sums<field::FloatingPoint>();
        auto const num_fields = num_fpfields();
        return prefix_sums[num_fields - 1];
    }

    /**
     * @brief Return the array size of the given field.
     */
    [[nodiscard]] int fpfield_dimension(int field_index) const {
        if (auto const num_fields = num_fpfields(); field_index >= num_fields) {
            throw std::runtime_error("fpfield #" + std::to_string(field_index) + " out of range (" +
                                     std::to_string(num_fields) + ")");
        }
        auto* const array_dims =
            this->underlying_storage().template get_array_dims<field::FloatingPoint>();
        return array_dims[field_index];
    }

    [[nodiscard]] field::FloatingPoint::type& fpfield(int field_index, int array_index = 0) {
        return this->underlying_storage().fpfield(this->current_row(), field_index, array_index);
    }

    [[nodiscard]] field::FloatingPoint::type const& fpfield(int field_index,
                                                            int array_index = 0) const {
        return this->underlying_storage().fpfield(this->current_row(), field_index, array_index);
    }

    /** @brief Return a data_handle to a floating point value.
     */
    [[nodiscard]] data_handle<field::FloatingPoint::type> fpfield_handle(int field_index,
                                                                         int array_index = 0) {
        return this->underlying_storage().fpfield_handle(this->id(), field_index, array_index);
    }

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        os << handle.underlying_storage().name() << '{' << handle.id() << '/'
           << handle.underlying_storage().size();
        auto const num = handle.num_fpfields();
        for (auto i = 0; i < num; ++i) {
            os << " fpfield[" << i << "]{";
            for (auto j = 0; j < handle.fpfield_dimension(i); ++j) {
                os << " " << handle.fpfield(i, j);
            }
            os << " }";
        }
        return os << '}';
    }
};
}  // namespace neuron::container::Mechanism
