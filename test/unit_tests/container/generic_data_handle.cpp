#include "neuron/container/generic_data_handle.hpp"
#include "neuron/container/node.hpp"
#include "neuron/model_data.hpp"

#include <catch2/catch.hpp>

#include <optional>
#include <sstream>

using namespace neuron::container;

template <typename T>
static std::string to_str(T const& x) {
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

TEST_CASE("generic_data_handle", "[Neuron][data_structures][generic_data_handle]") {
    // Checks that apply to both typeless-null and double-typed null handles.
    auto const check_double_or_typeless_null_handle = [](auto& handle) {
        THEN("Check it does not claim to refer to a modern container") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check it can be converted to a null data_handle<double>") {
            auto const typed_null = static_cast<data_handle<double>>(handle);
            REQUIRE_FALSE(typed_null);
            REQUIRE(typed_null == data_handle<double>{});
        }
        THEN("Check it can be made typeless-null by assigning nullptr") {
            REQUIRE_NOTHROW(handle = nullptr);
            AND_THEN("Check it has the expected string representation") {
                REQUIRE(to_str(handle) == "generic_data_handle{raw=nullptr type=typeless_null}");
            }
        }
        THEN("Check it can be made double*-null by assigning a null double*") {
            REQUIRE_NOTHROW(handle = static_cast<double*>(nullptr));
            AND_THEN("Check it has the expected string representation") {
                REQUIRE(to_str(handle) == "generic_data_handle{raw=nullptr type=double*}");
            }
        }
        THEN("Check it can be assigned a literal int value") {
            REQUIRE_NOTHROW(handle = 42);
            AND_THEN("Check it has the expected string representation") {
                REQUIRE(to_str(handle) == "generic_data_handle{raw=0x2a type=int}");
            }
        }
        THEN("Check it can be assigned a literal double value") {
            REQUIRE_NOTHROW(handle = 42.0);
            AND_THEN("Check it has the expected string representation") {
                // this is 42.0 interpreted as a 64-bit integer...
                REQUIRE(to_str(handle) ==
                        "generic_data_handle{raw=0x4045000000000000 type=double}");
            }
        }
    };
    auto const check_typeless_null = [&](auto& handle) {
        check_double_or_typeless_null_handle(handle);
        THEN("Check it has the expected string representation") {
            REQUIRE(to_str(handle) == "generic_data_handle{raw=nullptr type=typeless_null}");
        }
        THEN("Check it can be converted to data_handle<int>") {
            REQUIRE_NOTHROW(static_cast<data_handle<int>>(handle));
        }
    };
    GIVEN("A typeless null generic handle") {
        generic_data_handle handle{};
        check_typeless_null(handle);
    }
    GIVEN("A typeless null generic handle constructed from nullptr") {
        generic_data_handle handle{nullptr};
        check_typeless_null(handle);
    }
    GIVEN("A double*-typed null generic handle") {
        // construct a double*-typed null handle
        generic_data_handle null_handle{data_handle<double>{}};
        check_double_or_typeless_null_handle(null_handle);
        THEN("Check it cannot be converted to data_handle<int>") {
            REQUIRE_THROWS(static_cast<data_handle<int>>(null_handle));
        }
        THEN("Check it has the expected string representation") {
            REQUIRE(to_str(null_handle) == "generic_data_handle{raw=nullptr type=double*}");
        }
    }
    GIVEN("A handle wrapping a raw pointer (compatibility mode)") {
        auto* const raw_ptr = reinterpret_cast<double*>(0xdeadbeefdeadbeef);
        data_handle<double> typed_handle{raw_ptr};
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
            REQUIRE(to_str(handle) == "generic_data_handle{raw=0xdeadbeefdeadbeef type=double*}");
        }
        THEN("Check it does not claim to refer to a modern container") {
            REQUIRE_FALSE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check we can't get another type out of it") {
            REQUIRE_THROWS(handle.get<int>());
            REQUIRE_THROWS(handle.literal_value<int>());
        }
    }
    GIVEN("A generic_handle referring to an entry in an SOA container") {
        auto& node_data = neuron::model().node_data();
        REQUIRE(node_data.size() == 0);
        std::optional<Node::owning_handle> node{node_data};
        auto typed_handle = node->v_handle();
        THEN("Match typed_handle as true") {
            REQUIRE(typed_handle);
        }
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
            REQUIRE(to_str(handle) ==
                    "generic_data_handle{Node::field::Voltage row=0/1 type=double*}");
        }
        THEN("Check that it knows it refers to a modern data structure") {
            REQUIRE(handle.refers_to_a_modern_data_structure());
        }
        THEN("Check that we can't get another type out of it") {
            REQUIRE_THROWS(handle.get<int>());
            REQUIRE_THROWS(handle.get<double>());
        }
        THEN("Check we cannot obtain a literal value") {
            REQUIRE_THROWS(handle.literal_value<int>());
            REQUIRE_THROWS(handle.literal_value<double>());
        }
        WHEN("The row of the modern data structure is deleted") {
            node.reset();
            THEN("Check it still reports referring to a modern data structure") {
                REQUIRE(handle.refers_to_a_modern_data_structure());
            }
            THEN("Check it has the expected string representation") {
                REQUIRE(to_str(handle) ==
                        "generic_data_handle{Node::field::Voltage died/0 type=double*}");
            }
        }
    }
}
