#include "../../nrnconf.h"
#include "section.h"
#include "membfunc.h"
#include "nrniv_mf.h"
#include "netcon.h"
#include <map>
#include "OS/list.h"
#include "neuron.h"

#define precision 15

void nrn_prcellstate(int gid, const char* filesuffix);

declarePtrList(NetConList, NetCon);    // NetCons in same order as Point_process
implementPtrList(NetConList, NetCon);  // and there may be several per pp.


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
                fprintf(f, " %d %d %.*g\n", cellnodes[inode], j, precision, ml->_data[i][j]);
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
    NetConList** nclist = new NetConList*[pnt2index.size()];
    for (size_t i = 0; i < pnt2index.size(); ++i) {
        nclist[i] = new NetConList(1);
    }
    int nc_cnt = 0;
    Symbol* ncsym = hoc_lookup("NetCon");
    hoc_Item* q;
    ITERATE(q, ncsym->u.ctemplate->olist) {
        Object* obj = OBJ(q);
        NetCon* nc = (NetCon*) obj->u.this_pointer;
        Point_process* pp = nc->target_;
        const auto& it = pnt2index.find(pp);
        if (it != pnt2index.end()) {
            nclist[it->second]->append(nc);
            ++nc_cnt;
        }
    }
    fprintf(f, "netcons %d\n", nc_cnt);
    fprintf(f, " pntindex srcgid active delay weights\n");
    for (size_t i = 0; i < pnt2index.size(); ++i) {
        for (int j = 0; j < nclist[i]->count(); ++j) {
            NetCon* nc = nclist[i]->item(j);
            int srcgid = -3;
            srcgid = (nc->src_) ? nc->src_->gid_ : -3;
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
    // cleanup
    for (size_t i = 0; i < pnt2index.size(); ++i) {
        delete nclist[i];
    }
    delete[] nclist;
}

static void pr_realcell(PreSyn& ps, NrnThread& nt, FILE* f) {
    // threshold variable is a voltage
    assert(false);
    // printf("thvar=%p actual_v=%p end=%p\n",
    //        static_cast<double*>(ps.thvar_),
    //        nt._actual_v,
    //        nt._actual_v + nt.end);
    int inode = -1;
    // if (ps.thvar_ < nt._actual_v || ps.thvar_ >= (nt._actual_v + nt.end)) {
    //     assert(false);
    // if (ps.ssrc_) { /* not cache efficient, search the nodes in this section */
    //     printf("%s\n", ps.ssrc_ ? secname(ps.ssrc_) : "unknown");
    //     for (int i = 0; i < ps.ssrc_->nnode; ++i) {
    //         Node* nd = ps.ssrc_->pnode[i];
    //         // TODO this is broken
    //         if (ps.thvar_ == nd->_v) {
    //             inode = nd->v_node_index;
    //             break;
    //         }
    //     }
    //     if (inode < 0) { /* check parent node */
    //         Node* nd = ps.ssrc_->parentnode;
    //         // TODO this is broken
    //         if (ps.thvar_ == nd->_v) {
    //             inode = nd->v_node_index;
    //         }
    //     }
    // }
    // if (inode < 0) {
    //     hoc_execerror("gid not associated with a voltage", 0);
    // }
    // } else {
    //     inode = ps.thvar_ - nt._actual_v;
    // }

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
            Node* nd = nt._v_node[i];  // if not cach_efficient then _actual_area=NULL
            fprintf(f,
                    "%d %d %.*g %.*g %.*g\n",
                    cellnodes[i],
                    i < nt.ncell ? -1 : cellnodes[nt._v_parent_index[i]],
                    precision,
                    NODEAREA(nd),
                    precision,
                    nt._actual_a[i],
                    precision,
                    nt._actual_b[i]);
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
