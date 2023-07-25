#include "neuron/container/soa_container.hpp"
#include "neuron/container/view_utils.hpp"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"

#include <catch2/catch.hpp>

#include <thread>

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

/** @brief Tag type for just one double per row.
 */
struct B {
    using type = double;
};

/** @brief Tag type with multiple fields of differing array_dimension.
 */
struct C {
    using type = double;

    size_t num_variables() const {
        return 3;
    }

    int array_dimension(int field_index) const {
        return field_index + 1;
    }
};

/** @brief Tag type for an optional field, intended to be Off.
 */
struct DOff {
    static constexpr bool optional = true;
    using type = double;
};

/** @brief Tag type for an optional field, intended to be On.
 */
struct DOn {
    static constexpr bool optional = true;
    using type = double;
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
struct storage: soa<storage, field::A, field::B, field::C, field::DOff, field::DOn> {};
using owning_handle = handle_interface<owning_identifier<storage>>;
}  // namespace

TEST_CASE("Tag type with array_dimension and without num_variables", "[Neuron][data_structures]") {
    GIVEN("A standalone soa container") {
        storage data;  // Debian's GCC 10.2 doesn't like a {} before the ;
        THEN("Can we create a handle to a row in it") {
            REQUIRE_NOTHROW([&data]() { owning_handle instance{data}; }());
        }
    }
}

TEST_CASE("Multi-threaded calls to nrn_ensure_model_data_are_sorted()",
          "[Neuron][data_structures]") {
    GIVEN("An initialised model (albeit empty for the moment)") {
        REQUIRE(hoc_oc("create s\nfinitialize(-65)\n") == 0);
        REQUIRE(neuron::model().node_data().size() == 3);
        THEN("Call nrn_ensure_model_data_are_sorted multiple times concurrently") {
            // Calling nrn_ensure_model_data_are_sorted() in multiple threads should
            // succeed in all of them, but the underlying sort operations should be
            // serialised.
            constexpr auto num_threads = 7;
            std::vector<std::thread> threads;
            // Accumulate the tokens returned by nrn_ensure_model_data_are_sorted.
            std::mutex token_mutex{};
            std::vector<neuron::model_sorted_token> tokens;
            // Make sure the data are not already sorted, otherwise we won't follow the complicated
            // codepath
            neuron::model().node_data().mark_as_unsorted();
            for (auto i = 0; i < num_threads; ++i) {
                threads.emplace_back([&tokens, &token_mutex]() {
                    auto token = nrn_ensure_model_data_are_sorted();
                    std::unique_lock _{token_mutex};
                    tokens.push_back(token);
                });
            }
            // Wait for all the threads to end
            for (auto& thread: threads) {
                thread.join();
            }
            REQUIRE(tokens.size() == num_threads);
        }
        REQUIRE(hoc_oc("delete_section()") == 0);
        REQUIRE(neuron::model().node_data().size() == 0);
    }
}

TEST_CASE("soa::get_array_dims", "[Neuron][data_structures]") {
    storage data;

    data.set_field_status<field::DOn>(true);
    data.set_field_status<field::DOff>(false);

    auto c = field::C{};

    for (size_t field_index = 0; field_index < c.num_variables(); ++field_index) {
        CHECK(data.template get_array_dims<field::C>()[field_index] ==
              c.array_dimension(field_index));
        CHECK(data.template get_array_dims<field::C>(field_index) ==
              c.array_dimension(field_index));
    }

    CHECK(data.template get_array_dims<field::DOff>(0) == 1ul);
    CHECK(data.template get_array_dims<field::DOn>(0) == 1ul);
}

TEST_CASE("soa::get_num_variables", "[Neuron][data_structures]") {
    storage data;

    data.set_field_status<field::DOn>(true);
    data.set_field_status<field::DOff>(false);

    auto c = field::C{};

    CHECK(data.get_num_variables<field::C>() == c.num_variables());
    CHECK(data.get_num_variables<field::DOff>() == 1ul);
    CHECK(data.get_num_variables<field::DOn>() == 1ul);
}
