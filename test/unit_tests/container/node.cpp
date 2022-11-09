#include "neuron/container/node.hpp"
// Need to include this explicitly because we call methods that need external
// dependency headers
#include "neuron/container/soa_container_impl.hpp"
#include "section.h"
#include "../model_test_utils.hpp"

#include <catch2/catch.hpp>

#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

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
        return {static_cast<T*>(handle)};
    } else {
        assert(type == Transform::ViaGenericDataHandle);
        return static_cast<data_handle<T>>(generic_data_handle{handle});
    }
}

constexpr static double magic_voltage_value = 42.;

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
    }
    GIVEN("A handle wrapping a raw pointer (compatibility mode)") {
        double foo{magic_voltage_value};
        data_handle<double> handle{&foo};
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
            data_handle<double> other_handle{&foo};
            REQUIRE(handle == other_handle);
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
        REQUIRE(neuron::model().node_data().size() == 0);
        std::optional<::Node> node{std::in_place};
        node->set_v(magic_voltage_value);
        auto handle = node->v_handle();
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
        THEN("Check it does not compare equal to a handle in legacy mode") {
            double foo{};
            data_handle<double> foo_handle{&foo};
            REQUIRE(handle != foo_handle);
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
            REQUIRE(actual.str() == "data_handle<double>{Node::field::Voltage row=0/1}");
        }
        THEN("Check that deleting the (Node) object it refers to invalidates the handle") {
            node.reset();  // delete the underlying Node object
            REQUIRE_FALSE(handle);
            // REQUIRE(handle == data_handle<double>{});
            REQUIRE(handle.refers_to_a_modern_data_structure());
            std::ostringstream actual;
            actual << handle;
            REQUIRE(actual.str() == "data_handle<double>{Node::field::Voltage died/0}");
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

TEST_CASE("generic_data_handle", "[Neuron][data_structures][generic_data_handle]") {
    GIVEN("A null generic handle") {
        // no default constructor, have to go via a data_handle<T>
        generic_data_handle null_handle{data_handle<double>{}};
        THEN("Check it remembered the double type") {
            REQUIRE(null_handle.type_name() == "double*");
        }
        THEN("Check it can be converted back to data_handle<double>") {
            auto const round_trip = static_cast<data_handle<double>>(null_handle);
            REQUIRE_FALSE(round_trip);
            REQUIRE(round_trip == data_handle<double>{});
        }
        THEN("Check it cannot be converted to data_handle<int>") {
            REQUIRE_THROWS(static_cast<data_handle<int>>(null_handle));
        }
        THEN("Check it has the expected string representation") {
            std::ostringstream actual;
            actual << null_handle;
            REQUIRE(actual.str() == "generic_data_handle{raw=nullptr, type=double*}");
        }
        THEN("Check it does not claim to refer to a modern container") {
            REQUIRE_FALSE(null_handle.refers_to_a_modern_data_structure());
        }
    }
    GIVEN("A handle wrapping a raw pointer (compatibility mode)") {
        double foo{};
        data_handle<double> typed_handle{&foo};
        generic_data_handle handle{typed_handle};
        THEN("Check it remembered the double type") {
            REQUIRE(handle.type_name() == "double*");
        }
        THEN("Check it can be converted back to data_handle<double>") {
            REQUIRE_NOTHROW(static_cast<data_handle<double>>(handle));
        }
        THEN("Check it cannot be converted to data_handle<int>") {
            REQUIRE_THROWS(static_cast<data_handle<int>>(handle));
        }
        THEN("Check it has the expected string representation") {
            std::ostringstream actual, expected;
            actual << handle;
            expected << "generic_data_handle{raw=" << &foo << ", type=double*}";
            REQUIRE(actual.str() == expected.str());
        }
        THEN("Check it does not claim to refer to a modern container") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
        }
    }
    GIVEN("A generic_handle referring to an entry in an SOA container") {
        REQUIRE(neuron::model().node_data().size() == 0);
        std::optional<::Node> node{std::in_place};
        node->set_v(magic_voltage_value);
        auto typed_handle = node->v_handle();
        generic_data_handle handle{typed_handle};
        THEN("Check it remembered the double type") {
            REQUIRE(handle.type_name() == "double*");
        }
        THEN("Check it can be converted back to data_handle<double>") {
            REQUIRE_NOTHROW(static_cast<data_handle<double>>(handle));
        }
        THEN("Check it cannot be converted to data_handle<int>") {
            REQUIRE_THROWS(static_cast<data_handle<int>>(handle));
        }
        THEN("Check it has the expected string representation") {
            std::ostringstream actual;
            actual << handle;
            REQUIRE(actual.str() ==
                    "generic_data_handle{Node::field::Voltage row=0/1, type=double*}");
        }
        THEN("Check that it knows it refers to a modern data structure") {
            REQUIRE(handle.refers_to_a_modern_data_structure());
        }
        WHEN("The row of the modern data structure is deleted") {
            node.reset();
            THEN("Check it still reports referring to a modern data structure") {
                REQUIRE(handle.refers_to_a_modern_data_structure());
            }
        }
    }
}

// Declared in model_test_utils.hpp so they can be reused in other tests too
namespace neuron::test {
std::vector<double> get_node_voltages(std::vector<::Node> const& nodes) {
    std::vector<double> ret{};
    std::transform(nodes.begin(), nodes.end(), std::back_inserter(ret), [](auto const& node) {
        return node.v();
    });
    return ret;
}
std::tuple<std::vector<::Node>, std::vector<double>> get_nodes_and_reference_voltages(
    std::size_t num_nodes) {
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
                       node.set_v(v);
                       return node;
                   });
    return {std::move(nodes), std::move(reference_voltages)};
}
}  // namespace neuron::test

