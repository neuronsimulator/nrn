#pragma once
#include "neuron/container/soa_container.hpp"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"

#include <optional>
#include <vector>

namespace neuron::cache {
/** @brief Structure to store cached data for a mechanism.
 */
struct Mechanism {
    std::vector<std::vector<Datum>> pdata{};
};

struct Node {
    // This is just a toy example for the unit tests, which should be updated to
    // use something "realistic" when reasonable.
    std::vector<double*> voltage_ptrs{};
};

struct Thread {
    std::vector<Mechanism> mech{};
};

/** @brief Collect temporary cache data that is needed to simulate the model.
 *
 *  The data contained here have a lifetime that ends as soon as any
 *  modification is made to the model structure. This means that it is safe to
 *  store raw pointer versions of data handles, and to otherwise assume that
 *  addresses and offsets are constant.
 */
struct Model {
    Node node_data{};
    std::vector<Thread> thread{};
};

namespace detail {
// Defined in container.cpp to avoid problems with dlopen and inline variables.
extern std::optional<Model> model_cache;
}  // namespace detail

struct model_token {
    model_token(model_sorted_token token, Model& model_cache)
        : m_token{std::move(token)}
        , m_model_cache{model_cache} {}
    Model& operator*() {
        return m_model_cache.get();
    }
    Model const& operator*() const {
        return m_model_cache.get();
    }
    Model* operator->() {
        return &(m_model_cache.get());
    }
    Model const* operator->() const {
        return &(m_model_cache.get());
    }

  private:
    model_sorted_token m_token;
    std::reference_wrapper<Model> m_model_cache;
};

model_token acquire_valid();
Model generate(model_sorted_token const&);
void invalidate();
}  // namespace neuron::cache