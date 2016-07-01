/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>
#include <map>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/membfunc.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/utils/sdprintf.h"
#include "coreneuron/nrniv/nrniv_decl.h"

std::map<Point_process*, int> pnt2index; // for deciding if NetCon is to be printed
static int pntindex; // running count of printed point processes.
std::map<NetCon*, DiscreteEvent*> map_nc2src;

static void pr_memb(int type, Memb_list* ml, int* cellnodes, NrnThread& nt, FILE* f) {
  int is_art = nrn_is_artificial_[type];
  if (is_art)
    return;

  int header_printed = 0;
  int size = nrn_prop_param_size_[type];
  int psize = nrn_prop_dparam_size_[type];
  int receives_events = pnt_receive[type] ? 1 : 0;
  int layout = nrn_mech_data_layout_[type];
  int cnt = ml->nodecount;
  for (int i = 0; i < ml->nodecount; ++i) {
    int inode = ml->nodeindices[i];
    if (cellnodes[inode] >= 0) {
      if (!header_printed) {
        header_printed = 1;
        fprintf(f, "type=%d %s size=%d\n", type, memb_func[type].sym, size);
      }
      if (receives_events) {
        fprintf(f, "%d nri %d\n", cellnodes[inode], pntindex);
        int k = nrn_i_layout(i, cnt, 1, psize, layout);
        Point_process* pp = (Point_process*)nt._vdata[ml->pdata[k]];
        pnt2index[pp] = pntindex;
        ++pntindex;
      }
      for (int j=0; j < size; ++j) {
        int k = nrn_i_layout(i, cnt, j, size, layout);
        fprintf(f, " %d %d %.15g\n", cellnodes[inode], j, ml->data[k]);
      }
    }
  }
}