TEST_CASE("SOA-backed Node structure", "[Neuron][data_structures][node]") {
    GIVEN("A default-constructed node") {
        ::Node node{};
        THEN("Check its SOA-backed members have their default values") {
            REQUIRE(node.area() == field::Area{}.default_value());
            REQUIRE(node.v() == field::Voltage{}.default_value());
        }
    }
    GIVEN("A series of nodes with increasing integer voltages") {
        using neuron::test::get_node_voltages;
        auto nodes_and_voltages = neuron::test::get_nodes_and_reference_voltages();
        auto& nodes = std::get<0>(nodes_and_voltages);
        auto& reference_voltages = std::get<1>(nodes_and_voltages);
        auto& node_data = neuron::model().node_data();
        auto const& voltage_storage = std::as_const(node_data).get<field::Voltage>();
        // Flag this original order as "sorted" so that the tests that it is no
        // longer sorted after permutation are meaningful
        { auto token = node_data.get_sorted_token(); }
        REQUIRE(nodes.size() == voltage_storage.size());
        auto const require_logical_match = [&]() {
            THEN("Check the logical voltages still match") {
                REQUIRE(get_node_voltages(nodes) == reference_voltages);
            }
        };
        auto const require_logical_and_storage_match = [&]() {
            THEN("Check the logical voltages still match") {
                REQUIRE(get_node_voltages(nodes) == reference_voltages);
                AND_THEN("Check the underlying storage also matches") {
                    REQUIRE(voltage_storage == reference_voltages);
                }
            }
        };
        auto const require_logical_match_and_storage_different = [&]() {
            THEN("Check the logical voltages still match") {
                REQUIRE(get_node_voltages(nodes) == reference_voltages);
                AND_THEN("Check the underlying storage no longer matches") {
                    REQUIRE_FALSE(voltage_storage == reference_voltages);
                }
                AND_THEN("Check the container is not flagged as sorted") {
                    REQUIRE_FALSE(node_data.is_sorted());
                }
            }
        };
        WHEN("Values are read back immediately") {
            require_logical_and_storage_match();
        }
// As of nvc++/22.7 the range::v3 code behind rotate gives compilation errors,
// which are apparently fixed in 22.9+:
// https://forums.developer.nvidia.com/t/compilation-failures-with-range-v3-and-nvc-22-7/228444
#if !defined(__NVCOMPILER) || (__NVCOMPILER_MAJOR__ >= 22 && __NVCOMPILER_MINOR__ >= 9)
        WHEN("The underlying storage is rotated") {
            node_data.rotate(1);
            require_logical_match_and_storage_different();
        }
#endif
        WHEN("The underlying storage is reversed") {
            node_data.reverse();
            require_logical_match_and_storage_different();
        }
        std::vector<std::size_t> perm_vector(nodes.size());
        std::iota(perm_vector.begin(), perm_vector.end(), 0);
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
            node_data.apply_reverse_permutation(std::move(perm_vector));
            // the permutation is random, so we don't know if voltage_storage
            // will match reference_voltages or not
            require_logical_match();
            THEN("Check the storage is no longer flagged as sorted") {
                REQUIRE_FALSE(node_data.is_sorted());
            }
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
                auto const sorted_token = node_data.get_sorted_token();
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
                    REQUIRE_NOTHROW(node.set_v(node.v() + 42.0));
                }
                THEN("A non-const reference to the underlying storage cannot be obtained") {
                    REQUIRE_THROWS(node_data.get<field::Voltage>());
                }
                THEN("A const reference to the underlying storage can be obtained") {
                    REQUIRE_NOTHROW(std::as_const(node_data).get<field::Voltage>());
                }
                THEN("The sorted-ness flag cannot be modified") {
                    REQUIRE_THROWS(node_data.mark_as_unsorted());
                    AND_THEN("Attempts to do so fail") {
                        REQUIRE(node_data.is_sorted());
                    }
                }
                THEN("The storage cannot be permuted") {
                    // Checking one of the permuting operations should be enough
                    REQUIRE_THROWS(node_data.reverse());
                }
                // In read-only mode we cannot delete Nodes either, but because
                // we cannot throw from destructors it is not easy to test this
                // in this context. nodes.pop_back() would call std::terminate.
            }
            // sorted_token out of scope, underlying data no longer read-only
            THEN("After the token is discarded, new Nodes can be allocated") {
                REQUIRE_NOTHROW(::Node{});
            }
        }
    }
}
