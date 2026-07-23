/**
 * @file network_soa_sort.cpp
 * @brief Network SoA sort / weight repack for nrn_ensure_model_data_are_sorted.
 */
#include "neuron/container/network/point_process_access.hpp"
#include "neuron/container/network/sort.hpp"
#include "netcon.h"
#include "netcvode.h"
#include "nrniv_mf.h"
#include "section.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

extern cTemplate** nrn_pnt_template_;
extern int n_memb_func;
extern NetCvode* net_cvode_instance;

namespace neuron::container::network {
namespace {

constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

/** @brief Sync all Point_process dual-write rows from HOC point-process lists. */
void sync_all_point_processes() {
    if (!nrn_pnt_template_) {
        return;
    }
    for (int type = 0; type < n_memb_func; ++type) {
        cTemplate* tmp = nrn_pnt_template_[type];
        if (!tmp || !tmp->olist) {
            continue;
        }
        hoc_Item* q = nullptr;
        ITERATE(q, tmp->olist) {
            // HOC point-process templates store Point_process* in dataspace
            // (not this_pointer); use ob2pntproc_0 for both C and HOC shells.
            Point_process* pnt = ob2pntproc_0(OBJ(q));
            if (pnt) {
                nrn_point_process_soa_sync(pnt);
            }
        }
    }
}

/** @brief Enumerate live NetCon shells from the HOC template list. */
std::vector<::NetCon*> all_netcons() {
    std::vector<::NetCon*> out;
    Symbol* sym = hoc_lookup("NetCon");
    if (!sym || !sym->u.ctemplate || !sym->u.ctemplate->olist) {
        return out;
    }
    out.reserve(static_cast<std::size_t>(sym->u.ctemplate->count));
    hoc_Item* q = nullptr;
    ITERATE(q, sym->u.ctemplate->olist) {
        auto* nc = static_cast<::NetCon*>(OBJ(q)->u.this_pointer);
        if (nc) {
            out.push_back(nc);
        }
    }
    return out;
}

/** @brief Enumerate live PreSyn shells (psl_ when available). */
std::vector<::PreSyn*> all_presyns() {
    std::vector<::PreSyn*> out;
    if (net_cvode_instance && net_cvode_instance->psl_) {
        out.reserve(net_cvode_instance->psl_->size());
        for (::PreSyn* ps: *net_cvode_instance->psl_) {
            if (ps) {
                out.push_back(ps);
            }
        }
    }
    return out;
}

int target_thread_id(::NetCon* nc) {
    if (!nc || !nc->target_) {
        return -1;
    }
    if (nc->target_->_vnt) {
        return static_cast<NrnThread*>(nc->target_->_vnt)->id;
    }
    // Fall back to dual-write column if present.
    int const row = nrn_point_process_soa_row(nc->target_);
    if (row >= 0) {
        return neuron::model().point_processes().get<PointProcess::field::ThreadId>(
            static_cast<std::size_t>(row));
    }
    return -1;
}

int src_presyn_row(::NetCon* nc) {
    if (!nc || !nc->src_) {
        return -1;
    }
    return static_cast<int>(nc->src_->_soa.current_row());
}

/**
 * @brief Build perm[old_row] = new_row grouping rows by integer key in [0, nkey).
 * @return Number of rows placed in the keyed region (orphans follow).
 */
template <typename GetKey>
std::vector<std::size_t> partition_by_key(std::size_t n,
                                          int nkey,
                                          GetKey&& get_key,
                                          std::vector<std::size_t>& key_offsets) {
    key_offsets.assign(static_cast<std::size_t>(std::max(nkey, 0)), 0);
    std::vector<std::size_t> perm(n, npos);
    std::size_t global_i = 0;
    for (int k = 0; k < nkey; ++k) {
        key_offsets[static_cast<std::size_t>(k)] = global_i;
        for (std::size_t row = 0; row < n; ++row) {
            if (get_key(row) == k) {
                perm[row] = global_i++;
            }
        }
    }
    for (std::size_t row = 0; row < n; ++row) {
        if (perm[row] == npos) {
            perm[row] = global_i++;
        }
    }
    assert(global_i == n);
    return perm;
}

template <typename Storage, typename Token>
void apply_if_needed(Storage& store, Token& token, std::vector<std::size_t> perm) {
    if (store.size() == 0) {
        store.mark_as_sorted(token);
        return;
    }
    // apply_reverse_permutation always leaves the container sorted.
    store.apply_reverse_permutation(std::move(perm), token);
}

}  // namespace

void sort_network_data(neuron::cache::Model& cache,
                       PointProcess::storage::frozen_token_type& pp_token,
                       Weight::storage::frozen_token_type& w_token,
                       NetCon::storage::frozen_token_type& nc_token,
                       PreSyn::storage::frozen_token_type& ps_token) {
    auto& model = neuron::model();
    auto& pp_store = model.point_processes();
    auto& w_store = model.weights();
    auto& nc_store = model.netcons();
    auto& ps_store = model.presyns();

    // Ensure cache.thread is sized (caller should have done this; be defensive).
    if (cache.thread.size() < static_cast<std::size_t>(nrn_nthread)) {
        cache.thread.resize(static_cast<std::size_t>(nrn_nthread));
    }

    // ------------------------------------------------------------------
    // 0. Dual-write sync from legacy shells (values / thread membership).
    // ------------------------------------------------------------------
    sync_all_point_processes();
    auto netcons = all_netcons();
    for (::NetCon* nc: netcons) {
        // Keep WeightIndex from handles; do not heap→SoA wipe HOC-primary values.
        nc->soa_sync();
    }
    auto presyns = all_presyns();
    for (::PreSyn* ps: presyns) {
        ps->soa_sync();
    }

    // ------------------------------------------------------------------
    // 1. PointProcess: partition by thread_id.
    // ------------------------------------------------------------------
    {
        std::vector<std::size_t> pp_offsets;
        auto perm = partition_by_key(
            pp_store.size(),
            nrn_nthread,
            [&](std::size_t row) { return pp_store.get<PointProcess::field::ThreadId>(row); },
            pp_offsets);
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            cache.thread[static_cast<std::size_t>(tid)].point_process_offset =
                tid < static_cast<int>(pp_offsets.size())
                    ? pp_offsets[static_cast<std::size_t>(tid)]
                    : 0;
        }
        apply_if_needed(pp_store, pp_token, std::move(perm));
    }

