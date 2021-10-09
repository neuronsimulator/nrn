/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <vector>
#include <map>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/nrn_setup.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

#define precision 15
namespace coreneuron {
static std::map<Point_process*, int> pnt2index;  // for deciding if NetCon is to be printed
static int pntindex;                             // running count of printed point processes.
static std::map<NetCon*, DiscreteEvent*> map_nc2src;
static std::vector<int>* inv_permute_;

static int permute(int i, NrnThread& nt) {
    return nt._permute ? nt._permute[i] : i;
}

static int inv_permute(int i, NrnThread& nt) {
    nrn_assert(i >= 0 && i < nt.end);
    if (!nt._permute) {
        return i;
    }
    if (!inv_permute_) {
        inv_permute_ = new std::vector<int>(nt.end);
        for (int i = 0; i < nt.end; ++i) {
            (*inv_permute_)[nt._permute[i]] = i;
        }
    }
    return (*inv_permute_)[i];
}

static int ml_permute(int i, Memb_list* ml) {
    return ml->_permute ? ml->_permute[i] : i;
}

// Note: cellnodes array is in unpermuted order.

static void pr_memb(int type, Memb_list* ml, int* cellnodes, NrnThread& nt, FILE* f) {
    if (corenrn.get_is_artificial()[type])
        return;

    bool header_printed = false;
    int size = corenrn.get_prop_param_size()[type];
    int psize = corenrn.get_prop_dparam_size()[type];
    bool receives_events = corenrn.get_pnt_receive()[type];
    int layout = corenrn.get_mech_data_layout()[type];
    int cnt = ml->nodecount;
    for (int iorig = 0; iorig < ml->nodecount; ++iorig) {  // original index
        int i = ml_permute(iorig, ml);                     // present index
        int inode = ml->nodeindices[i];                    // inode is the permuted node
        int cix = cellnodes[inv_permute(inode, nt)];       // original index relative to this cell
        if (cix >= 0) {
            if (!header_printed) {
                header_printed = true;
                fprintf(f, "type=%d %s size=%d\n", type, corenrn.get_memb_func(type).sym, size);
            }
            if (receives_events) {
                fprintf(f, "%d nri %d\n", cix, pntindex);
                int k = nrn_i_layout(i, cnt, 1, psize, layout);
                Point_process* pp = (Point_process*) nt._vdata[ml->pdata[k]];
                pnt2index[pp] = pntindex;
                ++pntindex;
            }
            for (int j = 0; j < size; ++j) {
                int k = nrn_i_layout(i, cnt, j, size, layout);
                fprintf(f, " %d %d %.*g\n", cix, j, precision, ml->data[k]);
            }
        }
    }
}

static void pr_netcon(NrnThread& nt, FILE* f) {
    if (pntindex == 0) {
        return;
    }
    // pnt2index table has been filled

    // List of NetCon for each of the NET_RECEIVE point process instances
    // Also create the initial map of NetCon <-> DiscreteEvent (PreSyn)
    std::vector<std::vector<NetCon*>> nclist(pntindex);
    map_nc2src.clear();
    int nc_cnt = 0;
    for (int i = 0; i < nt.n_netcon; ++i) {
        NetCon* nc = nt.netcons + i;
        Point_process* pp = nc->target_;
        std::map<Point_process*, int>::iterator it = pnt2index.find(pp);
        if (it != pnt2index.end()) {
            nclist[it->second].push_back(nc);
            map_nc2src[nc] = nullptr;
            ++nc_cnt;
        }
    }
    fprintf(f, "netcons %d\n", nc_cnt);
    fprintf(f, " pntindex srcgid active delay weights\n");

    /// Fill the NetCon <-> DiscreteEvent map with PreSyn-s
    // presyns can come from any thread
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& ntps = nrn_threads[ith];
        for (int i = 0; i < ntps.n_presyn; ++i) {
            PreSyn* ps = ntps.presyns + i;
            for (int j = 0; j < ps->nc_cnt_; ++j) {
                NetCon* nc = netcon_in_presyn_order_[ps->nc_index_ + j];
                auto it_nc2src = map_nc2src.find(nc);
                if (it_nc2src != map_nc2src.end()) {
                    it_nc2src->second = ps;
                }
            }
        }
    }

    /// Fill the NetCon <-> DiscreteEvent map with InputPreSyn-s
    /// Traverse gid <-> InputPreSyn map and loop over NetCon-s of the
    /// correspondent InputPreSyn. If NetCon is in the nc2src map,
    /// remember its ips and the gid
    std::map<NetCon*, int> map_nc2gid;
    for (const auto& gid: gid2in) {
        InputPreSyn* ips = gid.second;  /// input presyn
        for (int i = 0; i < ips->nc_cnt_; ++i) {
            NetCon* nc = netcon_in_presyn_order_[ips->nc_index_ + i];
            auto it_nc2src = map_nc2src.find(nc);
            if (it_nc2src != map_nc2src.end()) {
                it_nc2src->second = ips;
                map_nc2gid[nc] = gid.first;  /// src gid of the input presyn
            }
        }
    }

