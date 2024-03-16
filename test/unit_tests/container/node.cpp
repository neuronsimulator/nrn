#include "neuron/container/node.hpp"
#include "neuron/container/soa_container.hpp"
#include "section.h"

#include <catch2/catch.hpp>

#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <unordered_map>

static_assert(std::is_default_constructible_v<Node>);
static_assert(!std::is_copy_constructible_v<Node>);
static_assert(std::is_move_constructible_v<Node>);
static_assert(!std::is_copy_assignable_v<Node>);
static_assert(std::is_move_assignable_v<Node>);
static_assert(std::is_destructible_v<Node>);

using namespace neuron::container;
using namespace neuron::container::Node;

// We want to check that the tests pass for all of:
// - data_handle<T>
// - data_handle<T> -> T* -> data_handle<T>
// - data_handle<T> -> generic_data_handle -> data_handle<T>
enum struct Transform { None, ViaRawPointer, ViaGenericDataHandle };
template <typename T>
data_handle<T> transform(data_handle<T> handle, Transform type) {
    if (type == Transform::None) {
        return handle;
    } else if (type == Transform::ViaRawPointer) {
        return data_handle<T>{static_cast<T*>(handle)};
    } else {
        assert(type == Transform::ViaGenericDataHandle);
        return static_cast<data_handle<T>>(generic_data_handle{handle});
    }
}

constexpr static double magic_voltage_value = 42.;

