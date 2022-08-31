#include "neuron/container/node.hpp"
#include "section.h"

#include <catch2/catch.hpp>

#include <optional>
#include <sstream>
#include <vector>

TEST_CASE("data_handle<double>", "[Neuron][data_structures][data_handle]") {
    GIVEN("A null handle") {
        neuron::container::data_handle<double> handle{};
        THEN("Check it is really null") {
            REQUIRE_FALSE(bool{handle});
        }
    }
    constexpr double magic_voltage_value = 42.;
    GIVEN("A handle wrapping a raw pointer (compatibility mode)") {
        double foo{magic_voltage_value};
        neuron::container::data_handle<double> handle{&foo};
        THEN("Check it is not null") {
            REQUIRE(bool{handle});
        }
        THEN("Check it yields the right value") {
            REQUIRE(*handle == foo);
        }
        THEN("Check it doesn't claim to be modern") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check it decays to the right raw pointer") {
            auto* foo_ptr = static_cast<double*>(handle);
            REQUIRE(foo_ptr == &foo);
        }
        THEN("Check it prints the right value") {
            std::ostringstream actual, expected{};
            actual << handle;
            expected << "data_handle<double>{raw=" << &foo << '}';
            REQUIRE(actual.str() == expected.str());
        }
    }
    GIVEN("A handle referring to an entry in an SOA container") {
        std::optional<Node> node{std::in_place};
        node->set_v(magic_voltage_value);
        auto handle = node->v_handle();
        THEN("Check it is not null") {
            REQUIRE(bool{handle});
        }
        THEN("Check it yields the right value") {
            REQUIRE(*handle == magic_voltage_value);
        }
        THEN("Check it claims to be modern") {
            REQUIRE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check it prints the right value") {
            std::ostringstream actual;
            actual << handle;
            REQUIRE(actual.str() == "data_handle<double>{cont=Node::field::Voltage row=0/1}");
        }
        THEN("Check that deleting the (Node) object it refers to invalidates the handle") {
            node.reset();
            REQUIRE_FALSE(bool{handle});
            std::ostringstream actual;
            actual << handle;
            REQUIRE(actual.str() == "data_handle<double>{cont=Node::field::Voltage died/0}");
        }
    }
}

template <typename Nodes>
std::vector<double> get_voltages(Nodes const& nodes) {
    std::vector<double> ret{};
    std::transform(nodes.begin(), nodes.end(), std::back_inserter(ret), [](auto const& node) {
        return node.v();
    });
    return ret;
}

TEST_CASE("SOA-backed Node structure", "[Neuron][data_structures][node]") {
    GIVEN("A series of nodes with increasing integer voltages") {
        std::vector<double> reference_voltages{};
        std::generate_n(std::back_inserter(reference_voltages), 10, [i = 0]() mutable {
            auto x = i++;
            return x * x;
        });
        std::vector<Node> nodes{};
        std::transform(reference_voltages.begin(),
                       reference_voltages.end(),
                       std::back_inserter(nodes),
                       [](auto v) {
                           Node node{};
                           node.set_v(v);
                           return node;
                       });
        THEN("Check we can read back the same voltages") {
            REQUIRE(get_voltages(nodes) == reference_voltages);
        }
    }
}