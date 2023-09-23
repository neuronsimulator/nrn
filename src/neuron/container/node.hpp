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

/**
 * @brief Above-diagonal element in node equation.
 */
struct AboveDiagonal {
    using type = double;
};

/**
 * @brief Area in um^2 but see treeset.cpp.
 */
struct Area {
    using type = double;
    constexpr type default_value() const {
        return 100.;
    }
};

/**
 * @brief Below-diagonal element in node equation.
 */
struct BelowDiagonal {
    using type = double;
};

/** @brief Membrane potential.
 */
struct Voltage {
    using type = double;
    constexpr type default_value() const {
        return DEF_vrest;
    }
};

/**
 * @brief Diagonal element in node equation.
 */
struct Diagonal {
    using type = double;
};

/**
 * @brief Field for fast_imem calculation.
 */
struct FastIMemSavD {
    static constexpr bool optional = true;
    using type = double;
};

/**
 * @brief Field for fast_imem calculation.
 */
struct FastIMemSavRHS {
    static constexpr bool optional = true;
    using type = double;
};

struct RHS {
    using type = double;
};

}  // namespace field

/**
 * @brief Base class defining the public API of Node handles.
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
     * @brief Return the above-diagonal element.
     */
    [[nodiscard]] field::AboveDiagonal::type& a() {
        return this->template get<field::AboveDiagonal>();
    }

    /**
     * @brief Return the above-diagonal element.
     */
    [[nodiscard]] field::AboveDiagonal::type const& a() const {
        return this->template get<field::AboveDiagonal>();
    }

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
    [[nodiscard]] field::Area::type& area_hack() {
        return area();
    }

    /**
     * @brief This is a workaround for area sometimes being a macro.
     * @todo Remove those macros once and for all.
     */
    [[nodiscard]] field::Area::type const& area_hack() const {
        return area();
    }

    /** @brief Return a data_handle to the area.
     */
    [[nodiscard]] data_handle<field::Area::type> area_handle() {
        return this->template get_handle<field::Area>();
    }

    /**
     * @brief Return the below-diagonal element.
     */
    [[nodiscard]] field::BelowDiagonal::type& b() {
        return this->template get<field::BelowDiagonal>();
    }

    /**
     * @brief Return the below-diagonal element.
     */
    [[nodiscard]] field::BelowDiagonal::type const& b() const {
        return this->template get<field::BelowDiagonal>();
    }

    /**
     * @brief Return the diagonal element.
     */
    [[nodiscard]] field::Diagonal::type& d() {
        return this->template get<field::Diagonal>();
    }

    /**
     * @brief Return the diagonal element.
     */
    [[nodiscard]] field::Diagonal::type const& d() const {
        return this->template get<field::Diagonal>();
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

    [[nodiscard]] field::FastIMemSavRHS::type& sav_d() {
        return this->template get<field::FastIMemSavD>();
    }

    [[nodiscard]] field::FastIMemSavRHS::type const& sav_d() const {
        return this->template get<field::FastIMemSavD>();
    }

    [[nodiscard]] field::FastIMemSavRHS::type& sav_rhs() {
        return this->template get<field::FastIMemSavRHS>();
    }

    [[nodiscard]] field::FastIMemSavRHS::type const& sav_rhs() const {
        return this->template get<field::FastIMemSavRHS>();
    }
    [[nodiscard]] data_handle<field::FastIMemSavRHS::type> sav_rhs_handle() {
        return this->template get_handle<field::FastIMemSavRHS>();
    }

    friend std::ostream& operator<<(std::ostream& os, handle_interface const& handle) {
        if (handle.id()) {
            return os << "Node{" << handle.id() << '/' << handle.underlying_storage().size()
                      << " v=" << handle.v() << " area=" << handle.area() << " a=" << handle.a()
                      << " b=" << handle.b() << " d=" << handle.d() << '}';
        } else {
            return os << "Node{null}";
        }
    }
};
}  // namespace neuron::container::Node
