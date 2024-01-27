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
        // the top-level mechanism data structure can be pretty-printed
        THEN("The mechanism data structure can be pretty-printed") {
            std::ostringstream oss;
            oss << mech_data;
            REQUIRE(oss.str() == "test_mechanism::storage{type=0, 2 fields}");
        }
        REQUIRE(mech_data.get_tag<field::FloatingPoint>().num_variables() == num_fields);
        WHEN("A row is added") {
            owning_handle mech_instance{mech_data};
            THEN("We cannot delete the mechanism type") {
                REQUIRE_THROWS(neuron::model().delete_mechanism(mech_type));
            }
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

                REQUIRE(mech_instance.fpfield_dimension(foo_index) == 1);
                REQUIRE(mech_instance.fpfield_dimension(bar_index) == bar_values.size());
                REQUIRE_THROWS(mech_instance.fpfield_dimension(num_fields));

                auto bar_handle = mech_instance.fpfield_handle(bar_index);
                REQUIRE(*bar_handle.next_array_element(2) == 0.9);
                REQUIRE_THROWS(*bar_handle.next_array_element(bar_values.size()));


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
                    // Also cover generic_data_handle printing
                    auto const gdh = neuron::container::generic_data_handle{
                        mech_instance.fpfield_handle(foo_index)};
                    std::ostringstream oss;
                    oss << gdh;
                    REQUIRE(oss.str() ==
                            "generic_data_handle{cont=test_mechanism foo row=0/1 type=double*}");
                }
                std::ostringstream actual;
                actual << mech_instance;
                REQUIRE(actual.str() ==
                        "test_mechanism{row=0/1 fpfield[0]{ 42 } fpfield[1]{ 7 0.7 0.9 }}");
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
        WHEN("We want to manipulate other mechanism data") {
            THEN("We cannot readd the same type") {
                REQUIRE_THROWS(
                    neuron::model().add_mechanism(mech_type, "test_mechanism_2", field_info));
            }
            THEN("We cannot access a non-existing mechanism") {
                auto const non_existing_mech_type = 313;
                REQUIRE_THROWS(neuron::model().mechanism_data(non_existing_mech_type));
            }
            THEN("We cannot get data for a null mechanism") {
                auto const null_mech_type = 345;
                neuron::model().add_mechanism(null_mech_type, "null_mechanism");
                neuron::model().delete_mechanism(null_mech_type);
                REQUIRE_THROWS(neuron::model().mechanism_data(null_mech_type));
            }
        }
    }
    GIVEN("A mechanism type that gets deleted") {
        std::vector<Variable> field_info{{"foo", 1}};  // one scalar variable
        constexpr auto foo_index = 0;
        constexpr auto foo_value = 29.0;
        // Register the storage with neuron::model() because we want to cover codepaths in the
        // pretty-printing
        auto const mech_type = 0;
        neuron::model().delete_mechanism(mech_type);
        auto& mech_data = neuron::model().add_mechanism(mech_type, "test_mechanism", field_info);
        REQUIRE(mech_data.get_tag<field::FloatingPoint>().num_variables() == 1);
        WHEN("A row is added and we take a handle to its value") {
            std::optional<owning_handle> instance{std::in_place, mech_data};
            instance->fpfield(foo_index) = foo_value;
            auto foo_handle = instance->fpfield_handle(foo_index);
            auto generic_foo = neuron::container::generic_data_handle{foo_handle};
            THEN("The handle yields the expected value") {
                REQUIRE(*foo_handle == foo_value);
            }
            AND_WHEN("The row is deleted again") {
                instance.reset();
                THEN("We can still print the handle") {
                    std::ostringstream oss;
                    oss << foo_handle;
                    REQUIRE(oss.str() == "data_handle<double>{cont=test_mechanism foo died/0}");
                }
                AND_WHEN("The mechanism type is also deleted") {
                    neuron::model().delete_mechanism(mech_type);
                    THEN("We can still print the handle, following an unusual codepath") {
                        {
                            std::ostringstream oss;
                            oss << foo_handle;
                            REQUIRE(oss.str() == "data_handle<double>{cont=unknown died/unknown}");
                        }
                        {
                            std::ostringstream oss;
                            oss << generic_foo;
                            REQUIRE(oss.str() ==
                                    "generic_data_handle{cont=unknown died/unknown type=double*}");
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Model::is_valid_mechanism", "[Neuron][data_structures]") {
    // Since `neuron::model` is a global we we've no clue what state it's in.
    // Hence we delete what we want to use and remember that we have to assume
    // there might be other mechanisms present that we shall ignore.

    auto& model = neuron::model();
    std::vector<Variable> field_info{{"foo", 1}};  // just a dummy value.

    model.delete_mechanism(0);
    model.add_mechanism(0, "zero", field_info);

    model.delete_mechanism(1);
    model.add_mechanism(1, "one", field_info);

    model.delete_mechanism(2);
    model.add_mechanism(2, "two", field_info);

    model.delete_mechanism(1);

    CHECK(model.is_valid_mechanism(0));
    CHECK(!model.is_valid_mechanism(1));
    CHECK(model.is_valid_mechanism(2));
}
