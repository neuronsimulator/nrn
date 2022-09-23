#include "hocdec.h"
#include "multicore.h"
#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

#include <optional>
extern int n_memb_func;
extern Memb_func* memb_func;
extern int* nrn_prop_dparam_size_;
namespace neuron {
Model::Model() {
    m_node_data.set_unsorted_callback(cache::invalidate);
}
namespace detail {
Model model_data;  // neuron/model_data.hpp
}  // namespace detail
}  // namespace neuron

namespace neuron::cache {
namespace detail {
std::optional<Model> model_cache{};  // neuron/cache/model_data.hpp
}  // namespace detail
void invalidate() {
    detail::model_cache = std::nullopt;
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
    // Generate flat arrays of pointers from data_handles for POINTER variables
    {
        cache.thread.resize(nrn_nthread);
        for (auto nth = 0; nth < nrn_nthread; ++nth) {
            auto const& nt = nrn_threads[nth];
            auto& nt_cache = cache.thread.at(nth);
            nt_cache.mech.resize(n_memb_func);
            for (auto* tml = nt.tml; tml; tml = tml->next) {
                Memb_list* const ml{tml->ml};
                if (!ml->nodecount) {
                    // No point setting up caches for a mechanism with no instances
                    continue;
                }
                // Mechanism type
                int const type{tml->index};
                // Mechanism name
                [[maybe_unused]] const char* mech_name = memb_func[type].sym->name;
                assert(type < nt_cache.mech.size());
                auto const dparam_size = nrn_prop_dparam_size_[type];
                auto& mech_pdata = nt_cache.mech[type].pdata;
                mech_pdata.resize(dparam_size);
                // Allocate storage
                for (int i = 0; i < dparam_size; ++i) {
                    auto const var_semantics = memb_func[type].dparam_semantics[i];
                    mech_pdata[i].resize(ml->nodecount);
                }
                for (int i = 0; i < ml->nodecount; ++i) {
                    for (int j = 0; j < dparam_size; ++j) {
                        auto const var_semantics = memb_func[type].dparam_semantics[j];
                        auto datum_value = ml->pdata[i][j];
                        if (var_semantics == -5) {
                            // POINTER variable, these are stored in ml->pdata
                            // as data handles
                            auto const& generic_handle = *datum_value.generic_handle;
                            auto* const pd = static_cast<double*>(generic_handle);
                            mech_pdata[j][i]._pvoid = pd;
                        } else {
                            // Not POINTER, copy the Datum as-is for now
                            ml->pdata[i][j] = datum_value;
                        }
                    }
                }
            }
        }
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
    if (!detail::model_cache) {
        // Cache is not valid, but the data are fixed in read-only mode and the
        // `model_is_sorted` token ensures it will stay that way
        detail::model_cache = generate(model_is_sorted);
    }
    return {std::move(model_is_sorted), *detail::model_cache};
}
}  // namespace neuron::cache
