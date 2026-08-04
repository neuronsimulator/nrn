#include "../../nrnconf.h"
#include "section.h"
#include "cabcode.h"
#include "membfunc.h"
#include "nrniv_mf.h"
#include "netcon.h"
#include <algorithm>
#include <map>
#include <queue>
#include <string>
#include <tuple>
#include <vector>
#include "neuron.h"
#include "utils/enumerate.h"

#if defined(NRN_ENABLE_GPU)
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/sync.hpp"
#endif

#define precision 15

void nrn_prcellstate(int gid, const char* filesuffix);

/** Morphological sort key so cell-local inode labels are independent of
 *  interleave permute (type 1/2). Without this, CPU (no permute) vs native GPU
 *  (permute 2) dumps renumber compartments and rdcellstate false-diffs.
 *  CoreNEURON prcellstate uses inv_permute for the same reason; NEURON applies
 *  permute in-place and does not keep nt._permute, so use section identity.
 */
static auto prcell_node_morph_key(Node* nd) {
    const char* sn = (nd && nd->sec) ? secname(nd->sec) : "";
    const int segi = nd ? nd->sec_node_index_ : -1;
    const double area = nd ? NODEAREA(nd) : 0.0;
    const double a = nd ? nd->a() : 0.0;
    const double b = nd ? nd->b() : 0.0;
    return std::make_tuple(std::string(sn ? sn : ""), segi, area, a, b);
}

static void pr_memb(int type,
                    Memb_list* ml,
                    int* cellnodes,
                    NrnThread& nt,
                    FILE* f,
                    std::map<void*, int>& pnt2index) {
    int size = nrn_prop_param_size_[type];
    int receives_events = pnt_receive[type] ? 1 : 0;
    // Visit instances in cell-local inode order (stable across node permute).
    std::vector<int> inst;
    inst.reserve(static_cast<size_t>(ml->nodecount));
    for (int i = 0; i < ml->nodecount; ++i) {
        int inode = ml->nodeindices[i];
        if (inode >= 0 && inode < nt.end && cellnodes[inode] >= 0) {
            inst.push_back(i);
        }
    }
    std::sort(inst.begin(), inst.end(), [&](int ia, int ib) {
        int ca = cellnodes[ml->nodeindices[ia]];
        int cb = cellnodes[ml->nodeindices[ib]];
        if (ca != cb) {
            return ca < cb;
        }
        // Multi-PP on same node: order by SoA field 0 then instance index.
        if (size > 0) {
            double va = ml->data(ia, 0);
            double vb = ml->data(ib, 0);
            if (va != vb) {
                return va < vb;
            }
        }
        return ia < ib;
    });
    if (inst.empty()) {
        return;
    }
    fprintf(f, "type=%d %s size=%d\n", type, memb_func[type].sym->name, size);
    for (int i: inst) {
        int inode = ml->nodeindices[i];
        if (receives_events) {
            fprintf(f, "%d nri %lu\n", cellnodes[inode], pnt2index.size());
            auto* pp = ml->pdata[i][1].get<Point_process*>();
            pnt2index.emplace(pp, pnt2index.size());
        }
        for (int j = 0; j < size; ++j) {
            fprintf(f, " %d %d %.*g\n", cellnodes[inode], j, precision, ml->data(i, j));
        }
    }
}

static void pr_netcon(NrnThread& nt, FILE* f, const std::map<void*, int>& pnt2index) {
    if (pnt2index.empty()) {
        return;
    }
    // pnt2index table has been filled

    // List of NetCon for each of the NET_RECEIVE point process instances
    // ... all NetCon list in the hoc NetCon cTemplate
    std::vector<std::vector<NetCon*>> nclist(pnt2index.size());
    int nc_cnt = 0;
    Symbol* ncsym = hoc_lookup("NetCon");
    hoc_Item* q;
    ITERATE(q, ncsym->u.ctemplate->olist) {
        Object* obj = OBJ(q);
        NetCon* nc = (NetCon*) obj->u.this_pointer;
        Point_process* pp = nc->target_;
        const auto& it = pnt2index.find(pp);
        if (it != pnt2index.end()) {
            nclist[it->second].push_back(nc);
            ++nc_cnt;
        }
    }
    fprintf(f, "netcons %d\n", nc_cnt);
    fprintf(f, " pntindex srcgid active delay weights\n");
    for (const auto&& [i, ncl]: enumerate(nclist)) {
        for (const auto& nc: ncl) {
            int srcgid = (nc->src_) ? nc->src_->gid_ : -3;
            if (srcgid < 0 && nc->src_ && nc->src_->osrc_) {
                const char* name = nc->src_->osrc_->ctemplate->sym->name;
                fprintf(f, "%zd %s %d %.*g", i, name, nc->active_ ? 1 : 0, precision, nc->delay_);
            } else if (srcgid < 0 && nc->src_ && nc->src_->ssrc_) {
                fprintf(f, "%zd %s %d %.*g", i, "v", nc->active_ ? 1 : 0, precision, nc->delay_);
            } else {
                fprintf(f, "%zd %d %d %.*g", i, srcgid, nc->active_ ? 1 : 0, precision, nc->delay_);
            }
            int wcnt = pnt_receive_size[nc->target_->prop->_type];
            for (int k = 0; k < wcnt; ++k) {
                fprintf(f,
                        " %.*g",
                        precision,
                        nc->has_weight_soa() && k < nc->weight_block_->size()
                            ? nc->weight_soa_value(k)
                            : 0.);
            }
            fprintf(f, "\n");
        }
    }
}