template <typename T>
static std::string to_str(T const& x) {
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

TEST_CASE("data_handle<double>", "[Neuron][data_structures][data_handle]") {
    GIVEN("A null handle") {
        data_handle<double> handle{};
        handle = transform(handle,
                           GENERATE(Transform::None,
                                    Transform::ViaRawPointer,
                                    Transform::ViaGenericDataHandle));
        THEN("Check it is really null") {
            REQUIRE_FALSE(handle);
        }
        THEN("Check it compares equal to a different null pointer") {
            data_handle<double> const other_handle{};
            REQUIRE(handle == other_handle);
        }
        THEN("Check it prints the right value") {
            REQUIRE(to_str(handle) == "data_handle<double>{null}");
        }
        THEN("Check it doesn't claim to be modern") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check it decays to a null pointer") {
            auto* foo_ptr = static_cast<double*>(handle);
            REQUIRE(foo_ptr == nullptr);
        }
    }
    GIVEN("A handle wrapping a raw pointer (compatibility mode)") {
        std::vector<double> foo(10);
        std::iota(foo.begin(), foo.end(), magic_voltage_value);

        data_handle<double> handle{foo.data()};
        handle = transform(handle,
                           GENERATE(Transform::None,
                                    Transform::ViaRawPointer,
                                    Transform::ViaGenericDataHandle));
        THEN("Check it is not null") {
            REQUIRE(handle);
        }
        THEN("Check it does not compare equal to a null handle") {
            data_handle<double> null_handle{};
            REQUIRE(handle != null_handle);
        }
        THEN("Check it compares equal to a different handle wrapping the same raw pointer") {
            data_handle<double> other_handle{foo.data()};
            REQUIRE(handle == other_handle);
        }
        THEN("Check it yields the right value") {
            REQUIRE(*handle == magic_voltage_value);
        }
        THEN("Check it doesn't claim to be modern") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
            REQUIRE_FALSE(handle.refers_to<neuron::container::Node::field::Voltage>(
                neuron::model().node_data()));
        }
        THEN("Check it decays to the right raw pointer") {
            auto* foo_ptr = static_cast<double*>(handle);
            REQUIRE(foo_ptr == foo.data());
        }
        THEN("Check it prints the right value") {
            std::ostringstream expected;
            expected << "data_handle<double>{raw=" << foo.data() << '}';
            REQUIRE(to_str(handle) == expected.str());
        }
        THEN("Check that we can store/retrieve in/from unordered_map") {
            std::unordered_map<data_handle<double>, std::string> map;
            map[handle] = "unordered_map";
            REQUIRE(map[handle] == "unordered_map");
        }
        THEN("Check that next_array_element works") {
            auto next = handle.next_array_element(5);
            REQUIRE(next);
            REQUIRE(*next == magic_voltage_value + 5);
        }
    }
    GIVEN("A handle to a void pointer") {
        auto foo = std::make_shared<double>(magic_voltage_value);

        data_handle<void> handle{foo.get()};
        handle = transform(handle,
                           GENERATE(Transform::None,
                                    Transform::ViaRawPointer,
                                    Transform::ViaGenericDataHandle));
        THEN("Check it is not null") {
            REQUIRE(handle);
        }
        THEN("Check it doesn't claim to be modern") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check it decays to the right raw pointer") {
            auto* foo_ptr = static_cast<void*>(handle);
            REQUIRE(foo_ptr == foo.get());
        }
        THEN("Check it matches another data_handle<void> to same pointer") {
            const data_handle<void> other_handle{foo.get()};
            REQUIRE(handle == other_handle);
        }
        THEN("Check it prints the right value") {
            std::ostringstream expected;
            expected << "data_handle<void>{raw=" << foo.get() << '}';
            REQUIRE(to_str(handle) == expected.str());
        }
    }
    GIVEN("A handle referring to an entry in an SOA container") {
        REQUIRE(neuron::model().node_data().size() == 0);
        std::optional<::Node> node{std::in_place};
        node->v() = magic_voltage_value;
        auto handle = node->v_handle();
        const auto handle_id = handle.identifier();
        handle = transform(handle,
                           GENERATE(Transform::None,
                                    Transform::ViaRawPointer,
                                    Transform::ViaGenericDataHandle));
        THEN("Check it is not null") {
            REQUIRE(handle);
        }
        THEN("Check it actually refers_to voltage and not something else") {
            REQUIRE(handle.refers_to<neuron::container::Node::field::Voltage>(
                neuron::model().node_data()));
            REQUIRE_FALSE(handle.refers_to<neuron::container::Node::field::Area>(
                neuron::model().node_data()));
        }
        THEN("Check it does not compare equal to a null handle") {
            data_handle<double> null_handle{};
            REQUIRE(handle != null_handle);
        }
        THEN("Check it does not compare equal to a handle in legacy mode") {
            double foo{};
            data_handle<double> foo_handle{&foo};
            REQUIRE(handle != foo_handle);
        }
        THEN("Check it yields the right value") {
            REQUIRE(*handle == magic_voltage_value);
            const auto const_handle(handle);
            REQUIRE(*const_handle == magic_voltage_value);
        }
        THEN("Check it claims to be modern") {
            REQUIRE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check it prints the right value") {
            REQUIRE(to_str(handle) == "data_handle<double>{Node::field::Voltage row=0/1 val=42}");
        }
        THEN("Check that getting next_array_element throws, dimension is 1") {
            REQUIRE_THROWS(handle.next_array_element());
        }
        THEN("Check that we can store/retrieve in/from unordered_map") {
            std::unordered_map<data_handle<double>, std::string> map;
            map[handle] = "unordered_map_modern_dh";
            REQUIRE(map[handle] == "unordered_map_modern_dh");
        }
        THEN("Make sure we get the current logical row number") {
            REQUIRE(handle.current_row() == 0);
        }
        THEN("Check that deleting the (Node) object it refers to invalidates the handle") {
            node.reset();  // delete the underlying Node object
            REQUIRE_FALSE(handle);
            // REQUIRE(handle == data_handle<double>{});
            REQUIRE(handle.refers_to_a_modern_data_structure());
            REQUIRE(to_str(handle) == "data_handle<double>{Node::field::Voltage died/0}");
            REQUIRE(handle.refers_to<neuron::container::Node::field::Voltage>(
                neuron::model().node_data()));
            REQUIRE_THROWS(*handle);
            const auto const_handle(handle);
            REQUIRE_THROWS(*const_handle);
            REQUIRE(handle.identifier() == handle_id);
        }
        THEN(
            "Check that mutating the underlying container while holding a raw pointer has the "
            "expected effect") {
            auto* raw_ptr = static_cast<double*>(handle);
            REQUIRE(raw_ptr);
            REQUIRE(*raw_ptr == magic_voltage_value);
            node.reset();  // delete the underlying Node object, handle is now invalid
            REQUIRE_FALSE(handle);
            REQUIRE(raw_ptr);  // no magic here, we have a dangling pointer
            data_handle<double> new_handle{raw_ptr};
            REQUIRE(new_handle);  // handle refers to no-longer-valid memory, but we can't detect
                                  // that
            REQUIRE(handle != new_handle);
            REQUIRE_FALSE(new_handle.refers_to_a_modern_data_structure());
            // dereferencing raw_ptr is undefined behaviour
        }
    }
}

