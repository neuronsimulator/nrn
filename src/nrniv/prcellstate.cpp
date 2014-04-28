#include "../../nrnconf.h"
#include "section.h"
#include "membfunc.h"
#include "nrniv_mf.h"
#include "netcon.h"
#include "OS/table.h"
#include "OS/list.h"

extern "C" {
void nrn_prcellstate(int gid, const char* filesuffix);
}

declarePtrList(NetConList, NetCon) // NetCons in same order as Point_process
implementPtrList(NetConList, NetCon) // and there may be several per pp.
declareTable(PV2I, void*, int)
implementTable(PV2I, void*, int)
static PV2I* pnt2index; // for deciding if NetCon is to be printed
static int pntindex; // running count of printed point processes.

static void pr_memb(int type, Memb_list* ml, int* cellnodes, NrnThread& nt, FILE* f) {
  int header_printed = 0;
  int size = nrn_prop_param_size_[type];
  int psize = nrn_prop_dparam_size_[type];
  int receives_events = pnt_receive[type] ? 1 : 0;
  for (int i = 0; i < ml->nodecount; ++i) {
    int inode = ml->nodeindices[i];
    if (cellnodes[inode] >= 0) {
      if (!header_printed) {
        header_printed = 1;
        fprintf(f, "type=%d %s size=%d\n", type, memb_func[type].sym->name, size);
      }
      if (receives_events) {
        fprintf(f, "%d nri %d\n", cellnodes[inode], pntindex);
        Point_process* pp = (Point_process*)ml->pdata[i][1]._pvoid;
        pnt2index->insert(pp, pntindex);
        ++pntindex;
      }
      for (int j=0; j < size; ++j) {
        fprintf(f, " %d %d %.15g\n", cellnodes[inode], j, ml->data[i][j]);
      }
    }
  }
}

static void pr_netcon(NrnThread& nt, FILE* f) {
  if (pntindex == 0) { return; }
  // pnt2index table has been filled

  // List of NetCon for each of the NET_RECEIVE point process instances
  // ... all NetCon list in the hoc NetCon cTemplate 
  NetConList** nclist = new NetConList*[pntindex];
  for (int i=0; i < pntindex; ++i) { nclist[i] = new NetConList(1); }
  int nc_cnt = 0;
  Symbol* ncsym = hoc_lookup("NetCon");
  hoc_Item* q;
  ITERATE (q, ncsym->u.ctemplate->olist) {
    Object* obj = OBJ(q);
    NetCon* nc = (NetCon*)obj->u.this_pointer;
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
      srcgid = (nc->src_) ? nc->src_->gid_ : -3;
      if (srcgid < 0 && nc->src_ && nc->src_->osrc_) {
        const char* name = nc->src_->osrc_->ctemplate->sym->name;
        fprintf(f, "%d %s %d %.15g", i, name, nc->active_?1:0, nc->delay_);
      }else if (srcgid < 0 && nc->src_ && nc->src_->ssrc_) {
        fprintf(f, "%d %s %d %.15g", i, "v", nc->active_?1:0, nc->delay_);
      }else{
        fprintf(f, "%d %d %d %.15g", i, srcgid, nc->active_?1:0, nc->delay_);
      }
      int wcnt = pnt_receive_size[nc->target_->prop->type];
      for (int k=0; k < wcnt; ++k) {
        fprintf(f, " %.15g", nc->weight_[k]);
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
  int inode = -1;
  if (ps.thvar_ < nt._actual_v || ps.thvar_ >= (nt._actual_v + nt.end)) {
    if (ps.ssrc_) { /* not cache efficient, search the nodes in this section */
printf("%s\n", ps.ssrc_?secname(ps.ssrc_):"unknown");
      for (int i=0; i < ps.ssrc_->nnode; ++i) {
        Node* nd = ps.ssrc_->pnode[i];
        if (ps.thvar_ == nd->_v) {
          inode = nd->v_node_index;
          break;
        }
      }
      if (inode < 0) { /* check parent node */
        Node* nd = ps.ssrc_->parentnode;
        if (ps.thvar_ == nd->_v) {
          inode = nd->v_node_index;
        }
      }
    }
    if (inode < 0) {
      hoc_execerror("gid not associated with a voltage", 0);
    }
  }else{
    inode = ps.thvar_ - nt._actual_v;
  }

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
    Node* nd = nt._v_node[i]; //if not cach_efficient then _actual_area=NULL
    fprintf(f, "%d %d %.15g %.15g %.15g\n",
      cellnodes[i], cellnodes[nt._v_parent_index[i]],
      NODEAREA(nd), nt._actual_a[i], nt._actual_b[i]);
  }
  fprintf(f, "inode v\n");
  for (int i=0; i < nt.end; ++i) if (cellnodes[i] >= 0) {
    Node* nd = nt._v_node[i]; //if not cach_efficient then _actual_v=NULL
    fprintf(f, "%d %.15g\n",
     cellnodes[i], NODEV(nd));
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

void nrn_prcellstate(int gid, const char* suffix) {
  PreSyn* ps = nrn_gid2outputpresyn(gid);
  if (!ps) { return; }
  // found it so create a <gid>_<suffix>.nrn file
  char buf[200];
  sprintf(buf, "%d_%s.nrndat", gid, suffix);
  FILE* f = fopen(buf, "w");
  assert(f);
  NrnThread& nt = *ps->nt_;
  fprintf(f, "gid = %d\n", gid);
  fprintf(f, "t = %.15g\n", nt._t);
  if (ps->thvar_) {
    pr_realcell(*ps, nt, f);
  }
  fclose(f);
}