static void pr_realcell(PreSyn& ps, NrnThread& nt, FILE* f) {
    assert(ps.thvar_);
    // threshold variable is a voltage

    // If the "modern" data is "sorted" then the order should match the "legacy"
    // data structures that still live alongside it
    auto const cache_token = nrn_ensure_model_data_are_sorted();
    assert(
        ps.thvar_.refers_to<neuron::container::Node::field::Voltage>(neuron::model().node_data()));
    int const inode = ps.thvar_.current_row() - cache_token.thread_cache(nt.id).node_data_offset;
    // hoc_execerror("gid not associated with a voltage", 0);

    // and the root node is ...
    int rnode = inode;
    while (rnode >= nt.ncell) {
        rnode = nt._v_parent_index[rnode];
    }

    // Membership: node belongs to this cell if parent-walk reaches rnode.
    // (Single-pass thread-order mark is wrong after interleave permute may
    // reorder siblings; parent-walk is order-independent.)
    std::vector<int> cellnodes(static_cast<size_t>(nt.end), -1);
    for (int i = 0; i < nt.end; ++i) {
        int j = i;
        while (j >= nt.ncell) {
            j = nt._v_parent_index[j];
        }
        if (j == rnode) {
            cellnodes[static_cast<size_t>(i)] = 0;  // temporary mark
        }
    }

    // Children lists among cell members; sort siblings by morphological key so
    // BFS local IDs match CPU (no permute) vs GPU (permute 2).
    std::vector<std::vector<int>> children(static_cast<size_t>(nt.end));
    for (int i = 0; i < nt.end; ++i) {
        if (cellnodes[static_cast<size_t>(i)] < 0 || i == rnode) {
            continue;
        }
        int ip = nt._v_parent_index[i];
        children[static_cast<size_t>(ip)].push_back(i);
    }
    for (auto& ch: children) {
        std::sort(ch.begin(), ch.end(), [&](int ia, int ib) {
            return prcell_node_morph_key(nt._v_node[ia]) < prcell_node_morph_key(nt._v_node[ib]);
        });
    }

    // BFS from root → cell-local inode 0..cnt-1
    int cnt = 0;
    for (int i = 0; i < nt.end; ++i) {
        cellnodes[static_cast<size_t>(i)] = -1;
    }
    std::queue<int> q;
    cellnodes[static_cast<size_t>(rnode)] = cnt++;
    q.push(rnode);
    while (!q.empty()) {
        int p = q.front();
        q.pop();
        for (int c: children[static_cast<size_t>(p)]) {
            cellnodes[static_cast<size_t>(c)] = cnt++;
            q.push(c);
        }
    }

    // Inverse: cell-local id → thread node index (print in local order)
    std::vector<int> local_to_thread(static_cast<size_t>(cnt), -1);
    for (int i = 0; i < nt.end; ++i) {
        int loc = cellnodes[static_cast<size_t>(i)];
        if (loc >= 0) {
            local_to_thread[static_cast<size_t>(loc)] = i;
        }
    }

    fprintf(f, "%d nodes  %d is the threshold node\n", cnt, cellnodes[static_cast<size_t>(inode)] - 1);
    fprintf(f, " threshold %.*g\n", precision, ps.threshold_);
    fprintf(f, "inode parent area a b d rhs\n");
    for (int loc = 0; loc < cnt; ++loc) {
        int i = local_to_thread[static_cast<size_t>(loc)];
        Node* nd = nt._v_node[i];
        fprintf(f,
                "%d %d %.*g %.*g %.*g %.*g %.*g\n",
                loc,
                i < nt.ncell ? -1 : cellnodes[static_cast<size_t>(nt._v_parent_index[i])],
                precision,
                NODEAREA(nd),
                precision,
                nd->a(),
                precision,
                nd->b(),
                precision,
                nd->d(),
                precision,
                nd->rhs());
    }
    fprintf(f, "inode v\n");
    for (int loc = 0; loc < cnt; ++loc) {
        int i = local_to_thread[static_cast<size_t>(loc)];
        Node* nd = nt._v_node[i];
        fprintf(f, "%d %.*g\n", loc, precision, NODEV(nd));
    }

    {
        std::map<void*, int> pnt2index;
        // each mechanism
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            pr_memb(tml->index, tml->ml, cellnodes.data(), nt, f, pnt2index);
        }

        // the NetCon info
        pr_netcon(nt, f, pnt2index);
    }
}

void nrn_prcellstate(int gid, const char* suffix) {
    PreSyn* ps = nrn_gid2outputpresyn(gid);
    if (!ps) {
        return;
    }
#if defined(NRN_ENABLE_GPU)
    if (neuron::gpu::enabled() && neuron::gpu::backend_native()) {
        neuron::gpu::sync_all_device_streams();
        neuron::gpu::sync_state_to_host_for_host_reads();
    }
#endif
    // found it so create a <gid>_<suffix>.nrn file
    char buf[200];
    Sprintf(buf, "%d_%s.nrndat", gid, suffix);
    FILE* f = fopen(buf, "w");
    assert(f);
    NrnThread& nt = *ps->nt_;
    fprintf(f, "gid = %d\n", gid);
    fprintf(f, "t = %.*g\n", precision, nt._t);
    fprintf(f, "celsius = %.*g\n", precision, celsius);
    if (ps->thvar_) {
        pr_realcell(*ps, nt, f);
    }
    fclose(f);
}
