#include "neuron/container/generic_data_handle.hpp"
#include "neuron/container/node.hpp"
#include "neuron/model_data.hpp"

#include <catch2/catch.hpp>

#include <memory>

using namespace neuron::container;

// Boiler plate to generate a `soa` with one floating point field `A`.
namespace {
namespace field {
/**
 * @brief Tag type that has zero-parameter array_dimension() and no num_variables().
 */
struct A {
    using type = double;
};
}  // namespace field
template <typename Identifier>
struct handle_interface: public handle_base<Identifier> {
    using base_type = handle_base<Identifier>;
    using base_type::base_type;
    /**
     * @brief Return the above-diagonal element.
     */
    [[nodiscard]] field::A::type& a() {
        return this->template get<field::A>();
    }

    [[nodiscard]] data_handle<double> a_handle() {
        return this->template get_handle<field::A>();
    }
};
struct storage: soa<storage, field::A> {};
using owning_handle = handle_interface<owning_identifier<storage>>;
}  // namespace


TEST_CASE("data_handle validity", "[Neuron][data_structures][data_handle]") {
    data_handle<double> null_handle{};

    // We can check if it's valid like so:
    if (null_handle) {
        // Shouldn't be reached, because the handle is null.
        REQUIRE(false);
    } else {
        REQUIRE(true);
    }

    // What about formerly valid data_handles?
    auto container = storage{};
    auto row1 = std::make_unique<owning_handle>(container);
    auto row2 = std::make_unique<owning_handle>(container);

    auto dh = row1->a_handle();
    // Should be valid now.
    REQUIRE(dh);

    row1 = nullptr;
    row2 = nullptr;

    if (dh) {
        // Shouldn't be reached.
        REQUIRE(false);
    } else {
        // Do something/nothing with an invalid data_handle.
        REQUIRE(true);
    }
}

TEST_CASE("data_handle raw_is_valid", "[Neuron][data_structures][data_handle]") {
    auto& container = neuron::model();
    auto& node_data = container.node_data();

    auto row = std::make_unique<Node::owning_handle>(node_data);
    auto v = row->v_handle();

    auto ptr = static_cast<double*>(v);
    auto dh1 = data_handle<double>(ptr);

    // It's valid and points into a know soa container, i.e. it's "modern":
    REQUIRE(dh1);
    REQUIRE(dh1.refers_to_a_modern_data_structure());

    row = nullptr;

    // It's not null and therefore valid. It shouldn't be modern, right now.
    auto dh2 = data_handle<double>(ptr);
    REQUIRE(dh2);
    REQUIRE(!dh2.refers_to_a_modern_data_structure());

    // Allocate a row again.
    row = std::make_unique<Node::owning_handle>(node_data);

    // It's still not null. But now due to ABA-like issue it might report as
    // modern.
    auto dh3 = data_handle<double>(ptr);
    REQUIRE(dh3);
}
