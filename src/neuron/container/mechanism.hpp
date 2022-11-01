#pragma once
#include "neuron/container/data_handle.hpp"
#include "neuron/container/view_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace neuron::container::Mechanism {
namespace field {
/** @brief Catch-all for floating point per-instance variables in the MOD file.
 *
 *  @todo Update the code generation so we get some hh_data = soa<hh_identifier,
 *  hh_a, hh_b, ...> type instead of fudging things this way.
 */
struct FloatingPoint {
    constexpr FloatingPoint(std::size_t num_copies)
        : m_num_copies{num_copies} {}
    /** @brief How many copes of this column should be created?
     */
    constexpr std::size_t num_instances() const {
        return m_num_copies;
    }
    using type = double;

  private:
    std::size_t m_num_copies{};
};
}  // namespace field

/**
 * @brief Base class defining the public API of Mechanism handles.
 * @tparam Identifier The concrete owning/non-owning identifier type.
 *
 * This allows the same struct-like accessors (v(), set_v(), ...) to be
 * used on all of the different types of objects that represent a single Node:
 * - owning_handle: stable over permutations of underlying data, manages
 *   lifetime of a row in the underlying storage. Only null when in moved-from
 *   state.
 * - handle: stable over permutations of underlying data, produces runtime error
 *   if it is dereferenced after the corresponding owning_handle has gone out of
 *   scope. Can be null.
 */
template <typename Identifier>
struct interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    /** @brief Return the number of floating point fields accessible via fpfield.
     */
    [[nodiscard]] std::size_t num_fpfields() const {
        return this->template get_tag<field::FloatingPoint>().num_instances();
    }

    [[nodiscard]] field::FloatingPoint::type& fpfield(std::size_t field_index) {
        if (field_index >= num_fpfields()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) +
                                     ") field index out of range");
        }
        return this->template get<field::FloatingPoint>(field_index);
    }

    [[nodiscard]] field::FloatingPoint::type const& fpfield(std::size_t field_index) const {
        if (field_index >= num_fpfields()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) +
                                     ") field index out of range");
        }
        return this->template get<field::FloatingPoint>(field_index);
    }

    /** @brief Return a data_handle to the area.
     */
    [[nodiscard]] data_handle<field::FloatingPoint::type> fpfield_handle(std::size_t field_index) {
        if (field_index >= num_fpfields()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) +
                                     ") field index out of range");
        }
        return this->template get_handle<field::FloatingPoint>(field_index);
    }

    /** @brief Set the area.
     */
    void set_fpfield(std::size_t field_index, field::FloatingPoint::type area) {
        if (field_index >= num_fpfields()) {
            throw std::runtime_error("Mechanism::fpfield(" + std::to_string(field_index) +
                                     ") field index out of range");
        }
        this->template get<field::FloatingPoint>(field_index) = area;
    }
    friend std::ostream& operator<<(std::ostream& os, interface const& handle) {
        os << handle.underlying_storage().name() << '{' << handle.id() << '/'
           << handle.underlying_storage().size();
        auto const num = handle.num_fpfields();
        for (auto i = 0ul; i < num; ++i) {
            os << ' ' << handle.fpfield(i);
        }
        return os << '}';
    }
};
}  // namespace neuron::container::Mechanism