    for (int i = 0; i < pntindex; ++i) {
        for (int j = 0; j < (int) (nclist[i].size()); ++j) {
            NetCon* nc = nclist[i][j];
            int srcgid = -3;
            auto it_nc2src = map_nc2src.find(nc);
            if (it_nc2src != map_nc2src.end()) {  // seems like there should be no NetCon which is
                                                  // not in the map
                DiscreteEvent* de = it_nc2src->second;
                if (de && de->type() == PreSynType) {
                    PreSyn* ps = (PreSyn*) de;
                    srcgid = ps->gid_;
                    Point_process* pnt = ps->pntsrc_;
                    if (srcgid < 0 && pnt) {
                        int type = pnt->_type;
                        fprintf(f,
                                "%d %s %d %.*g",
                                i,
                                corenrn.get_memb_func(type).sym,
                                nc->active_ ? 1 : 0,
                                precision,
                                nc->delay_);
                    } else if (srcgid < 0 && ps->thvar_index_ > 0) {
                        fprintf(
                            f, "%d %s %d %.*g", i, "v", nc->active_ ? 1 : 0, precision, nc->delay_);
                    } else {
                        fprintf(f,
                                "%d %d %d %.*g",
                                i,
                                srcgid,
                                nc->active_ ? 1 : 0,
                                precision,
                                nc->delay_);
                    }
                } else {
                    fprintf(f,
                            "%d %d %d %.*g",
                            i,
                            map_nc2gid[nc],
                            nc->active_ ? 1 : 0,
                            precision,
                            nc->delay_);
                }
            } else {
                fprintf(f, "%d %d %d %.*g", i, srcgid, nc->active_ ? 1 : 0, precision, nc->delay_);
            }
            int wcnt = corenrn.get_pnt_receive_size()[nc->target_->_type];
            for (int k = 0; k < wcnt; ++k) {
                fprintf(f, " %.*g", precision, nt.weights[nc->u.weight_index_ + k]);
            }
            fprintf(f, "\n");
        }
    }
    // cleanup
    nclist.clear();
}

static void pr_realcell(PreSyn& ps, NrnThread& nt, FILE* f) {
    // for associating NetCons with Point_process identifiers

    pntindex = 0;

    // threshold variable is a voltage
    printf("thvar_index_=%d end=%d\n", inv_permute(ps.thvar_index_, nt), nt.end);
    if (ps.thvar_index_ < 0 || ps.thvar_index_ >= nt.end) {
        hoc_execerror("gid not associated with a voltage", 0);
    }
    int inode = ps.thvar_index_;

    // and the root node is ...
    int rnode = inode;
    while (rnode >= nt.ncell) {
        rnode = nt._v_parent_index[rnode];
    }

    // count the number of nodes in the cell
    // do not assume all cell nodes except the root are contiguous
    // cellnodes is an unpermuted vector
    int* cellnodes = new int[nt.end];
    for (int i = 0; i < nt.end; ++i) {
        cellnodes[i] = -1;
    }
    int cnt = 0;
    cellnodes[inv_permute(rnode, nt)] = cnt++;
    for (int i = nt.ncell; i < nt.end; ++i) {  // think of it as unpermuted order
        if (cellnodes[inv_permute(nt._v_parent_index[permute(i, nt)], nt)] >= 0) {
            cellnodes[i] = cnt++;
        }
    }
    fprintf(f, "%d nodes  %d is the threshold node\n", cnt, cellnodes[inv_permute(inode, nt)] - 1);
    fprintf(f, " threshold %.*g\n", precision, ps.threshold_);
    fprintf(f, "inode parent area a b\n");
    for (int iorig = 0; iorig < nt.end; ++iorig)
        if (cellnodes[iorig] >= 0) {
            int i = permute(iorig, nt);
            int ip = nt._v_parent_index[i];
            fprintf(f,
                    "%d %d %.*g %.*g %.*g\n",
                    cellnodes[iorig],
                    ip >= 0 ? cellnodes[inv_permute(ip, nt)] : -1,
                    precision,
                    nt._actual_area[i],
                    precision,
                    nt._actual_a[i],
                    precision,
                    nt._actual_b[i]);
        }
    fprintf(f, "inode v\n");
    for (int i = 0; i < nt.end; ++i)
        if (cellnodes[i] >= 0) {
            fprintf(f, "%d %.*g\n", cellnodes[i], precision, nt._actual_v[permute(i, nt)]);
        }

    // each mechanism
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        pr_memb(tml->index, tml->ml, cellnodes, nt, f);
    }

    // the NetCon info (uses pnt2index)
    pr_netcon(nt, f);

    delete[] cellnodes;
    pnt2index.clear();
    if (inv_permute_) {
        delete inv_permute_;
        inv_permute_ = nullptr;
    }
}

int prcellstate(int gid, const char* suffix) {
    // search the NrnThread.presyns for the gid
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        for (int ip = 0; ip < nt.n_presyn; ++ip) {
            PreSyn& ps = nt.presyns[ip];
            if (ps.output_index_ == gid) {
                // found it so create a <gid>_<suffix>.corenrn file
                std::string filename = std::to_string(gid) + "_" + suffix + ".corenrn";
                FILE* f = fopen(filename.c_str(), "w");
                assert(f);
                fprintf(f, "gid = %d\n", gid);
                fprintf(f, "t = %.*g\n", precision, nt._t);
                fprintf(f, "celsius = %.*g\n", precision, celsius);
                if (ps.thvar_index_ >= 0) {
                    pr_realcell(ps, nt, f);
                }
                fclose(f);
                return 1;
            }
        }
    }
    return 0;
}
}  // namespace coreneuron