    // Order NetCons by (target thread, src PreSyn SoA row, NetCon SoA row).
    struct NcSortKey {
        ::NetCon* nc{};
        int tid{-1};
        int src_row{-1};
        std::size_t soa_row{0};
    };
    std::vector<NcSortKey> ordered;
    ordered.reserve(netcons.size());
    for (::NetCon* nc: netcons) {
        NcSortKey k;
        k.nc = nc;
        k.tid = target_thread_id(nc);
        k.src_row = src_presyn_row(nc);
        k.soa_row = nc->_soa.current_row();
        ordered.push_back(k);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](NcSortKey const& a, NcSortKey const& b) {
        if (a.tid != b.tid) {
            // Unassigned (-1) after real threads.
            int const at = a.tid < 0 ? nrn_nthread : a.tid;
            int const bt = b.tid < 0 ? nrn_nthread : b.tid;
            if (at != bt) {
                return at < bt;
            }
        }
        if (a.src_row != b.src_row) {
            return a.src_row < b.src_row;
        }
        return a.soa_row < b.soa_row;
    });

    // ------------------------------------------------------------------
    // 2. Weight repack: contiguous blocks per NetCon, packed by target thread.
    // ------------------------------------------------------------------
    {
        std::size_t const wsize = w_store.size();
        std::vector<std::size_t> desired;  // old rows in new order
        desired.reserve(wsize);
        std::vector<char> used(wsize, 0);
        std::vector<std::size_t> first_pos(static_cast<std::size_t>(std::max(nrn_nthread, 0)),
                                           npos);

        for (auto const& k: ordered) {
            if (k.tid >= 0 && k.tid < nrn_nthread) {
                auto& fp = first_pos[static_cast<std::size_t>(k.tid)];
                if (fp == npos) {
                    fp = desired.size();
                }
            }
            for (auto& wh: k.nc->weight_soa_) {
                if (!wh.id()) {
                    continue;
                }
                auto const row = wh.current_row();
                if (row < wsize && !used[row]) {
                    used[row] = 1;
                    desired.push_back(row);
                }
            }
        }
        // Orphan weight rows (not owned by any live NetCon) go at the end.
        for (std::size_t row = 0; row < wsize; ++row) {
            if (!used[row]) {
                desired.push_back(row);
            }
        }
        assert(desired.size() == wsize);

        std::size_t running = 0;
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            auto const fp = first_pos[static_cast<std::size_t>(tid)];
            if (fp != npos) {
                running = fp;
            }
            cache.thread[static_cast<std::size_t>(tid)].weight_offset = running;
        }

        std::vector<std::size_t> perm(wsize, npos);
        for (std::size_t new_i = 0; new_i < desired.size(); ++new_i) {
            perm[desired[new_i]] = new_i;
        }
        apply_if_needed(w_store, w_token, std::move(perm));
    }

    // ------------------------------------------------------------------
    // 3. NetCon SoA: same order as weight packing (target thread, src, …).
    // ------------------------------------------------------------------
    {
        std::size_t const n = nc_store.size();
        std::vector<std::size_t> perm(n, npos);
        std::vector<std::size_t> first_pos(static_cast<std::size_t>(nrn_nthread), npos);
        std::size_t global_i = 0;
        std::vector<char> seen(n, 0);
        for (auto const& k: ordered) {
            auto const old = k.soa_row;
            if (old < n && !seen[old]) {
                if (k.tid >= 0 && k.tid < nrn_nthread) {
                    auto& fp = first_pos[static_cast<std::size_t>(k.tid)];
                    if (fp == npos) {
                        fp = global_i;
                    }
                }
                perm[old] = global_i++;
                seen[old] = 1;
            }
        }
        for (std::size_t row = 0; row < n; ++row) {
            if (perm[row] == npos) {
                perm[row] = global_i++;
            }
        }
        assert(global_i == n);
        std::size_t running = 0;
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            auto const fp = first_pos[static_cast<std::size_t>(tid)];
            if (fp != npos) {
                running = fp;
            }
            cache.thread[static_cast<std::size_t>(tid)].netcon_offset = running;
        }
        apply_if_needed(nc_store, nc_token, std::move(perm));
    }

    // Refresh dual-write indices after PP/weight/NetCon permutes.
    for (::NetCon* nc: netcons) {
        nc->soa_sync();
    }

    // ------------------------------------------------------------------
    // 4. PreSyn by thread + fanout ranges (NcIndex/NcCount).
    // ------------------------------------------------------------------
    {
        // Refresh thread_id on PreSyn shells before partition.
        for (::PreSyn* ps: presyns) {
            ps->soa_sync();
        }
        std::vector<std::size_t> ps_offsets;
        auto perm = partition_by_key(
            ps_store.size(),
            nrn_nthread,
            [&](std::size_t row) { return ps_store.get<PreSyn::field::ThreadId>(row); },
            ps_offsets);
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            cache.thread[static_cast<std::size_t>(tid)].presyn_offset =
                tid < static_cast<int>(ps_offsets.size())
                    ? ps_offsets[static_cast<std::size_t>(tid)]
                    : 0;
        }
        apply_if_needed(ps_store, ps_token, std::move(perm));
    }

    // Fanout order uses NetCon* ranges; rebuild after topology/sort.
    ::PreSyn::mark_fanout_unsorted();
    ::PreSyn::ensure_fanout_order();

    // Final NetCon reverse-edge refresh (src PreSyn rows may have moved).
    for (::NetCon* nc: netcons) {
        nc->soa_sync();
    }

    // Ensure all four are marked sorted even if empty / trivial perm paths.
    pp_store.mark_as_sorted(pp_token);
    w_store.mark_as_sorted(w_token);
    nc_store.mark_as_sorted(nc_token);
    ps_store.mark_as_sorted(ps_token);
}

}  // namespace neuron::container::network
