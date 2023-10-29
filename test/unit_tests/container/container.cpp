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

TEST_CASE("defer delete storage pointer", "[Neuron][internal][data_structures]") {
    REQUIRE(detail::defer_delete_storage != nullptr);

    auto usage_before = detail::compute_defer_delete_storage_size();
    { storage data; }
    auto usage_after = detail::compute_defer_delete_storage_size();

    CHECK(usage_after.size - usage_before.size > 0);
    CHECK(usage_after.capacity > 0);
    CHECK(usage_before.size <= usage_before.capacity);
    CHECK(usage_after.size <= usage_after.capacity);
}

template <class Tag, class Storage>
std::size_t compute_row_size(const Storage& data) {
    std::size_t local_size = 0ul;
    auto tag = data.template get_tag<Tag>();
    for (int field_index = 0; field_index < detail::get_num_variables(tag); ++field_index) {
        local_size += data.template get_array_dims<Tag>()[field_index] * sizeof(typename Tag::type);
    }

    return local_size;
}

TEST_CASE("container memory usage", "[Neuron][internal][data_structures]") {
    storage data;
    data.set_field_status<field::DOn>(true);
    data.set_field_status<field::DOff>(false);

    std::size_t row_size = compute_row_size<field::A>(data) + compute_row_size<field::B>(data) +
                           compute_row_size<field::C>(data) + compute_row_size<field::DOn>(data);

    auto r1 = owning_handle{data};
    auto r2 = owning_handle{data};
    auto r3 = owning_handle{data};

    auto n_rows = data.size();

    auto usage = memory_usage(data);

    CHECK(usage.heavy_data.size == row_size * n_rows);
    CHECK(usage.heavy_data.size <= usage.heavy_data.capacity);

    CHECK(usage.stable_identifiers.size % n_rows == 0);
    CHECK(usage.stable_identifiers.size >= n_rows * sizeof(std::size_t*));
    CHECK(usage.stable_identifiers.size < n_rows * 4 * sizeof(std::size_t*));
    CHECK(usage.stable_identifiers.size <= usage.stable_identifiers.capacity);
}

TEST_CASE("model memory usage", "[Neuron][internal][data_structures]") {
    auto& model = neuron::model();

    auto& nodes = model.node_data();
    auto node1 = neuron::container::Node::owning_handle{nodes};

    auto& foo = model.add_mechanism(0,
                                    "foo",
                                    std::vector<neuron::container::Mechanism::Variable>{{"a", 1},
                                                                                        {"b", 2},
                                                                                        {"c", 1}});
    auto foo1 = neuron::container::Mechanism::owning_handle{foo};
    auto foo2 = neuron::container::Mechanism::owning_handle{foo};

    auto& bar = model.add_mechanism(1,
                                    "bar",
                                    std::vector<neuron::container::Mechanism::Variable>{{"a", 1}});
    auto bar1 = neuron::container::Mechanism::owning_handle{bar};
    auto bar2 = neuron::container::Mechanism::owning_handle{bar};

    auto usage = neuron::container::memory_usage(model);
    CHECK(usage.nodes.heavy_data.size > 0);
    CHECK(usage.nodes.heavy_data.size <= usage.nodes.heavy_data.capacity);
    CHECK(usage.nodes.stable_identifiers.size > 0);
    CHECK(usage.nodes.stable_identifiers.size <= usage.nodes.stable_identifiers.capacity);

    CHECK(usage.mechanisms.heavy_data.size > 0);
    CHECK(usage.mechanisms.heavy_data.size <= usage.mechanisms.heavy_data.capacity);
    CHECK(usage.mechanisms.stable_identifiers.size > 0);
    CHECK(usage.mechanisms.stable_identifiers.size <= usage.mechanisms.stable_identifiers.capacity);
}

TEST_CASE("cache::model memory_usage", "[Neuron][internal][data_structures]") {
    auto& model = neuron::cache::model;

    // We can't manipulate `cache::Model`, hence there nothing to check other
    // than the fact that it compiles and runs without throwing.
    auto usage = neuron::container::memory_usage(model);
}

TEST_CASE("format_memory", "[Neuron][internal]") {
    size_t kb = 1e3;
    size_t mb = 1e6;
    size_t gb = 1e9;
    size_t tb = 1e12;

    CHECK(neuron::container::format_memory(0) == "     0   ");
    CHECK(neuron::container::format_memory(1) == "     1   ");
    CHECK(neuron::container::format_memory(999) == "   999   ");
    CHECK(neuron::container::format_memory(kb) == "  1.00 kB");
    CHECK(neuron::container::format_memory(999 * kb) == "999.00 kB");
    CHECK(neuron::container::format_memory(mb) == "  1.00 MB");
    CHECK(neuron::container::format_memory(gb) == "  1.00 GB");
    CHECK(neuron::container::format_memory(tb) == "  1.00 TB");
}

neuron::container::MemoryUsage dummy_memory_usage() {
    auto model =
        neuron::container::ModelMemoryUsage{neuron::container::StorageMemoryUsage{{1, 11}, {2, 12}},
                                            neuron::container::StorageMemoryUsage{{3, 13},
                                                                                  {4, 14}}};
    auto cache_model = neuron::container::cache::ModelMemoryUsage{{5, 15}, {6, 16}};

    auto stable_pointers = neuron::container::VectorMemoryUsage(7, 17);
    auto stable_identifiers = neuron::container::VectorMemoryUsage(8, 18);

    auto memory_usage = MemoryUsage{model, cache_model, stable_pointers};

    return memory_usage;
}


TEST_CASE("total memory usage", "[Neuron][internal][data_structures]") {
    auto memory_usage = dummy_memory_usage();
    auto total = memory_usage.compute_total();
    CHECK(total.size == (7 * 8) / 2);
    CHECK(total.capacity == total.size + 7 * 10);
}

TEST_CASE("memory usage summary", "[Neuron][data_structures]") {
    auto usage = dummy_memory_usage();
    auto summary = neuron::container::MemoryUsageSummary(usage);
    auto total = usage.compute_total();

    size_t summary_total = summary.required + summary.convenient + summary.oversized +
                           summary.leaked;
    CHECK(summary.required <= total.size);
    CHECK(summary.convenient <= total.size);
    CHECK(summary.leaked <= total.size);
    CHECK(summary.oversized == total.capacity - total.size);
    CHECK(summary_total == total.capacity);
}