static void pr_netcon(NrnThread& nt, FILE* f) {
  if (pntindex == 0) { return; }
  // pnt2index table has been filled

  // List of NetCon for each of the NET_RECEIVE point process instances
  // Also create the initial map of NetCon <-> DiscreteEvent (PreSyn)
  std::vector< std::vector<NetCon*> > nclist;
  nclist.resize(pntindex);
  map_nc2src.clear();
  int nc_cnt = 0;
  for (int i=0; i < nt.n_netcon; ++i) {
    NetCon* nc = nt.netcons + i;
    Point_process* pp = nc->target_;
    std::map<Point_process*, int>::iterator it = pnt2index.find(pp);
    if (it != pnt2index.end()) {
      nclist[it->second].push_back(nc);
      map_nc2src[nc] = NULL;
      ++nc_cnt;
    }
  }
  fprintf(f, "netcons %d\n", nc_cnt);
  fprintf(f, " pntindex srcgid active delay weights\n");

    /// Fill the NetCon <-> DiscreteEvent map with PreSyn-s
  DiscreteEvent* de;
  std::map<NetCon*, DiscreteEvent*>::iterator it_nc2src;
  // presyns can come from any thread
  for ( int ith = 0; ith < nrn_nthread; ++ith) {
    NrnThread& ntps = nrn_threads[ith];
    for (int i = 0; i < ntps.n_presyn; ++i) {
      PreSyn* ps = ntps.presyns + i;
      for (int j = 0; j < ps->nc_cnt_; ++j) {
        NetCon* nc = netcon_in_presyn_order_[ps->nc_index_ + j];
        it_nc2src = map_nc2src.find(nc);
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
  std::map<int, InputPreSyn*>::iterator it_gid2in = gid2in.begin();
  for (; it_gid2in != gid2in.end(); ++it_gid2in) {
      InputPreSyn* ips = it_gid2in->second;                           /// input presyn
      for (int i = 0; i < ips->nc_cnt_; ++i) {
          NetCon* nc = netcon_in_presyn_order_[ips->nc_index_ + i];
          it_nc2src = map_nc2src.find(nc);
          if (it_nc2src != map_nc2src.end()) {
              it_nc2src->second = ips;
              map_nc2gid[nc] = it_gid2in->first;                     /// src gid of the input presyn
          }
      }
  }

  for (int i=0; i < pntindex; ++i) {
    for (int j=0; j < (int)(nclist[i].size()); ++j) {
      NetCon* nc = nclist[i][j];
      int srcgid = -3;
      it_nc2src = map_nc2src.find(nc);
      if (it_nc2src != map_nc2src.end()) { // seems like there should be no NetCon which is not in the map
        de = it_nc2src->second;
        if (de && de->type() == PreSynType) {
          PreSyn* ps = (PreSyn*)de;
          srcgid = ps->gid_;
          Point_process* pnt = ps->pntsrc_;
          if (srcgid < 0 && pnt) {
            int type = pnt->_type;
            fprintf(f, "%d %s %d %.15g", i, memb_func[type].sym, nc->active_?1:0, nc->delay_);
          }else if (srcgid < 0 && ps->thvar_) {
            fprintf(f, "%d %s %d %.15g", i, "v", nc->active_?1:0, nc->delay_);
          }else{
            fprintf(f, "%d %d %d %.15g", i, srcgid, nc->active_?1:0, nc->delay_);
          }
        }else{
          fprintf(f, "%d %d %d %.15g", i, map_nc2gid[nc], nc->active_?1:0, nc->delay_);
        }
      }else{
        fprintf(f, "%d %d %d %.15g", i, srcgid, nc->active_?1:0, nc->delay_);
      }
      int wcnt = pnt_receive_size[nc->target_->_type];
      for (int k=0; k < wcnt; ++k) {
        fprintf(f, " %.15g", nc->weight_[k]);
      }
      fprintf(f, "\n");
    }
  }
  // cleanup
  nclist.clear();
}

static void pr_realcell(PreSyn& ps, NrnThread& nt, FILE* f) {
  //for associating NetCons with Point_process identifiers

  pntindex = 0;

  // threshold variable is a voltage
printf("thvar=%p actual_v=%p end=%p\n", ps.thvar_, nt._actual_v,
nt._actual_v + nt.end);
  if (ps.thvar_ < nt._actual_v || ps.thvar_ >= (nt._actual_v + nt.end)) {
    hoc_execerror("gid not associated with a voltage", 0);
  }
  int inode = ps.thvar_ - nt._actual_v;

  // and the root node is ...
  int rnode = inode;
  while (rnode >= nt.ncell) {
    rnode = nt._v_parent_index[rnode];
  }

  // count the number of nodes in the cell
  // do not assume all cell nodes except the root are contiguous
  int* cellnodes = new int[nt.end];
  for (int i=0; i < nt.end; ++i) { cellnodes[i] = -1; }
  int cnt = 0;
  cellnodes[rnode] = cnt++;
  for (int i=nt.ncell; i < nt.end; ++i) {
    if (cellnodes[nt._v_parent_index[i]] >= 0) {
      cellnodes[i] = cnt++;
    }
  }
  fprintf(f, "%d nodes  %d is the threshold node\n", cnt, cellnodes[inode]-1);
  fprintf(f, " threshold %.15g\n", ps.threshold_);
  fprintf(f, "inode parent area a b\n");
  for (int i=0; i < nt.end; ++i) if (cellnodes[i] >= 0) {
    fprintf(f, "%d %d %.15g %.15g %.15g\n",
      cellnodes[i], cellnodes[nt._v_parent_index[i]],
      nt._actual_area[i], nt._actual_a[i], nt._actual_b[i]);
  }
  fprintf(f, "inode v\n");
  for (int i=0; i < nt.end; ++i) if (cellnodes[i] >= 0) {
    fprintf(f, "%d %.15g\n",
     cellnodes[i], nt._actual_v[i]);
  }
  
  // each mechanism
  for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
    pr_memb(tml->index, tml->ml, cellnodes, nt, f);
  }

  // the NetCon info (uses pnt2index)
  pr_netcon(nt, f);

  delete [] cellnodes;
  pnt2index.clear();
}

int prcellstate(int gid, const char* suffix) {
  //search the NrnThread.presyns for the gid
  for (int ith=0; ith < nrn_nthread; ++ith) {
    NrnThread& nt = nrn_threads[ith];
    for (int ip = 0; ip < nt.n_presyn; ++ip) {
      PreSyn& ps = nt.presyns[ip];
      if (ps.output_index_ == gid) {
        // found it so create a <gid>_<suffix>.corenrn file
        char buf[200];
        sd_ptr filename=sdprintf(buf, sizeof(buf), "%d_%s.corenrn", gid, suffix);
        FILE* f = fopen(filename, "w");
        assert(f);
        fprintf(f, "gid = %d\n", gid);
        fprintf(f, "t = %.15g\n", nt._t);
        fprintf(f, "celsius = %.15g\n", celsius);
        if (ps.thvar_) {
          pr_realcell(ps, nt, f);
        }
        fclose(f);
        return 1;
      }
    }
  }
  return 0;
}

