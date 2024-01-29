#include "../../nrnconf.h"
#include "section.h"
#include "membfunc.h"
#include "nrniv_mf.h"
#include "netcon.h"
#include <map>
#include "neuron.h"
#include "utils/enumerate.h"

#define precision 15

void nrn_prcellstate(int gid, const char* filesuffix);

static void pr_memb(int type,
                    Memb_list* ml,
                    int* cellnodes,
                    NrnThread& nt,
                    FILE* f,
                    std::map<void*, int>& pnt2index) {
    int header_printed = 0;
    int size = nrn_prop_param_size_[type];
    int receives_events = pnt_receive[type] ? 1 : 0;
    for (int i = 0; i < ml->nodecount; ++i) {
        int inode = ml->nodeindices[i];
        if (cellnodes[inode] >= 0) {
            if (!header_printed) {
                header_printed = 1;
                fprintf(f, "type=%d %s size=%d\n", type, memb_func[type].sym->name, size);
            }
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
                fprintf(f, " %.*g", precision, nc->weight_[k]);
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

    // count the number of nodes in the cell
    // do not assume all cell nodes except the root are contiguous
    int* cellnodes = new int[nt.end];
    for (int i = 0; i < nt.end; ++i) {
        cellnodes[i] = -1;
    }
    int cnt = 0;
    cellnodes[rnode] = cnt++;
    for (int i = nt.ncell; i < nt.end; ++i) {
        if (cellnodes[nt._v_parent_index[i]] >= 0) {
            cellnodes[i] = cnt++;
        }
    }
    fprintf(f, "%d nodes  %d is the threshold node\n", cnt, cellnodes[inode] - 1);
    fprintf(f, " threshold %.*g\n", precision, ps.threshold_);
    fprintf(f, "inode parent area a b\n");
    for (int i = 0; i < nt.end; ++i)
        if (cellnodes[i] >= 0) {
            Node* nd = nt._v_node[i];
            fprintf(f,
                    "%d %d %.*g %.*g %.*g\n",
                    cellnodes[i],
                    i < nt.ncell ? -1 : cellnodes[nt._v_parent_index[i]],
                    precision,
                    NODEAREA(nd),
                    precision,
                    nd->a(),
                    precision,
                    nd->b());
        }
    fprintf(f, "inode v\n");
    for (int i = 0; i < nt.end; ++i)
        if (cellnodes[i] >= 0) {
            Node* nd = nt._v_node[i];  // if not cach_efficient then _actual_v=NULL
            fprintf(f, "%d %.*g\n", cellnodes[i], precision, NODEV(nd));
        }

    {
        std::map<void*, int> pnt2index;
        // each mechanism
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            pr_memb(tml->index, tml->ml, cellnodes, nt, f, pnt2index);
        }

        // the NetCon info
        pr_netcon(nt, f, pnt2index);
    }
    delete[] cellnodes;
}

void nrn_prcellstate(int gid, const char* suffix) {
    PreSyn* ps = nrn_gid2outputpresyn(gid);
    if (!ps) {
        return;
    }
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
