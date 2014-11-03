/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/membfunc.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrniv/netcon.h"
#include "corebluron/nrniv/ivtable.h"
#include "corebluron/nrniv/ivlist.h"
#include "corebluron/nrnoc/nrnoc_decl.h"

declarePtrList(NetConList, NetCon) // NetCons in same order as Point_process
implementPtrList(NetConList, NetCon) // and there may be several per pp.
declareTable(PV2I, void*, int)
implementTable(PV2I, void*, int)
static PV2I* pnt2index; // for deciding if NetCon is to be printed
static int pntindex; // running count of printed point processes.

static void pr_memb(int type, Memb_list* ml, int* cellnodes, NrnThread& nt, FILE* f) {
  int is_art = nrn_is_artificial_[type];
  if (is_art)
    return;

  int header_printed = 0;
  int size = nrn_prop_param_size_[type];
  int psize = nrn_prop_dparam_size_[type];
  int receives_events = pnt_receive[type] ? 1 : 0;
  for (int i = 0; i < ml->nodecount; ++i) {
    int inode = ml->nodeindices[i];
    if (cellnodes[inode] >= 0) {
      if (!header_printed) {
        header_printed = 1;
        fprintf(f, "type=%d %s size=%d\n", type, memb_func[type].sym, size);
      }
      if (receives_events) {
        fprintf(f, "%d nri %d\n", cellnodes[inode], pntindex);
        Point_process* pp = (Point_process*)nt._vdata[ml->pdata[i*psize + 1]];
        pnt2index->insert(pp, pntindex);
        ++pntindex;
      }
      for (int j=0; j < size; ++j) {
        fprintf(f, " %d %d %.15g\n", cellnodes[inode], j, ml->data[i*size+j]);
      }
    }
  }
}

static void pr_netcon(NrnThread& nt, FILE* f) {
  if (pntindex == 0) { return; }
  // pnt2index table has been filled

  // List of NetCon for each of the NET_RECEIVE point process instances
  NetConList** nclist = new NetConList*[pntindex];
  for (int i=0; i < pntindex; ++i) { nclist[i] = new NetConList(1); }
  int nc_cnt = 0;
  for (int i=0; i < nt.n_netcon; ++i) {
    NetCon* nc = nt.netcons + i;
    Point_process* pp = nc->target_;
    int index;
    if (pnt2index->find(index, pp)) {
      nclist[index]->append(nc);
      ++nc_cnt;
    }
  }
  fprintf(f, "netcons %d\n", nc_cnt);
  fprintf(f, " pntindex srcgid active delay weights\n");
  for (int i=0; i < pntindex; ++i) {
    for (int j=0; j < nclist[i]->count(); ++j) {
      NetCon* nc = nclist[i]->item(j);
      int srcgid = -3;
      if (nc->src_) {
        if (nc->src_->type() == PreSynType) {
          PreSyn* ps = (PreSyn*)nc->src_;
          srcgid = ps->gid_;
          if (srcgid < 0 && ps->pntsrc_) {
            int type = ps->pntsrc_->_type;
            fprintf(f, "%d %s %d %.15g", i, memb_func[type].sym, nc->active_?1:0, nc->delay_);
          }else if (srcgid < 0 && ps->thvar_) {
            fprintf(f, "%d %s %d %.15g", i, "v", nc->active_?1:0, nc->delay_);
          }else{
            fprintf(f, "%d %d %d %.15g", i, srcgid, nc->active_?1:0, nc->delay_);
          }
        }else{
          srcgid = ((InputPreSyn*)nc->src_)->gid_;
          fprintf(f, "%d %d %d %.15g", i, srcgid, nc->active_?1:0, nc->delay_);
        }
      }else{
        fprintf(f, "%d %d %d %.15g", i, srcgid, nc->active_?1:0, nc->delay_);
      }
      int wcnt = pnt_receive_size[nc->target_->_type];
      for (int k=0; k < wcnt; ++k) {
        fprintf(f, " %.15g", nc->u.weight_[k]);
      }
      fprintf(f, "\n");
    }
  }
  // cleanup
  for (int i=0; i < pntindex; ++i) { delete nclist[i]; }
  delete [] nclist;
}

static void pr_realcell(PreSyn& ps, NrnThread& nt, FILE* f) {
  //for associating NetCons with Point_process identifiers
  pnt2index = new PV2I(1000);

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
  delete pnt2index;
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
        sprintf(buf, "%d_%s.corenrn", gid, suffix);
        FILE* f = fopen(buf, "w");
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