namespace neuron::test {
std::vector<double> get_node_voltages(std::vector<::Node> const& nodes) {
    std::vector<double> ret{};
    std::transform(nodes.begin(), nodes.end(), std::back_inserter(ret), [](auto const& node) {
        return node.v();
    });
    return ret;
}
std::tuple<std::vector<::Node>, std::vector<double>> get_nodes_and_reference_voltages(
    std::size_t num_nodes = 10) {
    std::vector<double> reference_voltages{};
    std::generate_n(std::back_inserter(reference_voltages), num_nodes, [i = 0]() mutable {
        auto x = i++;
        return x * x;
    });
    std::vector<::Node> nodes{};
    std::transform(reference_voltages.begin(),
                   reference_voltages.end(),
                   std::back_inserter(nodes),
                   [](auto v) {
                       ::Node node{};
                       node.v() = v;
                       return node;
                   });
    return {std::move(nodes), std::move(reference_voltages)};
}
}  // namespace neuron::test

TEST_CASE("SOA-backed Node structure", "[Neuron][data_structures][node]") {
    REQUIRE(neuron::model().node_data().size() == 0);
    GIVEN("A default-constructed node") {
        ::Node node{};
        THEN("Check its SOA-backed members have their default values") {
            REQUIRE(node.area() == field::Area{}.default_value());
            REQUIRE(node.v() == field::Voltage{}.default_value());
        }
        THEN("Check we can get a non-owning handle to it") {
            auto handle = node.non_owning_handle();
            AND_THEN("Check the handle yields the corect values") {
                REQUIRE(handle.area() == field::Area{}.default_value());
                REQUIRE(handle.v() == field::Voltage{}.default_value());
            }
        }
    }
    GIVEN("A series of nodes with increasing integer voltages") {
        using neuron::test::get_node_voltages;
        auto nodes_and_voltages = neuron::test::get_nodes_and_reference_voltages();
        auto& nodes = std::get<0>(nodes_and_voltages);
        auto& reference_voltages = std::get<1>(nodes_and_voltages);
        auto& node_data = neuron::model().node_data();
        // Flag this original order as "sorted" so that the tests that it is no
        // longer sorted after permutation are meaningful.
        {
            auto write_token = node_data.issue_frozen_token();
            node_data.mark_as_sorted(write_token);
        }
        auto const require_logical_match = [&]() {
            THEN("Check the logical voltages still match") {
                REQUIRE(get_node_voltages(nodes) == reference_voltages);
            }
        };
        auto const storage_match = [&]() {
            for (auto i = 0; i < nodes.size(); ++i) {
                if (node_data.get<field::Voltage>(i) != reference_voltages.at(i)) {
                    return false;
                }
            }
            return true;
        };
        auto const require_logical_and_storage_match = [&]() {
            THEN("Check the logical voltages still match") {
                REQUIRE(get_node_voltages(nodes) == reference_voltages);
                AND_THEN("Check the underlying storage also matches") {
                    REQUIRE(storage_match());
                }
            }
        };
        auto const require_logical_match_and_storage_different = [&]() {
            THEN("Check the logical voltages still match") {
                REQUIRE(get_node_voltages(nodes) == reference_voltages);
                AND_THEN("Check the underlying storage no longer matches") {
                    REQUIRE_FALSE(storage_match());
                }
            }
        };
        WHEN("Values are read back immediately") {
            require_logical_and_storage_match();
        }
        std::vector<std::size_t> perm_vector(nodes.size());
        std::iota(perm_vector.begin(), perm_vector.end(), 0);
        WHEN("The underlying storage is rotated") {
            auto rotated = perm_vector;
            std::rotate(rotated.begin(), std::next(rotated.begin()), rotated.end());
            auto const sorted_token = node_data.apply_reverse_permutation(std::move(rotated));
            require_logical_match_and_storage_different();
        }
        WHEN("A unit reverse permutation is applied to the underlying storage") {
            node_data.apply_reverse_permutation(std::move(perm_vector));
            require_logical_and_storage_match();
            // Should the data still be sorted here or not? Should
            // apply_permutation bother checking if the permutation did
            // anything?
        }
        WHEN("A random permutation is applied to the underlying storage") {
            std::mt19937 g{42};
            std::shuffle(perm_vector.begin(), perm_vector.end(), g);
            auto const sorted_token = node_data.apply_reverse_permutation(std::move(perm_vector));
            // the permutation is random, so we don't know if voltage_storage
            // will match reference_voltages or not
            require_logical_match();
        }
        auto const require_exception = [&](auto perm) {
            THEN("An exception is thrown") {
                REQUIRE_THROWS(node_data.apply_reverse_permutation(std::move(perm)));
                AND_THEN("The container is still flagged as sorted") {
                    REQUIRE(node_data.is_sorted());
                }
            }
        };
        WHEN("A too-short permutation is applied to the underlying storage") {
            std::vector<std::size_t> bad_perm(nodes.size() - 1);
            std::iota(bad_perm.begin(), bad_perm.end(), 0);
            require_exception(std::move(bad_perm));
        }
        WHEN("A permutation with a repeated entry is applied to the underlying storage") {
            std::vector<std::size_t> bad_perm(nodes.size());
            std::iota(bad_perm.begin(), bad_perm.end(), 0);
            bad_perm[0] = 1;  // repeated entry
            require_exception(std::move(bad_perm));
        }
        WHEN("A permutation with an invalid value is applied to the underlying storage") {
            std::vector<std::size_t> bad_perm(nodes.size());
            std::iota(bad_perm.begin(), bad_perm.end(), 0);
            bad_perm[0] = std::numeric_limits<std::size_t>::max();  // out of range
            require_exception(std::move(bad_perm));
        }
        WHEN("The last Node is removed") {
            nodes.pop_back();
            reference_voltages.pop_back();
            require_logical_and_storage_match();
        }
        WHEN("The first Node is removed") {
            nodes.erase(nodes.begin());
            reference_voltages.erase(reference_voltages.begin());
            require_logical_match_and_storage_different();
        }
        WHEN("The middle Node is removed") {
            auto const index_to_remove = nodes.size() / 2;
            nodes.erase(std::next(nodes.begin(), index_to_remove));
            reference_voltages.erase(std::next(reference_voltages.begin(), index_to_remove));
            require_logical_match_and_storage_different();
        }
        WHEN("The dense storage is sorted and marked read-only") {
            // A rough sketch of the concept here is that if we have a
            // SOA-backed quantity, like the node voltages, then referring
            // stably to those values requires something like
            // data_handle<double>. We might hold some complicated structure of
            // those "in the interpreter", let's say
            // std::list<data_handle<double>>, and want to flatten that into
            // something simpler for use in the translated MOD file code --
            // let's say std::vector<double*> -- while the data remain "sorted".
            {
                // Label the current order as sorted and acquire a token that
                // freezes it that way. The data should be sorted until the
                // token goes out of scope.
                auto frozen_token = node_data.issue_frozen_token();
                node_data.mark_as_sorted(frozen_token);
                REQUIRE(node_data.is_sorted());
                THEN("New nodes cannot be created") {
                    // Underlying node data is read-only, cannot allocate new Nodes.
                    REQUIRE_THROWS(::Node{});
                }
                // The token enforces that values cannot move in memory, but it
                // does not mean that they cannot be read from and written to
                THEN("Values in existing nodes can be modified") {
                    auto& node = nodes.front();
                    REQUIRE_NOTHROW(node.v());
                    REQUIRE_NOTHROW(node.v() += 42.0);
                }
                THEN("The sorted-ness flag cannot be modified") {
                    REQUIRE_THROWS(node_data.mark_as_unsorted());
                    AND_THEN("Attempts to do so fail") {
                        REQUIRE(node_data.is_sorted());
                    }
                }
                THEN(
                    "The storage *can* be permuted if the sorted token is transferred back to the "
                    "container") {
                    node_data.apply_reverse_permutation(std::move(perm_vector), frozen_token);
                }
                THEN("The storage cannot be permuted when a 2nd sorted token is used") {
                    // Checking one of the permuting operations should be enough
                    REQUIRE_THROWS(node_data.apply_reverse_permutation(std::move(perm_vector)));
                }
                // In read-only mode we cannot delete Nodes either, but because
                // we cannot throw from destructors it is not easy to test this
                // in this context. There is a separate test for this below
                // that is tagged with [tests_that_abort].
            }
            // sorted_token out of scope, underlying data no longer read-only
            THEN("After the token is discarded, new Nodes can be allocated") {
                REQUIRE_NOTHROW(::Node{});
            }
        }
    }
    REQUIRE(neuron::model().node_data().size() == 0);
}

