#include "neuron/container/mechanism.hpp"
#include "neuron/container/soa_container_impl.hpp"

#include <catch2/catch.hpp>

#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

using namespace neuron::container::Mechanism;

TEST_CASE("SOA-backed Mechanism data structure", "[Neuron][data_structures][mechanism]") {
    GIVEN("A mechanism with two copies of the same tagged variable") {
        constexpr std::size_t num_fields = 2;
        storage mech_data{0 /* type */, "test_mechanism", num_fields};
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
        WHEN("Many rows are added") {
            std::vector<double> reference_field0, reference_field1;
            constexpr auto num_instances = 10;
            std::generate_n(std::back_inserter(reference_field0), num_instances, [i = 0]() mutable {
                auto const x = i++;
                return x * x;
            });
            std::generate_n(std::back_inserter(reference_field1), num_instances, [i = 0]() mutable {
                auto const x = i++;
                return x * x * x;
            });
            std::vector<owning_handle> mech_instances{};
            std::transform(reference_field0.begin(),
                           reference_field0.end(),
                           reference_field1.begin(),
                           std::back_inserter(mech_instances),
                           [&mech_data](auto field0, auto field1) {
                               owning_handle mech{mech_data};
                               mech.set_fpfield(0, field0);
                               mech.set_fpfield(1, field1);
                               return mech;
                           });
            const auto check_field_values = [&](bool storage_should_match) {
                THEN("The correct values can be read back") {
                    std::vector<double> current_field0, current_field1;
                    std::transform(mech_instances.begin(),
                                   mech_instances.end(),
                                   std::back_inserter(current_field0),
                                   [](auto const& mech) { return mech.fpfield(0); });
                    std::transform(mech_instances.begin(),
                                   mech_instances.end(),
                                   std::back_inserter(current_field1),
                                   [](auto const& mech) { return mech.fpfield(1); });
                    REQUIRE(current_field0 == reference_field0);
                    REQUIRE(current_field1 == reference_field1);
                    auto const& storage0 =
                        mech_data.get_field_instance<field::PerInstanceFloatingPointField>(0);
                    auto const& storage1 =
                        mech_data.get_field_instance<field::PerInstanceFloatingPointField>(1);
                    if (storage_should_match) {
                        AND_THEN("The underlying storage matches the reference values") {
                            REQUIRE(reference_field0 == storage0);
                            REQUIRE(reference_field1 == storage1);
                        }
                    } else {
                        AND_THEN("The underlying storage no longer matches the reference values") {
                            REQUIRE(reference_field0 != storage0);
                            REQUIRE(reference_field1 != storage1);
                        }
                    }
                }
            };
            check_field_values(true);
            AND_WHEN("The underlying storage is reversed") {
                mech_data.reverse();
                check_field_values(false);
            }
            AND_WHEN("The underlying storage is rotated") {
                mech_data.rotate(num_instances / 2);
                check_field_values(false);
            }
        }
    }
}
