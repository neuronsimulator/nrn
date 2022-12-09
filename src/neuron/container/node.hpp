#pragma once
#include "membdef.h"  // DEF_vrest
#include "neuron/container/data_handle.hpp"
#include "neuron/container/view_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace neuron::container::Node {
namespace field {
/** @brief Area in um^2 but see treesetup.cpp.
 */
struct Area {
    using type = double;
    constexpr type default_value() const {
        return 100.;
    }
};

/** @brief Membrane potential.
 */
struct Voltage {
    using type = double;
    constexpr type default_value() const {
        return DEF_vrest;
    }
};

struct RHS {
    using type = double;
    // Not needed for 0
    // constexpr type default_value() const {
    //     return 0.0;
    // }
};

}  // namespace field

/**
 * @brief Base class defining the public API of Node handles.
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
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;

    /** @brief Return the area.
     */
    [[nodiscard]] field::Area::type& area() {
        return this->template get<field::Area>();
    }

    /** @brief Return the area.
     */
    [[nodiscard]] field::Area::type const& area() const {
        return this->template get<field::Area>();
    }

    /**
     * @brief This is a workaround for area sometimes being a macro.
     * @todo Remove those macros once and for all.
     */
    [[nodiscard]] field::Voltage::type& area_hack() {
        return area();
    }

    /**
     * @brief This is a workaround for area sometimes being a macro.
     * @todo Remove those macros once and for all.
     */
    [[nodiscard]] field::Voltage::type const& area_hack() const {
        return area();
    }

    /** @brief Return a data_handle to the area.
     */
    [[nodiscard]] data_handle<field::Area::type> area_handle() {
        return this->template get_handle<field::Area>();
    }

    /** @brief Set the area.
     */
    void set_area(field::Area::type area) {
        this->template get<field::Area>() = area;
    }

    /**
     * @brief Return the membrane potential.
     */
    [[nodiscard]] field::Voltage::type& v() {
        return this->template get<field::Voltage>();
    }

    /**
     * @brief Return the membrane potential.
     */
    [[nodiscard]] field::Voltage::type const& v() const {
        return this->template get<field::Voltage>();
    }

    /**
     * @brief Return a handle to the membrane potential.
     */
    [[nodiscard]] data_handle<field::Voltage::type> v_handle() {
        return this->template get_handle<field::Voltage>();
    }

    /**
     * @brief This is a workaround for v sometimes being a macro.
     * @todo Remove those macros once and for all.
     */
    [[nodiscard]] field::Voltage::type& v_hack() {
        return v();
    }

    /**
     * @brief This is a workaround for v sometimes being a macro.
     * @todo Remove those macros once and for all.
     */
    [[nodiscard]] field::Voltage::type const& v_hack() const {
        return v();
    }

    /** @brief Set the membrane potential.
     */
    void set_v(field::Voltage::type v) {
        this->template get<field::Voltage>() = v;
    }

    /**
     * @brief Return the right hand side of the Hines solver.
     */
    [[nodiscard]] field::RHS::type& rhs() {
        return this->template get<field::RHS>();
    }

    /**
     * @brief Return the right hand side of the Hines solver.
     */
    [[nodiscard]] field::RHS::type const& rhs() const {
        return this->template get<field::RHS>();
    }

    /**
     * @brief Return a handle to the right hand side of the Hines solver.
     */
    [[nodiscard]] data_handle<field::RHS::type> rhs_handle() {
        return this->template get_handle<field::RHS>();
    }

    /** @brief Set the right hand side of the Hines solver.
     */
    void set_rhs(field::RHS::type rhs) {
        this->template get<field::RHS>() = rhs;
    }   

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        return os << "Node{" << handle.id() << '/' << handle.underlying_storage().size()
                  << " v=" << handle.v() << " area=" << handle.area() << '}';
    }
};
}  // namespace neuron::container::Node