TEST_CASE("Fast membrane current storage", "[Neuron][data_structures][node][fast_imem]") {
    REQUIRE(neuron::model().node_data().size() == 0);

    auto const set_fast_imem = [](bool new_value) {
        nrn_use_fast_imem = new_value;
        nrn_fast_imem_alloc();
    };
    auto const check_throws = [](auto& node) {
        THEN("fast_imem fields cannot be accessed") {
            CHECK_THROWS(node.sav_d());
            CHECK_THROWS(node.sav_rhs());
            CHECK_FALSE(node.sav_rhs_handle());
        }
    };
    auto const check_default = [](auto& node) {
        THEN("fast_imem fields have their default values") {
            CHECK(node.sav_d() == 0.0);
            CHECK(node.sav_rhs() == 0.0);
            CHECK(*node.sav_rhs_handle() == 0.0);
        }
    };
    GIVEN("fast_imem calculation is disabled") {
        set_fast_imem(false);
        WHEN("A node is default-constructed") {
            REQUIRE(neuron::model().node_data().size() == 0);
            ::Node node{};
            check_throws(node);
            auto handle = node.sav_rhs_handle();
            // The sav_rhs field is disabled, so the handle is a plain, completely null one.
            CHECK(to_str(handle) == "data_handle<double>{null}");
            CHECK(to_str(generic_data_handle{handle}) ==
                  "generic_data_handle{raw=nullptr type=double*}");
            AND_WHEN("fast_imem calculation is enabled with a Node active") {
                set_fast_imem(true);
                check_default(node);
                // The current implementation prefers simplicity to magic where possible, so handle
                // will still be null.
                CHECK_FALSE(handle);
            }
        }
    }
    GIVEN("fast_imem calculation is enabled") {
        set_fast_imem(true);
        WHEN("A node is default-constructed") {
            REQUIRE(neuron::model().node_data().size() == 0);
            ::Node node{};
            check_default(node);
            auto handle = node.sav_rhs_handle();
            *handle = 42;  // non-default value
            generic_data_handle generic{handle};
            CHECK(handle);
            CHECK(to_str(handle) ==
                  "data_handle<double>{Node::field::FastIMemSavRHS row=0/1 val=42}");
            CHECK(to_str(generic) ==
                  "generic_data_handle{Node::field::FastIMemSavRHS row=0/1 type=double*}");
            AND_WHEN("fast_imem calculation is disabled with a Node active") {
                REQUIRE(neuron::model().node_data().size() == 1);
                set_fast_imem(false);
                check_throws(node);
                // This handle used to be valid, but it is now invalid because the optional field it
                // refers to has been disabled.
                CHECK_FALSE(handle);
                CHECK(to_str(handle) == "data_handle<double>{cont=deleted row=0/unknown}");
                CHECK(to_str(generic) ==
                      "generic_data_handle{cont=deleted row=0/unknown type=double*}");
                AND_WHEN("fast_imem calculation is re-enabled") {
                    set_fast_imem(true);
                    // non-default value written above has been lost
                    check_default(node);
                    // Implementation choice was to minimise magic, so the handles are still dead
                    CHECK_FALSE(handle);
                    CHECK(to_str(handle) == "data_handle<double>{cont=deleted row=0/unknown}");
                    CHECK(to_str(generic) ==
                          "generic_data_handle{cont=deleted row=0/unknown type=double*}");
                }
            }
        }
        WHEN("A series of Nodes are created with non-trivial fast_imem values") {
            constexpr auto num_nodes = 10;
            std::vector<::Node> nodes(num_nodes);
            std::vector<std::size_t> perm_vector(num_nodes);
            for (auto i = 0; i < num_nodes; ++i) {
                perm_vector[i] = i;
                nodes[i].sav_d() = i * i;
                nodes[i].sav_rhs() = i * i * i;
            }
            AND_WHEN("A random permutation is applied") {
                std::mt19937 g{42};
                std::shuffle(perm_vector.begin(), perm_vector.end(), g);
                auto& node_data = neuron::model().node_data();
                node_data.apply_reverse_permutation(std::move(perm_vector));
                THEN("The logical values should still match") {
                    for (auto i = 0; i < num_nodes; ++i) {
                        REQUIRE(nodes[i].sav_d() == i * i);
                        REQUIRE(nodes[i].sav_rhs() == i * i * i);
                    }
                }
            }
        }
    }
}

// Tests that cover code paths reaching std::terminate. "[.]" means they will not run by default,
// [tests_that_abort] means we have a tag to run them with.
TEST_CASE("Deleting a row from a frozen SoA container causes a fatal error",
          "[.][tests_that_abort]") {
    auto& node_data = neuron::model().node_data();  // SoA data store
    std::optional<::Node> node{std::in_place};      // take ownership of a row in node_data
    REQUIRE(node_data.size() == 1);                 // quick sanity check
    auto const frozen_token = node_data.issue_frozen_token();  // mark node_data frozen
    node.reset();  // Node destructor will trigger a call to std::terminate.
}
