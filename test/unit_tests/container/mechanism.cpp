#include "neuron/container/mechanism.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/model_data.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

using namespace neuron::container::Mechanism;

TEST_CASE("SOA-backed Mechanism data structure", "[Neuron][data_structures][mechanism]") {
    GIVEN("A mechanism with two copies of the same tagged variable") {
        // foo is scalar, bar is an array of dimension bar_values.size()
        constexpr std::array bar_values{7.0, 0.7, 0.9};
        std::vector<Variable> field_info{{"foo", 1}, {"bar", bar_values.size()}};
        auto const num_fields = field_info.size();
        auto const foo_index = 0;
        auto const bar_index = 1;
        // Have to register the storage with neuron::model(), otherwise pretty-printing of data
        // handles won't work
        auto const mech_type = 0;
        neuron::model().delete_mechanism(mech_type);
        auto& mech_data = neuron::model().add_mechanism(mech_type, "test_mechanism", field_info);
        REQUIRE(mech_data.get_tag<field::FloatingPoint>().num_variables() == num_fields);
        WHEN("A row is added") {
            owning_handle mech_instance{mech_data};
            THEN("Values can be read and written") {
                constexpr auto field0_value = 42.0;
                mech_instance.fpfield(foo_index) = field0_value;
                for (auto i = 0; i < bar_values.size(); ++i) {
                    mech_instance.fpfield(bar_index, i) = bar_values[i];
                }
                REQUIRE_THROWS(mech_instance.fpfield(num_fields));
                REQUIRE_THROWS(mech_instance.fpfield(bar_index, bar_values.size()));
                REQUIRE(mech_instance.fpfield(foo_index) == field0_value);
                for (auto i = 0; i < bar_values.size(); ++i) {
                    REQUIRE(mech_instance.fpfield(bar_index, i) == bar_values[i]);
                }
                REQUIRE_THROWS(mech_instance.fpfield(num_fields));
                AND_THEN("Data handles give useful information when printed") {
                    auto const require_str = [&mech_instance](std::string_view ref, auto... args) {
                        auto const dh = mech_instance.fpfield_handle(args...);
                        std::ostringstream oss;
                        oss << dh;
                        REQUIRE(oss.str() == ref);
                    };
                    // Slightly worried that the val=xxx formatting isn't portable, in which case we
                    // could generate the reference strings using the values above.
                    require_str("data_handle<double>{cont=test_mechanism foo row=0/1 val=42}",
                                foo_index);
                    require_str("data_handle<double>{cont=test_mechanism bar[0/3] row=0/1 val=7}",
                                bar_index,
                                0);
                    require_str("data_handle<double>{cont=test_mechanism bar[1/3] row=0/1 val=0.7}",
                                bar_index,
                                1);
                    require_str("data_handle<double>{cont=test_mechanism bar[2/3] row=0/1 val=0.9}",
                                bar_index,
                                2);
                }
            }
        }
        WHEN("Many rows are added") {
            std::vector<double> reference_field0, reference_field1_0, reference_field1_1;
            constexpr auto num_instances = 10;
            std::generate_n(std::back_inserter(reference_field0), num_instances, [i = 0]() mutable {
                auto const x = i++;
                return x * x;
            });
            std::generate_n(std::back_inserter(reference_field1_0),
                            num_instances,
                            [i = 0]() mutable {
                                auto const x = i++;
                                return x * x * x;
                            });
            std::generate_n(std::back_inserter(reference_field1_1),
                            num_instances,
                            [i = 0]() mutable {
                                auto const x = i++;
                                return x * x * x * x;
                            });
            std::vector<owning_handle> mech_instances{};
            for (auto i = 0; i < num_instances; ++i) {
                auto& mech = mech_instances.emplace_back(mech_data);
                mech.fpfield(0) = reference_field0[i];
                mech.fpfield(1, 0) = reference_field1_0[i];
                mech.fpfield(1, 1) = reference_field1_1[i];
            }
            REQUIRE(mech_data.empty() == mech_instances.empty());
            REQUIRE(mech_data.size() == mech_instances.size());
            enum struct StorageCheck { Skip, Match, NotMatch };
            const auto check_field_values = [&](StorageCheck storage_should_match) {
                THEN("The correct values can be read back") {
                    std::vector<double> current_field0, current_field1_0, current_field1_1;
                    std::transform(mech_instances.begin(),
                                   mech_instances.end(),
                                   std::back_inserter(current_field0),
                                   [](auto const& mech) { return mech.fpfield(0); });
                    std::transform(mech_instances.begin(),
                                   mech_instances.end(),
                                   std::back_inserter(current_field1_0),
                                   [](auto const& mech) { return mech.fpfield(1, 0); });
                    std::transform(mech_instances.begin(),
                                   mech_instances.end(),
                                   std::back_inserter(current_field1_1),
                                   [](auto const& mech) { return mech.fpfield(1, 1); });
                    REQUIRE(current_field0 == reference_field0);
                    REQUIRE(current_field1_0 == reference_field1_0);
                    REQUIRE(current_field1_1 == reference_field1_1);
                    auto const field_matches =
                        [&](auto const& reference, int index, int array_index = 0) {
                            for (auto i = 0; i < mech_data.size(); ++i) {
                                if (mech_data.fpfield(i, index, array_index) != reference[i]) {
                                    return false;
                                }
                            }
                            return true;
                        };
                    if (storage_should_match == StorageCheck::Match) {
                        AND_THEN("The underlying storage matches the reference values") {
                            REQUIRE(field_matches(reference_field0, 0));
                            REQUIRE(field_matches(reference_field1_0, 1, 0));
                            REQUIRE(field_matches(reference_field1_1, 1, 1));
                        }
                    } else if (storage_should_match == StorageCheck::NotMatch) {
                        AND_THEN("The underlying storage no longer matches the reference values") {
                            REQUIRE_FALSE(field_matches(reference_field0, 0));
                            REQUIRE_FALSE(field_matches(reference_field1_0, 1, 0));
                            REQUIRE_FALSE(field_matches(reference_field1_1, 1, 1));
                        }
                    }
                }
            };
            check_field_values(StorageCheck::Match);
            AND_WHEN("The underlying storage is permuted") {
                std::vector<std::size_t> perm_vector(mech_instances.size());
                std::iota(perm_vector.begin(), perm_vector.end(), 0);
                std::mt19937 g{42};
                std::shuffle(perm_vector.begin(), perm_vector.end(), g);
                mech_data.apply_reverse_permutation(std::move(perm_vector));
                check_field_values(StorageCheck::Skip);
            }
            auto const apply_to_all = [&](auto callable) {
                auto const impl = [](auto callable, auto&... vecs) { (callable(vecs), ...); };
                impl(std::move(callable),
                     mech_instances,
                     reference_field0,
                     reference_field1_0,
                     reference_field1_1);
            };
            AND_WHEN("An element is deleted") {
                apply_to_all([](auto& vec) { vec.erase(vec.begin()); });
                check_field_values(StorageCheck::Skip);
            }
        }
    }
}
