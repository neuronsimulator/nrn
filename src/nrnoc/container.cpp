#include "hocdec.h"
#include "multicore.h"
#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

#include <optional>

namespace {
std::optional<neuron::cache::Model> model_cache{};
}  // namespace

namespace neuron {
Model::Model() {
    m_node_data.set_unsorted_callback(cache::invalidate);
}
namespace detail {
Model model_data;  // neuron/model_data.hpp
}  // namespace detail
}  // namespace neuron

namespace neuron::cache {
void invalidate() {
    model_cache.reset();
}

/** @brief Generate cache data, assuming the model is sorted.
 *
 *  The token argument is what gives us confidence that the model is already sorted.
 */
Model generate(model_sorted_token const&) {
    auto& model = neuron::model();
    auto& node_data = model.node_data();
    auto const node_size = node_data.size();
    Model cache{};
    // TODO: const-ness is a bit awkward here
    // TODO: should be able to do a range-for over node_data and get non-owning
    // Node views?
    auto& vptrs = cache.node_data.voltage_ptrs;
    vptrs.resize(node_size);
    for (auto i = 0ul; i < node_size; ++i) {
        vptrs[i] = &node_data.get<neuron::container::Node::field::Voltage>(i);
    }
    return cache;
}

/** @brief Acquire valid cache data, reusing it if possible.
 */
model_token acquire_valid() {
    // Make sure that the data are sorted and mark them read-only
    auto model_is_sorted = nrn_ensure_model_data_are_sorted();
    // If the cache already exists, it should be valid -- operations that
    // invalidate it are supposed to eagerly destroy it.
    if (!model_cache) {
        // Cache is not valid, but the data are fixed in read-only mode and the
        // `model_is_sorted` token ensures it will stay that way
        model_cache = generate(model_is_sorted);
    }
    return {std::move(model_is_sorted), *model_cache};
}
}  // namespace neuron::cache
