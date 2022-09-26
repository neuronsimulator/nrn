#include "hocdec.h"
#include "multicore.h"
#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

#include <optional>
extern int n_memb_func;
extern Memb_func* memb_func;
extern int* nrn_prop_dparam_size_;

namespace {
std::optional<neuron::cache::Model> model_cache{};
neuron::cache::Mechanism generate_mech_cache(Memb_list const& ml, int mech_type) {
    neuron::cache::Mechanism mech_cache{};
    // Mechanism name
    [[maybe_unused]] const char* mech_name = memb_func[mech_type].sym->name;
    std::cout << "Setting up cache for " << ml.nodecount << " instances of mechanism type " << mech_type << " (" << mech_name << ')' << std::endl;
    
    auto& mech_pdata = mech_cache.pdata;
    auto const dparam_size = nrn_prop_dparam_size_[mech_type];
    // TODO: transpose this so mech_pdata.size() == dparam_size, this is already
    // roughly what's done in CoreNEURON but not immediately doing it here for
    // expedience
    mech_pdata.resize(ml.nodecount);
    // Allocate storage
    for (int i = 0; i < mech_pdata.size(); ++i) {
        mech_pdata[i].resize(dparam_size);
    }
    for (int i = 0; i < ml.nodecount; ++i) {
        for (int j = 0; j < dparam_size; ++j) {
            auto const var_semantics = memb_func[mech_type].dparam_semantics[j];
            auto datum_value = ml.pdata[i][j];
            if (var_semantics == -5) {
                // POINTER variable, these are stored in ml->pdata as data handles
                auto const& generic_handle = *datum_value.generic_handle;
                auto* const pd = static_cast<double*>(generic_handle);
                mech_pdata[i][j]._pvoid = pd;
            } else {
                // Not POINTER, copy the Datum as-is for now
                mech_pdata[i][j] = datum_value;
            }
        }
    }
    return mech_cache;
}
neuron::cache::Thread generate_thread_cache(NrnThread& nt) {
    neuron::cache::Thread nt_cache;
    nt_cache.mech.resize(n_memb_func);
    // Non-artificial cells
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        Memb_list* const ml{tml->ml};
        assert(ml);
        if (!ml->nodecount) {
            // No point setting up caches for a mechanism with no instances
            continue;
        }
        // Mechanism type
        int const type{tml->index};
        assert(type < nt_cache.mech.size());
        nt_cache.mech[type] = generate_mech_cache(*ml, type);
    }
    return nt_cache;
}
}

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
    if (!model_cache.has_value()) {
        return;
    }
    for (auto nth = 0; nth < nrn_nthread; ++nth) {
        auto* const old_ptr = std::exchange(nrn_threads[nth].cache, nullptr);
        assert(old_ptr);
    }
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
    // Generate flat arrays of pointers from data_handles for POINTER variables.
    // In the further future this could also be an opportunity to substitute
    // device pointers for GPU execution..?
    std::transform(nrn_threads, nrn_threads + nrn_nthread, std::back_inserter(cache.thread), generate_thread_cache);
    // Handle artificial cells that do not have thread-specific data
    cache.art_cell.resize(n_memb_func);
    for (int mech_type = 0; mech_type < n_memb_func; ++mech_type) {
        if (!nrn_is_artificial_[mech_type]) {
            continue;
        }
        cache.art_cell[mech_type] = generate_mech_cache(memb_list[mech_type], mech_type);
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
        // Set up the pointer so it's possible to navigate to the cache from
        // NrnThread objects -- this is just to ease migration and is
        // conceptually a little dubious.
        for (auto nth = 0; nth < nrn_nthread; ++nth) {
            auto* const old_ptr = std::exchange(nrn_threads[nth].cache, &(*model_cache));
            assert(!old_ptr);
        }
    }
    return {std::move(model_is_sorted), *model_cache};
}
}  // namespace neuron::cache
