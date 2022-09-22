#pragma once
#include "neuron/container/soa_container.hpp"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"

#include <optional>
#include <vector>

namespace neuron {
struct NodeDataCache {
    std::vector<double*> m_voltages{};
};

/** @brief Collect temporary cache data that is needed to simulate the model.
 *
 *  The data contained here have a lifetime that ends as soon as any
 *  modification is made to the model structure. This means that it is safe to
 *  store raw pointer versions of data handles, and to otherwise assume that
 *  addresses and offsets are constant.
 */
struct ModelCache {
    NodeDataCache m_node{};
};

namespace detail {
// Defined in container.cpp to avoid problems with dlopen and inline variables.
extern std::optional<ModelCache> model_cache;
}  // namespace detail

struct model_token {
    model_token(model_sorted_token token, ModelCache& model_cache)
        : m_token{std::move(token)}
        , m_model_cache{model_cache} {}
    ModelCache& operator*() {
        return m_model_cache.get();
    }
    ModelCache* operator->() {
        return &(m_model_cache.get());
    }

  private:
    model_sorted_token m_token;
    std::reference_wrapper<ModelCache> m_model_cache;
};

/** @brief Generate cache data, assuming the model is sorted.
 *
 *  The token argument is what gives us confidence that the model is already sorted.
 */
inline ModelCache generate_model_cache(neuron::model_sorted_token const&) {
    auto const& model = neuron::model();
    ModelCache cache{};
    return cache;
}

inline model_token acquire_valid_model_cache() {
    // Make sure that the data are sorted and mark them read-only
    auto model_is_sorted = nrn_ensure_model_data_are_sorted();
    // If the cache already exists, it should be valid -- operations that
    // invalidate it are supposed to eagerly destroy it.
    if (!detail::model_cache) {
        // Cache is not valid, but the data are fixed in read-only mode and the
        // `model_is_sorted` token ensures it will stay that way
        detail::model_cache = generate_model_cache(model_is_sorted);
    }
    return {std::move(model_is_sorted), *detail::model_cache};
}
}  // namespace neuron