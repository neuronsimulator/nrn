#include "neuron/container/mechanism.hpp"

#include <catch2/catch.hpp>

#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

using namespace neuron::container::Mechanism;

TEST_CASE("SOA-backed Mechanism data structure", "[Neuron][data_structures][mechanism]") {
    GIVEN("A mechanism with two copies of the same tagged variable") {
        constexpr std::size_t num_fields = 2;
        storage mech_data{num_fields};
        REQUIRE(mech_data.num_floating_point_fields() == num_fields);
        WHEN("A row is added") {
            owning_handle mech_instance{mech_data};
            THEN("Values can be read and written") {
                constexpr auto field0_value = 42.0;
                constexpr auto field1_value = 7.0;
                mech_instance.set_fpfield(0, field0_value);
                mech_instance.set_fpfield(1, field1_value);
                REQUIRE_THROWS(mech_instance.set_fpfield(num_fields, -1));
                REQUIRE(mech_instance.fpfield(0) == field0_value);
                REQUIRE(mech_instance.fpfield(1) == field1_value);
                REQUIRE_THROWS(mech_instance.fpfield(num_fields));
            }
        }
    }
}
