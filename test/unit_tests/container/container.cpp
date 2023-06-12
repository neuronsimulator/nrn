#include "neuron/container/soa_container.hpp"
#include "neuron/container/view_utils.hpp"

#include <catch2/catch.hpp>

using namespace neuron::container;

// Mock up a neuron::container::soa<...>-based data structure that includes features that are not
// currently tested in the real NEURON data structure code.

namespace {
namespace field {
/**
 * @brief Tag type that has zero-parameter array_dimension() and no num_variables().
 */
struct A {
    using type = float;
    [[nodiscard]] int array_dimension() const {
        return 42;
    }
};
}  // namespace field
template <typename Identifier>
struct handle_interface: handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;
    /**
     * @brief Return the above-diagonal element.
     */
    [[nodiscard]] field::A::type& a() {
        return this->template get<field::A>();
    }
};
struct storage: soa<storage, field::A> {};
using owning_handle = handle_interface<owning_identifier<storage>>;
}  // namespace

TEST_CASE("Tag type with array_dimension and without num_variables", "[Neuron][data_structures]") {
    storage data;  // Debian's GCC 10.2 doesn't like a {} before the ;
    owning_handle instance{data};
}
