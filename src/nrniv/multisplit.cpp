#include <../../nrnconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <InterViews/resource.h>
#include <vector>
#include "nrn_ansi.h"
#include "nrndae_c.h"
#include "nrniv_mf.h"
#include <nrnoc2iv.h>
#include <nrnmpi.h>
#include <multisplit.h>
#include <unordered_map>
#include <memory>

void nrnmpi_multisplit(Section*, double x, int sid, int backbone_style);
int nrn_multisplit_active_;

extern void setup_topology();
extern void (*nrn_multisplit_setup_)();
extern void* nrn_multisplit_triang(NrnThread*);
extern void* nrn_multisplit_reduce_solve(NrnThread*);
extern void* nrn_multisplit_bksub(NrnThread*);
extern double t;

void nrn_multisplit_ptr_update();
void nrnmpi_multisplit_clear();
void nrn_multisplit_nocap_v();
void nrn_multisplit_nocap_v_part1(NrnThread*);
void nrn_multisplit_nocap_v_part2(NrnThread*);
void nrn_multisplit_nocap_v_part3(NrnThread*);
void nrn_multisplit_adjust_rhs(NrnThread*);
extern void (*nrn_multisplit_solve_)();
static void multisplit_v_setup();
static void multisplit_solve();

extern double nrnmpi_rtcomp_time_;
#if NRNMPI
extern double nrnmpi_splitcell_wait_;
#else
static double nrnmpi_splitcell_wait_;
#endif

#if NRNMPI
#else
static int nrnmpi_use;
static void nrnmpi_int_allgather(int*, int*, int) {}
static void nrnmpi_int_allgatherv(int*, int*, int*, int*) {}
static void nrnmpi_postrecv_doubles(double*, int, int, int, void**) {}
static void nrnmpi_send_doubles(double*, int, int, int) {}
static void nrnmpi_wait(void**) {}
static void nrnmpi_barrier() {}
static double nrnmpi_wtime() {
    return 0.0;
}
#endif

class MultiSplit;
class MultiSplitControl;

#define D(i)   vec_d[i]
#define RHS(i) vec_rhs[i]
#define S1A(i) sid1A[i]
#define S1B(i) sid1B[i]

// any number of nodes can have the same sid (generally those nodes
// will be on different machines).
// A node cannot have more than one sid.
// A tree cannot have more than two nodes with an sid and those nodes
// must be distinct.

// NOTE:
// For cache efficiency, it would be nice to keep individual cell nodes
// together (i.e v_node from 0 to rootnodecount-1 are NOT the root nodes.)
// This would allow keeping the backbones contiguous and would simplify the
// solution of the sid0,1 2x2 at the end of triangularization that forms
// an N shaped matrix. Unfortunately, in several places, e.g. matrix setup,
// the first rootnodecount v_node are assumed to be the individual cell roots
// and at this time it would be confusing to work on that tangential issue.
// However, it does mean that we need some special structure since
// sid0 ...backbone interior nodes... sid1 are not contiguous in the v_node
// for a single cell.
// Presently the node order is
// 1) all the roots of cells with 0 or 1 sid (no backbone involved)
// 2) all the sid0 when there are two sids in a cell. 1) + 2) = rootnodecount
// 3) the interior backbone nodes (does not include sid0 or sid1 nodes)
// 4) all the sid1 nodes
// 5) all remaining nodes
// backbone_begin is the index for 2).
// backbone_interior_begin is one more than the last index for 2)
// backbone_sid1_begin is the index for 4)
// backbone_end is the index for 5.

// Modification to handle short backbones
// Numerical accuracy/stability suffers when backbones are electrically
// short. Unfortunately, it is impossible to divide many 3-d reconstructed
// cells with any kind of load balance without using them. Also,
// unfortunately, it is probably not possible to solve the problem
// in general. However we can extend, without introducing
// too much parallel inefficiency, the implementation to allow an
// arbitrarily short backbone treated as a single node, as long as it
// does not connect to a short backbone on another machine. e.g we can
// have alternating short and long sausages but not a sequence of
// contiguous short sausages.  The idea is to exchange backbone info
// in two stages. First receive the info that short backbones need
// (by definition only from long backbones), compute the exact D,RHS
// backbone end points (only a 2x2 matrix solve), and then send the
// exact short backbone info.  matrix_exchange carries out the
// augmented communication. But we need to distinguish short from
// long backbones since for short ones we need to factor out the
// first part of bksub_backbone and do it in the middle of
// matrix_exchange (as well as avoid doing that twice in bksub_backbone).
// We can accomplish that by introducing the index
// backbone_long_begin between the backbone_begin and backbone_interior_begin
// indices. Then in matrix exchange we do backbone_begin to
// backbone_long_begin - 1 , and in bksub_backbone, the first part is
// from backbone_long_begin to backbone_interior_begin - 1.

// Modification to handle any number of backbones, short or long, exactly.
// In this case, it is not necessary to consider numerical stability/accuracy.
// However a tree matrix with rank equal to the number of distinct sid for
// an entire parallelized cell must be solved between the send and receive
// stages for that cell. The main issue to be resolved is: which machine receives
// the matrix information. ie. a single sid subtree sends d and rhs to the
// (rthost)machine that gaussian eliminates the reduced tree
// matrix and a two sid backbone subtree sends the two d, a, b, and two rhs
// to rthost. After rthost solves the matrix (triangualize and back substitute)
// the answer for each sid is sent back to the machines needing it.
// Clearly, some savings results if rthost happens to be one of the machines
// with a 2 sid backbone. Also, it is probably best if a machine serves
// as rthost to as few cells as possible.

// Modification to handle multiple subtrees with same sid on same machine.
// This is very valuable for load balancing. It demands a backbone style
// of 2 (using reduced tree matrices) and we allow user specified host
// for the reduced tree. The exact solution using reduced trees has
// proved so much more useful than the long/short backbone methods
// that we should consider removing the code
// for those others and implement only the reduced tree method.

// Area handling prior to ReducedTree extension was and mostly still is:
// 1) sending long to short, long, reduced on the send side multiplies
// the send buffer (D and RHS components) by area (if non-zero area node)
// from 0 to iarea_short_long using area_node_indices and buf_area_indices.
// 2) receive what is needed by short backbone (and reduced tree) and the
// receive buffer for short backbong divides by area from iarea_short_long
// to narea using aread_node_indices and buf_area_indices. (not needed by
// reduced tree but the local D and RHS will have to be multiplied by the
// node area when filling the ReducedTree. Also the sid1A and sid1B terms
// will have to be area multiplied both on the ReducedTree host
//  and when backbones are sent to the ReducedTree host
// 3) The short backbone is solved consistent with its original node area
// (and the ReducedTree is solved with all its equation having been multiplied
// by area.
// 4) Note that solving the short and reduced tree gives the answer which
// does not have to be scaled but we multiply the D and RHS by 1e30 so
// those equation are v = RHS/D regardless of area or coupling. So no need
// to divide by area on the short or reduced tree machines and it would not
// matter if we did so or not.
// 5) all remaining receives take place and what is received gets divided
// by the node area (0 to iarea_short_long). As mentioned above, this does
// not affect the result that was sent from the short or reduced tree
// host (since D and RHS were scaled by 1e30).
// So... Anything sent to reduced trees from another machine needs to get its
// info multiplied by the node area. This is sid0 D and RHS and if there is
// a sid1 then that D and RHS as well as sid1A and sid1B.
// Also... Anything rmapped into a reduced tree from the reduced tree machine
// needs to get its rmap info multiplied by the node area ( in fact we can
// do that in place before calling fillrmap since those values get the result
// *1e30 returned into them).
// We do the old methods (backbone style 0 and 1) completely segregated from
// the new reducedtree method by introducing a vector parallel to
// nodeindex_buffer_ called nodeindex_rthost_ that contains the hostid for the
// reduced tree host associated with that sid. Then -1 means old style and
// >= 0 means reducedtree style.

// the implementation so far has turned out to be too brittle with regard
// to unimportant topology changes in the sense of new point processes or
// anything that ends up calling multisplit_v_setup. The experienced
// cause of this is because MultiSplitTransferInfo holds pointers into
// the sid1A and sid1B off diag elements and those arrays are freed
// and reallocated when multisplit_v_setup is called. Although
// exchange_setup cries out for a simplified and
// more efficient implementation, for now, opportunistically reset the
// pointers in the MultiSplitTransferInfo at the end multisplit_v_setup


struct Area2Buf {
    int inode;
    int n;               // using only for transfer to ReducedTree on another
    int ibuf[3];         // machine so n is 2 or 3
    double adjust_rhs_;  // for cvode
    MultiSplit* ms;      // for recalculating ibuf pointers after v_setup
};
struct Area2RT {
    int inode;
    int n;  // 2 or 3
    double* pd[3];
    double adjust_rhs_;  // for cvode
    MultiSplit* ms;      // for recalculating pd pointers after v_setup
};

using Int2IntTable = std::unordered_map<int, int>;

// d and rhs keep getting reorderd according to thread cache efficiency
// so we need to retain some logical info in order to update the
// pointer lists in the Reduced tree. Prior to this comment, this was
// done only for sid1A and sid1B near the end of v_setup. That is ok
// since sid1A and sid1B are allocated by v_setup. But d and rhs are
// filled into the Node* after return from v_setup.

class ReducedTree {
  public:
    ReducedTree(MultiSplitControl*, int rank, int mapsize);
    virtual ~ReducedTree();
    void solve();
    void nocap();
    // private:
  public:
    void gather();
    void scatter();

    MultiSplitControl* msc;
    int n;
    int* ip;
    double* rhs;
    double* d;
    double* a;
    double* b;
    int n2, n4, nmap;

  public:
    // next implementation, exploit the fact that send buffer can be
    // smaller than the receive buffer since we only need to send back
    // the answer in RHS. Now we are sending back RHS, D,
    // and even the offdiags.
    double **smap, **rmap;
    int *ismap, *irmap;
    int nsmap, irfill;

    int* rmap2smap_index;  // needed for nocap when cvode used
    int* nzindex;          // needed for nocap when cvode used
    double* v;             // needed for nocap when cvode used

    void reorder(int j, int nt, int* mark, int* all_bb_relation, int* allsid);
    std::unique_ptr<Int2IntTable> s2rt{new Int2IntTable()};  // sid2rank table
    void fillrmap(int sid1, int sid2, double* pd);
    void fillsmap(int sid, double* prhs, double* pdiag);
    void pr_map(int, double*);
};

class MultiSplit {
  public:
    Node* nd[2];
    int sid[2];
    int backbone_style;  // 0 = no, 1 = yes, 2 = reducedtree (exact)
    int rthost;          // nrnmpi_myid where reducedtree will be solved
    // the problem with using backsid_ to find the index now is that
    // the value may not be unique since several backbones can be
    // connected at the same sid on this machine. So store the index into
    // the backsid) array.
    int back_index;
    // and need the thread id as well
    int ithread;
    // reduced tree pointers to d and rhs can become invalid and
    // need to be restored. According to the exchange_setup that calls
    // ReducedTree::fillrmap, the order is nd[0]->rhs, nd[0]->d, nd[1]->rhs
    // nd[1]->d.
    ReducedTree* rt_;
    int rmap_index_;
    int smap_index_;
};

// note: the tag_ below was added in case this machine interacts with
// another via more than one message type. i.e combinations of  short<->long,
// reduced<->long, and long<->long backbone interactions.
// The user should avoid this
// eventuality and in fact we might not allow it.

// mpi transfer information
struct MultiSplitTransferInfo {
    int host_;           // host id for send-receive
    int nnode_;          // number of nodes involved
    int* nodeindex_;     // v_node indices of those nodes. Pointer into nodeindex_buffer_
    int* nodeindex_th_;  // thread for above
    int nnode_rt_;       // number of off diag elements involved
    int* nd_rt_index_;   // v_node indices of offdiag. Not pointer into nodeindex_buffer. Needed for
                         // area2buf
    int* nd_rt_index_th_;  // thread for above
    double** offdiag_;     // pointers to sid1A or sid1B off diag elements
    int* ioffdiag_;        // indices of above, to recalculate offdiag when freed
    int size_;             // 2*nnode_ + nnode_rt_ doubles needed in buffer
    int displ_;            // displacement into trecvbuf_
    void* request_;        // MPI_Request
    int tag_;              // short<->long, long<->long, subtree->rthost, rthost->subtree
    int rthost_;           // host id where the reduced tree is located (normally -1)
};

using MultiSplitTable = std::unordered_map<Node*, MultiSplit*>;
using MultiSplitList = std::vector<MultiSplit*>;

#include <multisplitcontrol.h>
static MultiSplitControl* msc_;

void nrnmpi_multisplit(Section* sec, double x, int sid, int backbone_style) {
    if (!msc_) {
        msc_ = new MultiSplitControl();
    }
    msc_->multisplit(sec, x, sid, backbone_style);
}

MultiSplitControl::MultiSplitControl() {
    narea2buf_ = narea2rt_ = 0;
    area2buf_ = 0;
    area2rt_ = 0;
    nthost_ = 0;
    ihost_reduced_long_ = ihost_short_long_ = 0;
    msti_ = 0;
    tbsize = 0;
    ndbsize = 0;
    trecvbuf_ = 0;
    tsendbuf_ = 0;
    nodeindex_buffer_ = 0;
    nodeindex_buffer_th_ = 0;
    nodeindex_rthost_ = 0;
    narea_ = 0;
    iarea_short_long_ = 0;
    buf_area_indices_ = 0;
    area_node_indices_ = 0;

    nrtree_ = 0;
    rtree_ = 0;


    multisplit_list_ = 0;
    nth_ = 0;
    mth_ = 0;
}

MultiSplitControl::~MultiSplitControl() {}

MultiSplitThread::MultiSplitThread() {
    backbone_begin = backbone_long_begin = backbone_interior_begin = 0;
    backbone_sid1_begin = backbone_long_sid1_begin = backbone_end = 0;
    sid1A = sid1B = 0;
    sid0i = 0;

    nbackrt_ = 0;
    backsid_ = 0;
    backAindex_ = 0;
    backBindex_ = 0;
    i1 = i2 = i3 = 0;
}

MultiSplitThread::~MultiSplitThread() {
    del_sidA();
}

void MultiSplitControl::multisplit(Section* sec, double x, int sid, int backbone_style) {
#if 0
	if (sid > 1000) { pexch(); return; }
	if (sid >= 1000) { pmat(sid>1000); return; }
#endif
    if (sid < 0) {
        if (classical_root_to_multisplit_) {
            nrn_multisplit_setup_ = multisplit_v_setup;
            nrn_multisplit_solve_ = multisplit_solve;
            nrn_matrix_node_free();
        }
        exchange_setup();
        return;
    }
    nrn_multisplit_active_ = 1;
    if (backbone_style != 2) {
        hoc_execerror("only backbone_style 2 is now supported", 0);
    }
    if (!classical_root_to_multisplit_) {
        classical_root_to_multisplit_.reset(new MultiSplitTable());
        classical_root_to_multisplit_->reserve(97);
        multisplit_list_ = new MultiSplitList;
    }
    Node* nd = node_exact(sec, x);
    if (tree_changed) {
        setup_topology();
    }
    //	printf("root of %s(%g) ", secname(sec), x);
    Node* root;
    for (sec = nd->sec; sec; sec = sec->parentsec) {
        root = sec->parentnode;
    }
    assert(root);
    //	printf("is %s\n", secname(root->sec));
    MultiSplit* ms;
    const auto& msiter = classical_root_to_multisplit_->find(root);
    if (msiter != classical_root_to_multisplit_->end()) {
        ms = msiter->second;
        if (backbone_style == 2) {
            if (ms->backbone_style != 2) {
                hoc_execerror("earlier call for this cell did not have a backbone style = 2", 0);
            }
        } else if (backbone_style == 1) {
            ms->backbone_style = 1;
        }
        ms->nd[1] = nd;
        ms->sid[1] = sid;
        if (ms->sid[1] == ms->sid[0]) {
            char s[100];
            Sprintf(s, "two sid = %d at same point on tree rooted at", sid);
            hoc_execerror(s, secname(root->sec));
        }
    } else {
        ms = new MultiSplit();
        ms->backbone_style = backbone_style;
        ms->rthost = -1;
        ms->nd[0] = nd;
        ms->nd[1] = 0;
        ms->sid[0] = sid;
        ms->sid[1] = -1;
        ms->back_index = -1;
        ms->ithread = -1;
        ms->rt_ = 0;
        ms->rmap_index_ = -1;
        ms->smap_index_ = -1;
        (*classical_root_to_multisplit_)[root] = ms;
        multisplit_list_->push_back(ms);
    }
}

void MultiSplitThread::del_sidA() {
    if (sid1A) {
        delete[] sid1A;
        delete[] sid1B;
        delete[] sid0i;
        sid1A = 0;
        sid1B = 0;
        sid0i = 0;
    }
    if (nbackrt_) {
        delete[] backsid_;
        delete[] backAindex_;
        delete[] backBindex_;
        nbackrt_ = 0;
    }
}

void MultiSplitControl::del_msti() {
    int i;
    if (nrtree_) {
        for (i = 0; i < nrtree_; ++i) {
            delete rtree_[i];
        }
        delete[] rtree_;
        nrtree_ = 0;
    }
    if (msti_) {
        for (i = 0; i < nthost_; ++i) {
            if (msti_[i].nnode_rt_) {
                delete[] msti_[i].nd_rt_index_;
                delete[] msti_[i].nd_rt_index_th_;
                delete[] msti_[i].offdiag_;
                delete[] msti_[i].ioffdiag_;
            }
        }
        delete[] msti_;
        msti_ = 0;
        if (nodeindex_buffer_) {
            delete[] nodeindex_buffer_;
            delete[] nodeindex_buffer_th_;
            delete[] nodeindex_rthost_;
        }
        nodeindex_buffer_ = 0;
        nodeindex_buffer_th_ = 0;
        nodeindex_rthost_ = 0;
        if (trecvbuf_) {
            delete[] trecvbuf_;
            delete[] tsendbuf_;
        }
        trecvbuf_ = 0;
        tsendbuf_ = 0;
        if (narea_) {
            delete[] buf_area_indices_;
            delete[] area_node_indices_;
            buf_area_indices_ = 0;
            area_node_indices_ = 0;
            narea_ = 0;
        }
        if (narea2buf_) {
            // at present, statically 3 since only used for ReducedTree
            //			for (i=0; i < narea2buf_; ++i) {
            //				delete [] area2buf_->ibuf;
            //			}
            delete[] area2buf_;
            area2buf_ = 0;
            narea2buf_ = 0;
        }
        if (narea2rt_) {
            delete[] area2rt_;
            area2rt_ = 0;
            narea2rt_ = 0;
        }
    }
}

void nrnmpi_multisplit_clear() {
    if (msc_) {
        msc_->multisplit_clear();
        nrn_multisplit_active_ = 0;
    }
}

void MultiSplitControl::multisplit_clear() {
    // printf("nrnmpi_multisplit_clear()\n");
    int i;
    nrn_multisplit_solve_ = 0;
    nrn_multisplit_setup_ = 0;
    for (i = 0; i < nth_; ++i) {
        mth_[i].del_sidA();
    }
    if (mth_) {
        delete[] mth_;
        mth_ = 0;
    }
    nth_ = 0;
    del_msti();
    if (classical_root_to_multisplit_) {
        for (const auto& mspair: *classical_root_to_multisplit_) {
            delete mspair.second;
        }
        classical_root_to_multisplit_.reset();
        delete multisplit_list_;
        multisplit_list_ = nullptr;
    }
}

// in the incremental development of exchange_setup from only long to short
// to reduced tree we end up with many iterations over all pc.multisplit
// calls. ie. every machines sid info. If the performance becomes
// visible relative to the total setup time then one can replace all those
// iterations by preserving only the sid graph info that is needed by each
// machine. A sid graph, tree, would contain a list of sid's and a list of
// machines. Then there would be a map of sid2graph. This list and map
// could be constructed with a single iteration. ie. when an edge is seen
// the graphs of the two sids would merge. Then one could throw away all
// graphs that have no sid on this machine. I would expect an order of
// magnitude performance improvement in exchange_setup.

void MultiSplitControl::exchange_setup() {
    int i, j, k;
    del_msti();  // above recalc_diam so multisplit_v_setup does not
                 // attempt to fill offdiag_
    if (diam_changed) {
        recalc_diam();
    }

    // how many nodes are involved on this machine
    // all nodes are distinct. For now we independently assert that
    // all the sid on this machine are distinct. So when we get an
    // sid from another machine it does not have to be directed to
    // more than one place. So, just like splitcell, we cannot test
    // a multisplit on a single machine.
    // When sids are not distinct, in the context of reduced trees,
    // there is no problem with respect to sending sid info to
    // multiple places since each sid info gets added to the reduced
    // tree and the reduced tree answer gets sent back to each sid.
    // So there is not a lot of special code involved with multiple
    // pieces on the same host, since each piece <-> reduced tree
    // independently of where the piece and reduced tree are located.
    int n = 0;
    int nwc = 0;  // number of backbonestyle=2 nodes.
    if (classical_root_to_multisplit_) {
        for (MultiSplit* ms: *multisplit_list_) {
            ++n;
            if (ms->nd[1]) {
                ++n;
            }
            if (ms->backbone_style == 2) {
                ++nwc;
                if (ms->nd[1]) {
                    ++nwc;
                }
            } else if (ms->backbone_style == 1) {
                if (!ms->nd[1]) {
                    ms->backbone_style = 0;
                }
            }
        }
    }
    // printf("%d n=%d nsbb=%d nwc=%d\n", nrnmpi_myid, n, nsbb, nwc);
    if (nrnmpi_numprocs == 1 && n == 0) {
        return;
    }
    // how many nodes are involved on each of the other machines
    int* nn = new int[nrnmpi_numprocs];
    if (nrnmpi_use) {
        nrnmpi_int_allgather(&n, nn, 1);
    } else {
        nn[0] = n;
    }
    // if (nrnmpi_myid==0) for (i=0; i < nrnmpi_numprocs;++i) printf("%d %d nn=%d\n", nrnmpi_myid,
    // i, nn[i]);
    // what are the sid's on this machine
    int* sid = 0;
    int* inode = 0;
    int* threadid = 0;
    // an parallel array back to MultiSplit
    MultiSplit** vec2ms = new MultiSplit*[n];
    // following used to be is_ssb (is sid for short backbone)
    // now we code that as well as enough information for the
    // backbone_style==2 case to figure out the reduced tree matrix
    // so 0 means backbone_style == 0, 1 means backbone_style == 1
    // and > 1 means backbone_style == 2 and the value is
    // other backbone sid+3 (so a value of 2 means that
    // it is a single sid subtree and a value of 3 means that the
    // other backbone sid is 0.
    int* bb_relation = 0;
    if (n > 0) {
        sid = new int[n];
        inode = new int[n];
        bb_relation = new int[n];
        threadid = new int[n];
    }
    if (classical_root_to_multisplit_) {
        i = 0;
        for (MultiSplit* ms: *multisplit_list_) {
            sid[i] = ms->sid[0];
            inode[i] = ms->nd[0]->v_node_index;
            threadid[i] = ms->ithread;
            bb_relation[i] = ms->backbone_style;
            vec2ms[i] = ms;
            ++i;
            if (ms->nd[1]) {
                sid[i] = ms->sid[1];
                inode[i] = ms->nd[1]->v_node_index;
                threadid[i] = ms->ithread;
                bb_relation[i] = ms->backbone_style;
                if (ms->backbone_style == 2) {
                    bb_relation[i - 1] += 1 + sid[i];
                    bb_relation[i] += 1 + sid[i - 1];
                }
                vec2ms[i] = ms;
                ++i;
            }
        }
    }
    // for (i=0; i < n; ++i) { printf("%d %d sid=%d inode=%d %s %d\n", nrnmpi_myid, i, sid[i],
    // inode[i], secname(v_node[inode[i]]->sec), v_node[inode[i]]->sec_node_index_);}
    // what are the sid's and backbone status on each of the other machines
    // first need a buffer for them and their offsets
    int* displ = new int[nrnmpi_numprocs + 1];
    displ[0] = 0;
    for (i = 0; i < nrnmpi_numprocs; ++i) {
        displ[i + 1] = displ[i] + nn[i];
    }
    int nt = displ[nrnmpi_numprocs];  // total number of (distinct) nodes
    int* allsid = new int[nt];
    int* all_bb_relation = new int[nt];
    if (nrnmpi_use) {
        nrnmpi_int_allgatherv(sid, allsid, nn, displ);
        nrnmpi_int_allgatherv(bb_relation, all_bb_relation, nn, displ);
    } else {
        for (i = 0; i < n; ++i) {
            allsid[i] = sid[i];
            all_bb_relation[i] = bb_relation[i];
        }
    }
    if (!n) {
        delete[] allsid;
        delete[] all_bb_relation;
        delete[] displ;
        delete[] nn;
        delete[] vec2ms;
        errno = 0;
        return;
    }

    // now we need to analyze
    // how many machines do we need to communicate with

    // note that when long sid is connected to 1 or more long sid
    // and also a short backbone sid that there is no need to send
    // or receive info from the long sid because the short sid sends
    // back the answer.

    // A similar communication pattern holds for the backbone_style==2
    // case. i.e. a star instead of all2all. The only problem is
    // which host is the center of the star. The following
    // code for backbone_style < 2 is almost unchanged but
    // backbone_style >=2 issue handling is inserted as needed.

    // there should not be that many sid on one machine. so just use
    // linear search

    // for an index into the all... vectors, mark[index] is the index
    // into this hosts vectors of the corresponding sid. It gets slowly
    // transformed into a communication pattern in the sense that
    // mark[index] = -1 means no communication will take place between
    // this host and the host associated with the index.
    // For reduced tree purposes, we overload the mark semantics so
    // that mark[index] points to one (of the possibly two) sid on the
    // same cell. ie. all the sid on other cpus that are part of the
    // whole cell point to that sid.

    // For backbone style 2,
    // when there are several pieces on the same host with the same
    // sid, then the mark points to the principle pieces sid. i.e all
    // the sid on this cpu that are part of the whole cell also point
    // to that sid.

    int* mark = new int[nt];
    int* connects2short = new int[n];
    for (i = 0; i < n; ++i) {   // so we know if we will be sending to
        connects2short[i] = 0;  // a short backbone.
    }
    for (i = 0; i < nt; ++i) {
        mark[i] = -1;
        for (j = 0; j < n; ++j) {
            if (allsid[i] == sid[j]) {
                // here we can errcheck the case of 2 on one
                // machine and a different style for the same
                // sid on another machine. If this passes on
                // all machines then a single cell is necessarily
                // consistent with regard to all sids for it
                // being backbone_style == 2
                if ((bb_relation[j] >= 2) != (all_bb_relation[i] >= 2)) {
                    hoc_execerror("backbone_style==2 inconsistent between hosts for same sid", 0);
                }

                if (all_bb_relation[i] < 2) {
                    mark[i] = j;
                    if (all_bb_relation[i] == 1) {
                        connects2short[j] = 1;
                    }
                }
            }
        }
    }

    // but we do not want to mark allsid on this machine
    for (i = displ[nrnmpi_myid]; i < displ[nrnmpi_myid] + n; ++i) {
        mark[i] = -1;
    }
    // but note that later when we mark the reduced tree items we
    // do leave the marks for allsid on this machine (but the marks
    // have a slightly different meaning

    // undo all the marks for long to long if there is a long to short
    // but be careful not to undo the short to long
    for (i = 0; i < nt; ++i) {
        if (mark[i] >= 0 && connects2short[mark[i]] && all_bb_relation[i] == 0 &&
            bb_relation[mark[i]] == 0) {
            mark[i] = -1;
        }
    }
    // make sure no short to short connections
    for (i = 0; i < nt; ++i) {
        if (mark[i] >= 0) {
            if (bb_relation[mark[i]] == 1 && all_bb_relation[i] == 1) {
                hoc_execerror("a short to short backbone interprocessor connection exists", 0);
            }
        }
    }

    // right now, the communication pattern represented by mark is
    // bogus for backbone_style == 2 and they are all -1.
    // We want to adjust mark so all the sid on
    // a single cell point to one of the sid on this cell
    // The following uses an excessive number of iterations
    // over the all... vectors (proportional to number of distinct sids
    // on a cell and should be made more efficient
    for (j = 0; j < n; ++j) {
        if (bb_relation[j] >= 2) {
            reduced_mark(j, sid[j], nt, mark, allsid, all_bb_relation);
            // note the above will also mark both this host sids
            // so when the other one comes around here it will
            // mark nothing.
        }
    }

#if 0
for (i=0; i < nt; ++i) { printf("%d %d allsid=%d all_bb_relation=%d mark=%d\n",
nrnmpi_myid, i, allsid[i], all_bb_relation[i], mark[i]);}
#endif

    // it is an error if there are two pieces of the same cell on the
    // same host. All we need to do is make sure the mark for this
    // cpu are unique. i.e belong to different cells
    // This is no longer an error. For backbonestyle 2, allowing
    // multiple pieces of same cell on same cpu.
    if (0) {
        int* mcnt = new int[n];
        for (j = 0; j < n; ++j) {
            mcnt[j] = 0;
        }
        for (i = displ[nrnmpi_myid]; i < displ[nrnmpi_myid] + n; ++i) {
            if (all_bb_relation[i] >= 2 && mark[i] >= 0) {
                mcnt[mark[i]] += 1;
                if (all_bb_relation[i] > 2) {
                    ++i;
                }
            }
        }
        for (j = 0; j < n; ++j) {
            assert(mcnt[j] < 2);
        }
        delete[] mcnt;
    }

    // for reduced trees, we now need to settle on which host will solve
    // a reduced tree. If rthost is this one then this receives from
    // and sends to every host that has a sid on the cell. If the rthost
    // is another, then this sends and receives to rthost.

    // For now let it be the first host with two sid, and if there is
    // no such host (then why are we using style 2?) then the first
    // host with 1.
    nrtree_ = 0;
    int* rthost = new int[n];
    ReducedTree** rt = new ReducedTree*[n];
    for (j = 0; j < n; ++j) {
        rthost[j] = -1;
        rt[j] = 0;
    }
    if (nwc) {
        for (MultiSplit* ms: *multisplit_list_) {
            if (ms->backbone_style == 2) {
                for (j = 0; j < n; ++j) {  // find the mark value
                    if (sid[j] == ms->sid[0]) {
                        break;
                    }
                }
                ms->rthost = -1;
                k = -1;
                for (int ih = 0; ih < nrnmpi_numprocs; ++ih) {
                    for (i = displ[ih]; i < displ[ih + 1]; ++i) {
                        if (mark[i] == j) {
                            if (all_bb_relation[i] > 2) {
                                ms->rthost = ih;
                                break;
                            }
                            if (all_bb_relation[i] == 2) {
                                if (k == -1) {
                                    k = ih;
                                }
                            }
                        }
                    }
                    if (ms->rthost != -1) {
                        break;
                    }
                }
                if (ms->rthost == -1) {
                    ms->rthost = k;
                }
                // only want rthost[j] for sid[0], not sid[1]
                rthost[j] = ms->rthost;
                // printf("%d sid0=%d sid1=%d rthost=%d\n", nrnmpi_myid, ms->sid[0], ms->sid[1],
                // ms->rthost);
            }
        }
        // it is important that only the principal sid has rthost != 0
        // since we count them below, nrtree_, to allocate rtree_.
    }
#if 0
for (j=0; j < n; ++j) {
printf("%d j=%d sid=%d bb_relation=%d rthost=%d\n", nrnmpi_myid,j, sid[j],
bb_relation[j], rthost[j]);
}
#endif
    // there can be a problem when one host sends parts of two cells
    // to another host due to cells not being in the same order with
    // respect to NrnHashIterate. So demand that they are in the
    // same order. Note this can also be used to assert that there
    // are not two pieces on a host from the same cell.
    // But we no longer use NrnHashIterate and the order is consistent
    // because ordering is always with respect to the allgathered
    // arrays of size nt.
    if (0) {
        for (i = 0; i < nrnmpi_numprocs; ++i) {
            int ix = -1;
            int nj = displ[i + 1];
            for (j = displ[i]; j < nj; ++j) {
                if (all_bb_relation[j] >= 2 && mark[j] >= 0 && rthost[mark[j]] == nrnmpi_myid) {
                    assert(mark[j] > ix);
                    ix = mark[j];
                    if (all_bb_relation[j] > 2) {
                        ++j;
                    }
                }
            }
        }
    }

    // allocate rtree_
    for (j = 0; j < n; ++j) {
        if (rthost[j] == nrnmpi_myid) {
            ++nrtree_;
        }
    }
    if (nrtree_) {
        i = 0;
        rtree_ = new ReducedTree*[nrtree_];
        for (j = 0; j < n; ++j) {
            if (rthost[j] == nrnmpi_myid) {
                // sid to rank table
                std::unique_ptr<Int2IntTable> s2rt{new Int2IntTable()};
                s2rt->reserve(20);
                int rank = 0;
                int mapsize = 0;
                for (k = 0; k < nt; ++k) {
                    if (mark[k] == j && all_bb_relation[k] >= 2) {
                        const auto& result = s2rt->insert({allsid[k], rank});
                        if (result.second) {
                            rank++;
                        }
                        if (all_bb_relation[k] == 2) {
                            mapsize += 2;
                        } else {
                            mapsize += 3;
                        }
                    }
                }
                rtree_[i] = new ReducedTree(this, rank, mapsize);
                rt[j] = rtree_[i];  // needed for third pass below
                // printf("%d new ReducedTree(%d, %d)\n", nrnmpi_myid, rank, mapsize);
                // at this point the ReducedTree.s2rt is not in tree order
                // (in fact we do not even know if it is a tree)
                // so reorder. For a tree, we know there must be ReducedTree.n - 1
                // edges.
                rtree_[i]->s2rt = std::move(s2rt);
                rtree_[i]->reorder(j, nt, mark, all_bb_relation, allsid);
                ++i;
            }
        }
    }

    // fill in the rest of the rthost[], mt->rthost, and rt
    j = 0;
    for (MultiSplit* ms: *multisplit_list_) {
        if (ms->backbone_style == 2) {
            int jj = mark[displ[nrnmpi_myid] + j];
            ms->rthost = rthost[jj];
            rthost[j] = ms->rthost;
            rt[j] = rt[jj];
            if (ms->nd[1]) {
                ++j;
            }
        }
        ++j;
    }
#if 0
	for (int ii=0; ii < multisplit_list_->count(); ++ii) {
		MultiSplit* ms = multisplit_list_->item(ii);
		if (ms->backbone_style == 2) {
printf("%d sid0=%d sid1=%d rthost=%d\n", nrnmpi_myid, ms->sid[0], ms->sid[1], ms->rthost);
		}
	}
for (j=0; j < n; ++j) {
printf("%d j=%d sid=%d bb_relation=%d rthost=%d\n", nrnmpi_myid,j, sid[j],
bb_relation[j], rthost[j]);
}
#endif

    // count the reduced tree related nodes that have non-zero area
    for (i = 0; i < n; ++i) {
        // remember rthost >= 0 only for sid[0]
        if (rthost[i] >= 0)
            for (j = 0; j < 2; ++j) {
                NrnThread* _nt = nrn_threads + threadid[i];
                Node* nd = _nt->_v_node[inode[i + j]];
                if (nd->_classical_parent && nd->sec_node_index_ < nd->sec->nnode - 1) {
                    if (rthost[i] == nrnmpi_myid) {
                        ++narea2rt_;
                    } else {
                        ++narea2buf_;
                    }
                }
                if (bb_relation[i] == 2) {  // sid[0] only
                    break;
                }
                if (j == 1) {
                    ++i;
                }
            }
    }
    if (narea2rt_) {
        area2rt_ = new Area2RT[narea2rt_];
    }
    if (narea2buf_) {
        area2buf_ = new Area2Buf[narea2buf_];
    }
    // can fill in all of the info for area2rt_ but only some for area2buf_
    // actually, area2rt is the one used in practice since only the soma ever
    // has a sid at a non-zero area node and the reduced tree will probably
    // be on the machine with the soma.
    narea2rt_ = narea2buf_ = 0;
    for (i = 0; i < n; ++i) {
        // remember rthost >= 0 only for sid[0]
        if (rthost[i] >= 0)
            for (j = 0; j < 2; ++j) {
                NrnThread* _nt = nrn_threads + threadid[i];
                Node* nd = _nt->_v_node[inode[i + j]];
                auto* const vec_d = _nt->node_d_storage();
                auto* const vec_rhs = _nt->node_rhs_storage();
                if (nd->_classical_parent && nd->sec_node_index_ < nd->sec->nnode - 1) {
                    if (rthost[i] == nrnmpi_myid) {
                        Area2RT& art = area2rt_[narea2rt_];
                        art.ms = vec2ms[i];
                        art.inode = inode[i + j];
                        art.n = 2;
                        art.pd[0] = &D(art.inode);
                        art.pd[1] = &RHS(art.inode);
                        if (bb_relation[i] > 2) {
                            art.n = 3;
                            k = vec2ms[i]->back_index;
                            MultiSplitThread& t = mth_[vec2ms[i]->ithread];
                            if (j == 0) {
                                art.pd[2] = t.sid1A + t.backAindex_[k];
                            } else {
                                art.pd[2] = t.sid1B + t.backBindex_[k];
                            }
                        }
                        ++narea2rt_;
                    } else {
                        Area2Buf& ab = area2buf_[narea2buf_];
                        ab.ms = vec2ms[i];
                        ab.inode = inode[i + j];
                        // can figure out how many but not
                        // the indices into the send buffer.
                        ab.n = 2;
                        if (bb_relation[i] > 2) {
                            ab.n = 3;
                        }
                        ++narea2buf_;
                    }
                }
                if (bb_relation[i] == 2) {  // sid[0] only
                    break;
                }
            }
    }
#if 0
printf("%d narea2rt=%d narea2buf=%d\n", nrnmpi_myid, narea2rt_, narea2buf_);
for (i = 0; i < narea2rt_; ++i) {
	Area2RT& art = area2rt_[i];
	printf("%d area2rt[%d] inode=%d n=%d thread=%d\n", nrnmpi_myid, i, art.inode, art.n, art.ms->ithread);
}
for (i = 0; i < narea2buf_; ++i) {
	Area2Buf& ab = area2buf_[i];
	printf("%d area2buf[%d] inode=%d n=%d\n", nrnmpi_myid, i, ab.inode, ab.n);
}
#endif
#if 0
if (nrnmpi_myid == 0) {
for (i=0; i < n; ++i) {
        printf("%d %d sid=%d bbrelation=%d rthost=%d rt=%p\n", nrnmpi_myid, i, sid[i], bb_relation[i], rthost[i], rt[i]);
}
for (i=0; i < nrnmpi_numprocs+1; ++i) {
	printf("%d displ[%d]=%d\n", nrnmpi_myid, i, displ[i]);
}
for (i=0; i < nt; ++i) {
	printf("%d %d allsid=%d mark=%d all_bb_relation=%d\n", nrnmpi_myid, i, allsid[i], mark[i], all_bb_relation[i]);
}}
#endif
    // it is best if the user arranges things so that machine -> machine
    // has only one type of connection, i.e long->short, long->long,
    // or short->long since that means only one message required.
    // But for generality we program for the possibility of needing
    // two message types (long->short and short->long are the same
    // type and distinct from long->long)
    // Actually we do need 3 because one cannot mix long->short
    // and long->long in the same message because they need
    // different tags.
    // we also calculate the indices for the types
    // we need to have separate tags for long <-> reduced trees
    nthost_ = 0;
    //	int ndbsize, tbsize;
    ndbsize = 0;
    tbsize = 0;

    for (i = 0; i < nrnmpi_numprocs; ++i) {
        int nh = 0;
        int type = 0;
        int nh1 = 0;
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            if (mark[j] >= 0) {
                // how many distinct all_bb_relation<2: 1,2,3?
                if (bb_relation[mark[j]] == 1) {  // short<->long
                    assert(all_bb_relation[j] == 0);
                    type |= 04;
                    ++ndbsize;                         // space for node index
                    tbsize += 2;                       // d and rhs
                } else if (all_bb_relation[j] == 1) {  // long<->short
                    type |= 01;
                    ++ndbsize;
                    tbsize += 2;
                } else if (all_bb_relation[j] < 2) {  // long<->long
                    type |= 02;
                    ++ndbsize;
                    tbsize += 2;
                } else if (all_bb_relation[j] >= 2) {
                    int rth = rthost[mark[j]];
                    if (rth == nrnmpi_myid) {
                        if (i != nrnmpi_myid) {
                            // this is the rthost
                            // reducedtree->long
                            type |= 010;
                            // no node buffer since all
                            // info <-> ReducedTree
                            // ++ndbsize;
                            tbsize += 2;
                            if (all_bb_relation[j] > 2) {
                                tbsize += 1;
                            }
                        }
                    } else if (i == nrnmpi_myid) {
                        // only this to the rthost
                        // long->reducedtree
                        nh1 += 1;
                        // may not be distinct hosts
                        // be careful not to count twice
                        for (int jj = displ[i]; jj < j; ++jj) {
                            if (rth == rthost[mark[jj]]) {
                                nh1 -= 1;
                                break;
                            }
                        }
                        ++ndbsize;
                        tbsize += 2;  // off diag element
                        if (all_bb_relation[j] > 2) {
                            tbsize += 1;
                        }
                    }
                }
            }
        }
        nh = (type & 1) + ((type / 2) & 1) + ((type / 4) & 1) + ((type / 010) & 1);
        nthost_ += nh + nh1;
        // printf("%d type=%d %d %d %d %d %d nh=%d nthost_=%d\n", nrnmpi_myid, type, type&1,
        // (type/2)&1, (type/4)&1, (type/010)&1, (type/020)&1, nh, nthost_);
    }
    // printf("%d x nthost_=%d\n", nrnmpi_myid, nthost_);
    msti_ = new MultiSplitTransferInfo[nthost_];
    for (i = 0; i < nthost_; ++i) {  // two of these needed before rest of fill
        msti_[i].nnode_rt_ = 0;
        msti_[i].nd_rt_index_ = 0;
        msti_[i].nd_rt_index_th_ = 0;
        msti_[i].offdiag_ = 0;
        msti_[i].ioffdiag_ = 0;
        msti_[i].rthost_ = -1;
    }
    if (ndbsize) {  // can be 0 if rthost
        nodeindex_buffer_ = new int[ndbsize];
        nodeindex_buffer_th_ = new int[ndbsize];
        nodeindex_rthost_ = new int[ndbsize];
    }
    for (i = 0; i < ndbsize; ++i) {
        nodeindex_rthost_[i] = -1;
    }
    // printf("%d nthost_=%d ndbsize=%d tbsize=%d nrtree_=%d\n", nrnmpi_myid, nthost_, ndbsize,
    // tbsize, nrtree_); tbsize_=tbsize;
    if (tbsize) {
        trecvbuf_ = new double[tbsize];
        tsendbuf_ = new double[tbsize];
    }
    // We need to fill the msti_ array
    // in the order long<->short  long<->reduced
    // followed by long<->long followed by reduced<->long
    // followed short<->long.  // three distinct message tags
    nthost_ = 0;
    int mdisp = 0;
    k = 0;

    // first pass for the long<->short backbones
    for (i = 0; i < nrnmpi_numprocs; ++i) {
        int b = 0;
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            if (mark[j] >= 0 && bb_relation[mark[j]] == 0 && all_bb_relation[j] == 1) {
                nodeindex_buffer_th_[k] = threadid[mark[j]];
                nodeindex_buffer_[k++] = inode[mark[j]];
                // printf("%d i=%d j=%d k=%d nthost=%d mark=%d inode=%d\n", nrnmpi_myid, i, j, k,
                // nthost_, mark[j], inode[mark[j]]);
                ++b;
            }
        }
        if (b) {
            msti_[nthost_].displ_ = mdisp;
            msti_[nthost_].size_ = 2 * b;
            mdisp += 2 * b;
            msti_[nthost_].host_ = i;
            msti_[nthost_].nnode_ = b;
            msti_[nthost_].tag_ = 1;
            ++nthost_;
        }
    }

    // second pass for the long<->reducedtree
    // includes the communication of sid <-> reduced tree for the
    // non rthost cpus
    // first make a list of the rthost values, the size of that
    // list will be how many msti we need for this pass.
    int* tmphost = new int[n];  // cannot be more than this.
    int ntmphost = 0;
    i = nrnmpi_myid;
    for (j = displ[i]; j < displ[i + 1]; ++j) {
        int j1 = mark[j];
        if (j1 >= 0 && bb_relation[j1] >= 2 && rthost[j1] != i) {
            tmphost[ntmphost++] = rthost[j1];
            for (int itmp = 1; itmp < ntmphost; ++itmp) {
                if (tmphost[itmp - 1] == rthost[j1]) {
                    ntmphost--;
                    break;
                }
            }
        }
    }
    // printf("%d second pass ntmphost = %d\n", nrnmpi_myid, ntmphost);
    // now we can loop over that list and fill the msti
    for (int itmp = 0; itmp < ntmphost; ++itmp) {
        i = nrnmpi_myid;
        int b = 0;
        int br = 0;
        // only the ones on this machine and rthost not this machine
        // Note: rthost generally has different sid value
        // and number of sid than this machine
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            int jj = j - displ[i];
            int j1 = mark[j];  // point to sid[0] on this machine
            // skip if sid[1] of the rthost
            // printf("%d i=%d j=%d j1=%d bb_relation[j1]=%d rthost[j1]=%d\n",
            // nrnmpi_myid, i, j, j1, bb_relation[j1], rthost[j1]);
            if (j1 >= 0 && bb_relation[j1] >= 2 && rthost[j1] == tmphost[itmp]) {  // only the first
                                                                                   // has rthost
                // printf("%d sid=%d bb_relation=%d send to rthost=%d k=%d\n", nrnmpi_myid,
                // sid[j1], bb_relation[j1], rthost[j1], k);
                // mark[j] points to the index for sid0
                nodeindex_rthost_[k] = rthost[j1];
                nodeindex_buffer_th_[k] = threadid[j - displ[i]];
                nodeindex_buffer_[k++] = inode[j - displ[i]];
                ++b;
                // one or two?
                if (bb_relation[jj] > 2) {
                    // int iii = i;
                    {
                        // except that msti_[nthost_].offdiag_ has not been allocated, this is a
                        // good time to figure out the pointers to sid1A and sid1B and which is
                        // which
                        MultiSplitTransferInfo& m = msti_[nthost_];
                        int i;
                        int* ix;
                        int* ixth;
                        double** od;
                        int* iod;
                        // start by assuming 2 and then increment by 2
                        if (m.offdiag_) {
                            ix = m.nd_rt_index_;
                            ixth = m.nd_rt_index_th_;
                            od = m.offdiag_;
                            iod = m.ioffdiag_;
                            m.nd_rt_index_ = new int[m.nnode_rt_ + 2];
                            m.nd_rt_index_th_ = new int[m.nnode_rt_ + 2];
                            m.offdiag_ = new double*[m.nnode_rt_ + 2];
                            m.ioffdiag_ = new int[m.nnode_rt_ + 2];
                            for (i = 0; i < m.nnode_rt_; ++i) {
                                m.nd_rt_index_[i] = ix[i];
                                m.nd_rt_index_th_[i] = ixth[i];
                                m.offdiag_[i] = od[i];
                                m.ioffdiag_[i] = iod[i];
                            }
                            delete[] ix;
                            delete[] ixth;
                            delete[] od;
                            delete[] iod;
                            ix = m.nd_rt_index_ + m.nnode_rt_;
                            ixth = m.nd_rt_index_th_;
                            od = m.offdiag_ + m.nnode_rt_;
                            iod = m.ioffdiag_ + m.nnode_rt_;
                        } else {
                            m.nd_rt_index_ = new int[2];
                            m.nd_rt_index_th_ = new int[2];
                            m.offdiag_ = new double*[2];
                            m.ioffdiag_ = new int[2];
                            ix = m.nd_rt_index_;
                            ixth = m.nd_rt_index_th_;
                            od = m.offdiag_;
                            iod = m.ioffdiag_;
                        }
                        // now fill in
                        // sid0 is for sid1A[0] , sid1 is for sid1B[???]
                        // sid0: nodeindex = inode[j1]
                        // in multisplit_v_setup, we created two parallel vectors of size
                        // nbackrt_ (number of backbones sending to reduced tree).
                        // backsid_ is sid0, and
                        // backAindex_ corresponding  sid1A index for sid0
                        // backBindex_ sid1B index for sid1
                        // Should not be too many backbones on this machine so linear
                        // search should be ok. BUG again. i is MultiSplit.back_index and
                        // can determine MultiSplit from j1.
                        i = vec2ms[jj]->back_index;
                        MultiSplitThread& t = mth_[vec2ms[jj]->ithread];
                        assert(sid[jj] == t.backsid_[i]);
                        ix[0] = inode[jj];
                        ix[1] = inode[jj + 1];
                        ixth[0] = vec2ms[jj]->ithread;
                        ixth[1] = vec2ms[jj]->ithread;
                        od[0] = t.sid1A + t.backAindex_[i];
                        od[1] = t.sid1B + t.backBindex_[i];
                        iod[0] = t.backAindex_[i];
                        iod[1] = t.backBindex_[i];
#if 0
printf("%d offdiag nbrt=%d iii=%d i=%d j1=%d jj=%d sid=%d ix = %d %d  back = %d %d\n",
nrnmpi_myid, nbackrt_, iii, i, j1, jj, sid[j1], ix[0], ix[1], t.backAindex_[i], t.backBindex_[i]);
#endif
                        m.nnode_rt_ += 2;
                    }
                    nodeindex_rthost_[k] = rthost[j1];
                    ++j;
                    ++jj;
                    nodeindex_buffer_th_[k] = threadid[jj];
                    nodeindex_buffer_[k++] = inode[jj];
                    ++b;
                    br += 2;
                }
            }
        }
        if (b || br) {
            msti_[nthost_].displ_ = mdisp;
            msti_[nthost_].size_ = 2 * b + br;
            mdisp += 2 * b + br;
            msti_[nthost_].host_ = tmphost[itmp];
            msti_[nthost_].rthost_ = tmphost[itmp];
            msti_[nthost_].nnode_ = b;
            msti_[nthost_].tag_ = 3;
            ++nthost_;
        }
    }
    delete[] tmphost;

    // third pass for the long->long backbones
    for (i = 0; i < nrnmpi_numprocs; ++i) {
        int b = 0;
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            if (mark[j] >= 0 && bb_relation[mark[j]] == 0 && all_bb_relation[j] == 0) {
                nodeindex_buffer_th_[k] = threadid[mark[j]];
                nodeindex_buffer_[k++] = inode[mark[j]];
                // printf("%d i=%d j=%d k=%d nthost=%d mark=%d inode=%d\n", nrnmpi_myid, i, j, k,
                // nthost_, mark[j], inode[mark[j]]);
                ++b;
            }
        }
        if (b) {
            msti_[nthost_].displ_ = mdisp;
            msti_[nthost_].size_ = 2 * b;
            mdisp += 2 * b;
            msti_[nthost_].host_ = i;
            msti_[nthost_].nnode_ = b;
            msti_[nthost_].tag_ = 2;
            ++nthost_;
        }
    }

    // fourth pass for the reducedtree->long backbones
    // the reduced tree <-> sids not on this host
    ihost_reduced_long_ = nthost_;
    for (i = 0; i < nrnmpi_numprocs; ++i) {
        int b = 0;
        int br = 0;
        // not the ones on this machine and rthost this machine
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            int j1 = mark[j];
            if (j1 >= 0 && all_bb_relation[j] >= 2 && rthost[j1] == nrnmpi_myid &&
                rthost[j1] != i) {
                // printf("%d i=%d sid=%d bb_relation=%d send to rthost=%d rt=%p\n", nrnmpi_myid, i,
                // allsid[j], all_bb_relation[j], rthost[j1], rt[j1]);
                // don't care about the nodeindex, only
                // the ReducedTree.
                // fill in the ReducedTree map with RHS and D pointers to the sid[0] related receive
                // buffer and next iteration the sid[1]
                int ib = mdisp + 2 * b;
                // printf("%d fill buf %d\n", nrnmpi_myid, ib);
                rt[j1]->fillrmap(allsid[j], -1, trecvbuf_ + ib + 1);  // exchange order is d, rhs
                rt[j1]->fillrmap(allsid[j], allsid[j], trecvbuf_ + ib);
                rt[j1]->fillsmap(allsid[j], tsendbuf_ + ib + 1, tsendbuf_ + ib);
                ++b;                           // at least D and RHS received and sent back
                if (all_bb_relation[j] > 2) {  // two of these
                    // need to fill in off diag after this iteration is done and then re-iterate
                    // because they are at the end of the buffer section for machine i.
                    ++br;  // an offdiag received
                           // though I suppose it does not
                           // need to be sent back
                }
            }
        }
        // reiterate and fill in off diag
        br = 0;
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            int j1 = mark[j];
            if (j1 >= 0 && all_bb_relation[j] >= 2 && rthost[j1] == nrnmpi_myid &&
                rthost[j1] != i) {
                if (all_bb_relation[j] > 2) {  // two of these
                    int ib = mdisp + 2 * b + br;
                    // printf("%d offdiag fill buf %d j1=%d rt=%p\n", nrnmpi_myid, ib, j1, rt[j1]);
                    rt[j1]->fillrmap(allsid[j + 1], allsid[j], trecvbuf_ + ib);
                    rt[j1]->fillrmap(allsid[j], allsid[j + 1], trecvbuf_ + ib + 1);
                    br += 2;
                    j += 1;
                }
            }
        }

        if (b || br) {
            msti_[nthost_].displ_ = mdisp;
            msti_[nthost_].size_ = 2 * b + br;
            mdisp += 2 * b + br;
            msti_[nthost_].host_ = i;
            msti_[nthost_].rthost_ = nrnmpi_myid;
            msti_[nthost_].nnode_ = b;
            // no nnode_rt_ for this phase
            // msti_[nthost_].nnode_rt_ = br;
            // msti_[nthost_].offdiag_ = new double*[br];
            msti_[nthost_].tag_ = 3;
            ++nthost_;
        }
    }

    // fifth pass for the short->long backbones
    int tmp_index = k;
    ihost_short_long_ = nthost_;
    for (i = 0; i < nrnmpi_numprocs; ++i) {
        int b = 0;
        for (j = displ[i]; j < displ[i + 1]; ++j) {
            if (mark[j] >= 0 && bb_relation[mark[j]] == 1 && all_bb_relation[j] == 0) {
                nodeindex_buffer_th_[k] = threadid[mark[j]];
                nodeindex_buffer_[k++] = inode[mark[j]];
                // printf("%d i=%d j=%d k=%d nthost=%d mark=%d inode=%d\n", nrnmpi_myid, i, j, k,
                // nthost_, mark[j], inode[mark[j]]);
                ++b;
            }
        }
        if (b) {
            msti_[nthost_].displ_ = mdisp;
            msti_[nthost_].size_ = 2 * b;
            mdisp += 2 * b;
            msti_[nthost_].host_ = i;
            msti_[nthost_].nnode_ = b;
            msti_[nthost_].tag_ = 1;
            ++nthost_;
        }
    }

#if 0
printf("%d nthost_ = %d\n", nrnmpi_myid, nthost_);
for (i=0; i < nthost_; ++i) {
MultiSplitTransferInfo& m = msti_[i];
printf("%d i=%d displ_=%d host_=%d nnode_=%d tag_=%d\n", nrnmpi_myid, i, m.displ_,
m.host_, m.nnode_, m.tag_);
}
#endif

#if 0
for (i=0; i < k; ++i) {
j=nodeindex_buffer_[i];
printf("%d %d nodeindex_buffer %d %s %d\n", nrnmpi_myid, i, j,
secname(v_node[j]->sec), v_node[j]->sec_node_index_);
}
#endif

    // the sids on this machine with rthost on this machine
    // go into the ReducedTree directly
    if (nrtree_) {
        i = 0;
        int ib = 0;
        for (MultiSplit* ms: *multisplit_list_) {
            NrnThread* _nt = nrn_threads + ms->ithread;
            MultiSplitThread& t = mth_[ms->ithread];
            auto* const vec_d = _nt->node_d_storage();
            auto* const vec_rhs = _nt->node_rhs_storage();
            if (ms->rthost == nrnmpi_myid) {
                // printf("%d nrtree_=%d i=%d rt=%p\n", nrnmpi_myid, nrtree_, i, rt[i]);
                int j = ms->nd[0]->v_node_index;
                // printf("%d call fillrmap sid %d,%d  %d node=%d\n", nrnmpi_myid, ms->sid[0],
                // ms->sid[0], j);
                ms->rt_ = rt[i];
                ms->rmap_index_ = rt[i]->irfill;
                ms->smap_index_ = rt[i]->nsmap;
                rt[i]->fillrmap(ms->sid[0], -1, &RHS(j));
                rt[i]->fillrmap(ms->sid[0], ms->sid[0], &D(j));
                rt[i]->fillsmap(ms->sid[0], &RHS(j), &D(j));
                if (ms->nd[1]) {
                    ib = ms->back_index;
                    assert(t.backsid_[ib] == ms->sid[0]);
                    // fill sid1 row
                    // note: for cvode need to keep fillrmap as pairs so following
                    // moved from between  sid1A,sid1B fillrmap.
                    j = ms->nd[1]->v_node_index;
                    // printf("%d call fillrmap sid1 %d %d node=%d\n", nrnmpi_myid,  ms->sid[1],
                    // ms->sid[1], j);
                    rt[i]->fillrmap(ms->sid[1], -1, &RHS(j));
                    rt[i]->fillrmap(ms->sid[1], ms->sid[1], &D(j));
                    rt[i]->fillsmap(ms->sid[1], &RHS(j), &D(j));

                    // fill sid1A for sid0 row
                    // printf("%d call fillrmap sid %d,%d %d ib=%d t.backsid=%d t.backAindex=%d\n",
                    // nrnmpi_myid, ms->sid[1], ms->sid[0], ib, t.backsid_[ib], t.backAindex_[ib]);
                    rt[i]->fillrmap(ms->sid[1], ms->sid[0], t.sid1A + t.backAindex_[ib]);

                    // fill sid1B
                    // printf("%d call fillrmap sid %d,%d ib=%d backBindex=%d\n", nrnmpi_myid,
                    // ms->sid[0], ms->sid[1], ib, backBindex_[ib]);
                    rt[i]->fillrmap(ms->sid[0], ms->sid[1], t.sid1B + t.backBindex_[ib]);
                }
            }
            ++i;
            if (ms->nd[1]) {
                ++i;
            }
        }
    }

    if (nthost_) {
        msti_[0].nodeindex_th_ = nodeindex_buffer_th_;
        msti_[0].nodeindex_ = nodeindex_buffer_;
        for (i = 1; i < nthost_; ++i) {
            msti_[i].nodeindex_th_ = msti_[i - 1].nodeindex_th_ + msti_[i - 1].nnode_;
            msti_[i].nodeindex_ = msti_[i - 1].nodeindex_ + msti_[i - 1].nnode_;
        }
    }
    // count how many nodeindex_buffer are non-zero area nodes
    // iarea_short_long_ and narea_ only for ones
    // not related to the ReducedTree method
    // printf("%d ndbsize=%d\n", nrnmpi_myid, ndbsize);
    for (i = 0; i < ndbsize; ++i) {
        NrnThread* _nt = nrn_threads + nodeindex_buffer_th_[i];
        Node* nd = _nt->_v_node[nodeindex_buffer_[i]];
        if (i == tmp_index) {
            iarea_short_long_ = narea_;
        }
        if (nd->_classical_parent && nd->sec_node_index_ < nd->sec->nnode - 1) {
            if (nodeindex_rthost_[i] < 0) {
                ++narea_;
            }
        }
    }
    // printf("%d narea=%d iarea_short_long=%d\n", nrnmpi_myid, narea_, iarea_short_long_);
    if (narea_) {
        buf_area_indices_ = new int[narea_];
        area_node_indices_ = new int[narea_];
        k = 0;
        for (i = 0; i < ndbsize; ++i) {
            NrnThread* _nt = nrn_threads + nodeindex_buffer_th_[i];
            Node* nd = _nt->_v_node[nodeindex_buffer_[i]];
            if (nd->_classical_parent && nd->sec_node_index_ < nd->sec->nnode - 1) {
                buf_area_indices_[k] = 2 * i;
                area_node_indices_[k] = nodeindex_buffer_[i];
                ++k;
            }
        }
    }

    // look through the nthost list and fill in the area2buf receive buffer
    // indices for info sent to reducetrees (by definition,not on this machine)
    for (i = 0; i < nthost_; ++i) {
        MultiSplitTransferInfo& msti = msti_[i];
        if (msti.tag_ == 3) {  // reduced tree related
            // but we only want to, not from, the reduced tree machine
            // any nodes nonzero area?
            // printf("%d i=%d nnode=%d nodeindex=%p host=%d rthost=%d\n", nrnmpi_myid, i,
            // msti.nnode_, msti.nodeindex_, msti.host_, msti.rthost_);
            if (msti.rthost_ != nrnmpi_myid)
                for (j = 0; j < msti.nnode_; ++j) {
                    NrnThread* _nt = nrn_threads + msti.nodeindex_th_[j];
                    int in = msti.nodeindex_[j];
                    Node* nd = _nt->_v_node[in];
                    if (nd->_classical_parent && nd->sec_node_index_ < nd->sec->nnode - 1) {
                        // non-zero area
                        // search the area2buf list
                        for (k = 0; k < narea2buf_; ++k) {
                            if (area2buf_[k].inode == in && area2buf_[k].ms->ithread == _nt->id) {
                                break;
                            }
                        }
                        assert(k < narea2buf_);
                        Area2Buf& ab = area2buf_[k];
                        // ok. fill in the ibuf info
                        ab.ibuf[0] = msti.displ_ + 2 * j;
                        ab.ibuf[1] = ab.ibuf[0] + 1;
                        if (ab.n == 3) {
                            // which offdiag item?
                            int ioff;
                            for (ioff = 0; ioff < msti.nnode_rt_; ++ioff) {
                                if (msti.nd_rt_index_[ioff] == in &&
                                    msti.nd_rt_index_th_[ioff] == _nt->id) {
                                    break;
                                }
                            }
                            assert(ioff < msti.nnode_rt_);
                            ab.ibuf[2] = msti.displ_ + 2 * msti.nnode_ + ioff;
                        }
#if 0
printf("%d in=%d n=%d ibuf %d %d %d\n", nrnmpi_myid, in, ab.n,
ab.ibuf[0], ab.ibuf[1], (ab.n == 3) ? ab.ibuf[2] : -1);
#endif
                    }
                }
        }
    }

    // printf("%d leave exchange_setup\n", nrnmpi_myid);
    // nrnmpi_int_allgather(&n, nn, 1);

    delete[] nn;
    delete[] sid;
    delete[] inode;
    delete[] threadid;
    delete[] displ;
    delete[] allsid;
    delete[] vec2ms;
    delete[] mark;
    delete[] bb_relation;
    delete[] all_bb_relation;
    delete[] connects2short;
    delete[] rthost;
    delete[] rt;

    errno = 0;
}

// when d and rhs pointers are updated in the Node, they must be updated in
// the reducedtree rmap and smap as well
void nrn_multisplit_ptr_update() {
    msc_->rt_map_update();
}

void MultiSplitControl::rt_map_update() {
    for (MultiSplit* ms: *multisplit_list_) {
        if (ms->rthost == nrnmpi_myid) {  // reduced tree on this host
            assert(ms->rt_);
            assert(ms->rmap_index_ >= 0);
            assert(ms->smap_index_ >= 0);
            MultiSplitThread& t = mth_[ms->ithread];
            double** r = ms->rt_->rmap + ms->rmap_index_;
            double** s = ms->rt_->smap + ms->smap_index_;
            for (int j = 0; j < 2; ++j)
                if (ms->nd[j]) {
                    *s++ = *r++ = &NODERHS(ms->nd[j]);
                    *s++ = *r++ = &NODED(ms->nd[j]);
                }
            if (ms->nd[1]) {  // do sid1a and sid1b as well
                assert(ms->back_index >= 0);
                *r++ = t.sid1A + t.backAindex_[ms->back_index];
                *r++ = t.sid1B + t.backBindex_[ms->back_index];
            }
        }
    }
    // also need to do the Area2RT.pd[3] where the
    // order is d, rhs, and possibly sid1A or sid1B
    for (int i = 0; i < narea2rt_; ++i) {
        Area2RT& art = area2rt_[i];
        MultiSplit& ms = *art.ms;
        NrnThread* _nt = nrn_threads + ms.ithread;
        art.pd[0] = _nt->node_d_storage() + art.inode;
        art.pd[1] = _nt->node_rhs_storage() + art.inode;
        if (art.n == 3) {
            MultiSplitThread& t = mth_[ms.ithread];
            if (art.inode == ms.nd[0]->v_node_index) {
                art.pd[2] = t.sid1A + t.backAindex_[ms.back_index];
            } else if (art.inode == ms.nd[1]->v_node_index) {
                art.pd[2] = t.sid1B + t.backBindex_[ms.back_index];
            } else {
                assert(0);
            }
        }
    }
}

void MultiSplitControl::pexch() {
    int i, j, k, id;
    id = nrnmpi_myid;
    // assume only one thread when there is transfer info
    NrnThread* nt = nrn_threads;
    Printf("%d nthost_=%d\n", id, nthost_);
    for (i = 0; i < nthost_; ++i) {
        MultiSplitTransferInfo& ms = msti_[i];
        Printf("%d %d host=%d nnode=%d displ=%d\n", id, i, ms.host_, ms.nnode_, ms.displ_);
        for (j = 0; j < ms.nnode_; ++j) {
            k = ms.nodeindex_[j];
            Printf("%d %d %d %d %s %d\n",
                   id,
                   i,
                   j,
                   k,
                   secname(nt->_v_node[k]->sec),
                   nt->_v_node[k]->sec_node_index_);
        }
    }
}

void MultiSplitControl::reduced_mark(int m,
                                     int sid,
                                     int nt,
                                     int* mark,
                                     int* allsid,
                                     int* all_bb_relation) {
    int i;
    for (i = 0; i < nt; ++i) {
        if (mark[i] == -1 && allsid[i] == sid) {
            mark[i] = m;
            if (all_bb_relation[i] > 2) {
                int sid2 = all_bb_relation[i] - 3;
                reduced_mark(m, sid2, nt, mark, allsid, all_bb_relation);
            }
        }
    }
}

/*
section ParallelContext.multisplit_connect(x, sid) is a generalization of
root ParallelContext.splitcell_connect(adjacent_host).
sid refers to a point and all points with the same sid are connected
together with 0 resistance wires.
If a connected set of sections has only 1 sid then that point will be the
root and triangularization can proceed normally.
If a connected set of sections has 2 sid then triangularization proceeds
from the leaves to the backbone between the 2 sid which forms a cable.
Then special triangularization with fill-in of the sid1A column
proceeds from the backbone element adjacent to sid1 and then with fill-in
of the sid1B column (which modifies the sid1A column) from the backbone end
adjacent to sid0
in the hope that the fill-in elements at the sid are small enough
to ignore in the interprocessor jacobian element matrix exchange. Note
that gaussian elimination in the backbone is more complex as normal
tree elimination with the same number of nodes.
We defer the implementation of more than 2 sid in a subtree.
*/

/* Gaussian elimination strategy for the backbone.
There are two different strategies that optimize either minimum
number of matrix operations that is best for long backbones, or
numerical stability by minimizing the far off diagonal just before
exchange which would be best for short (tightly coupled) backbones.

For long backbones, it is simplest to begin triangularization
using the equations adjacent to the center as pivot equations.
This fills in a single column. But in the trivial case of the center
being the only interior point, there are no triangularization operations
before exchange.
1 xx
2 xxx
3  xxx
4   xxx
5    xxx
6     xxx
7      xx


1 x  x
2 xx x
3  xxx
4   xxx
5    xxx
6    x xx
7    x  x

exchange occurs

1 x  x
2  x x
3   xx
4    x
5    xx
6    x x
7    x  x

For short backbones and greatest stability/accuracy and still simple but
many more operations, one begins adjacent to the sids. Now,
in the trivial case of the center being the only interior point, it is
used to reduce the off diagonal element. If there is no interior point
nothing happens.

1 xx
2 xxx       triangularize starting with equation 6 (cannot start with
3  xxx      equation 1 or 7 because we do not know the correct value for d)
4   xxx     this eliminates the a elements but at the cost of introducing
5    xxx    A elements in column 7
6     xxx
7      xx

1 x     x   cannot use 1, instead do the same starting at 2
2 xx    x   this eliminates b elements but at the cost of introducing
3  xx   x   B elements in column 1. Also A elements are modified.
4   xx  x
5    xx x
6     xxx
7      xx

1 x     x
2 xx    x
3 x x   x
4 x  x  x
5 x   x x
6 x    xx
7 x     x

exchange occurs (modified d for 1 and 7
solve the 1 and 7 2x2 equations then back substitute

*/

void MultiSplitControl::prstruct() {
    int id, i, it;
    for (id = 0; id < nrnmpi_numprocs; ++id) {
        nrnmpi_barrier();
        if (id == nrnmpi_myid) {
            Printf("myid=%d\n", id);
            Printf(" MultiSplit %ld\n", multisplit_list_->size());
            for (i = 0; i < multisplit_list_->size(); ++i) {
                MultiSplit* ms = (*multisplit_list_)[i];
                Printf("  %2d bbs=%d bi=%-2d rthost=%-4d %-4d %s{%d}",
                       i,
                       ms->backbone_style,
                       ms->back_index,
                       ms->rthost,
                       ms->sid[0],
                       secname(ms->nd[0]->sec),
                       ms->nd[0]->sec_node_index_);
                if (ms->nd[1]) {
                    Printf("   %-4d %s{%d}",
                           ms->sid[1],
                           secname(ms->nd[1]->sec),
                           ms->nd[1]->sec_node_index_);
                }
                Printf("\n");
            }
            for (it = 0; it < nrn_nthread; ++it) {
                NrnThread* _nt = nrn_threads + it;
                MultiSplitThread& t = mth_[it];
                Printf(" backbone_begin=%d backbone_long_begin=%d backbone_interior_begin=%d\n",
                       t.backbone_begin,
                       t.backbone_long_begin,
                       t.backbone_interior_begin);
                Printf(" backbone_sid1_begin=%d backbone_long_sid1_begin=%d backbone_end=%d\n",
                       t.backbone_sid1_begin,
                       t.backbone_long_sid1_begin,
                       t.backbone_end);
                Printf(" nbackrt_=%d  i, backsid_[i], backAindex_[i], backBindex_[i]\n",
                       t.nbackrt_);
                if (t.nbackrt_) {
                    for (int i = 0; i < t.nbackrt_; ++i) {
                        Printf("  %2d %2d %5d %5d",
                               i,
                               t.backsid_[i],
                               t.backAindex_[i],
                               t.backBindex_[i]);
                        Node* nd = _nt->_v_node[t.backbone_begin + t.backAindex_[i]];
                        Printf(" %s{%d}", secname(nd->sec), nd->sec_node_index_);
                        nd = _nt->_v_node[t.backbone_begin + t.backBindex_[i]];
                        Printf(" %s{%d}", secname(nd->sec), nd->sec_node_index_);
                        Printf("\n");
                    }
                }
            }
            Printf(" ReducedTree %d\n", nrtree_);
            for (i = 0; i < nrtree_; ++i) {
                ReducedTree* rt = rtree_[i];
                Printf("  %d n=%d nmap=%d\n", i, rt->n, rt->nmap);
                rt->pr_map(tbsize, trecvbuf_);
            }
            Printf(" MultiSplitTransferInfo %d\n", nthost_);
            for (i = 0; i < nthost_; ++i) {
                MultiSplitTransferInfo& m = msti_[i];
                Printf("  %d host=%d rthost=%d nnode=%d nnode_rt=%d size=%d tag=%d\n",
                       i,
                       m.host_,
                       m.rthost_,
                       m.nnode_,
                       m.nnode_rt_,
                       m.size_,
                       m.tag_);
                if (m.nnode_) {
                    Printf("    nodeindex=%p  nodeindex_buffer = %p\n",
                           m.nodeindex_,
                           nodeindex_buffer_);
                }
            }
            Printf(" ndbsize=%d  i  nodeindex_buffer_=%p  nodeindex_rthost_=%p\n",
                   ndbsize,
                   nodeindex_buffer_,
                   nodeindex_rthost_);
            if (ndbsize) {
                for (int i = 0; i < ndbsize; ++i) {
                    Printf("  %d %d %d\n", i, nodeindex_buffer_[i], nodeindex_rthost_[i]);
                }
            }
            Printf(" tbsize=%d trecvbuf_=%p tsendbuf_=%p\n", tbsize, trecvbuf_, tsendbuf_);
            Printf("\n");
        }
    }
    nrnmpi_barrier();
}

// following uses the short backbone method (N form of the matrix)


// What is a good order for the above? We already have a tree order structure
// with a root. If there is a single sid involved then that should be the
// root and we are conceptually the same as splitcell.cpp
// For two sid, then the tree is ordered as (see above comment at the NOTE)

void multisplit_solve() {
    msc_->solve();
}

void* nrn_multisplit_triang(NrnThread* nt) {
    msc_->mth_[nt->id].triang(nt);
    return nullptr;
}
void* nrn_multisplit_reduce_solve(NrnThread* nt) {
    if (nt->id == 0) {
        msc_->reduce_solve();
    }
    return nullptr;
}
void* nrn_multisplit_bksub(NrnThread* nt) {
    msc_->mth_[nt->id].bksub(nt);
    return nullptr;
}

void MultiSplitControl::solve() {
    //	if (t < .025) prstruct();
    //	if (nrnmpi_myid == 0) pmat(true);
    NrnThread* nt = nrn_threads;
    MultiSplitThread& t = mth_[0];
    t.triang_subtree2backbone(nt);
    t.triang_backbone(nt);
    //	if (nrnmpi_myid == 4) pmat(true);
    //	pmat1("t");
    // printf("%d enter matrix exchange\n", nrnmpi_myid);
    matrix_exchange();
    // printf("%d leave matrix exchange\n", nrnmpi_myid);
    //	pmat1("e");
    t.bksub_backbone(nt);
    t.bksub_subtrees(nt);
    //	pmat(true);
    //	nrnmpi_barrier(); // only possible if everyone is actually multisplit
    //	abort();
}

void MultiSplitThread::triang(NrnThread* _nt) {
    triang_subtree2backbone(_nt);
    triang_backbone(_nt);
}
void MultiSplitControl::reduce_solve() {
    matrix_exchange();
}
void MultiSplitThread::bksub(NrnThread* _nt) {
    bksub_backbone(_nt);
    bksub_subtrees(_nt);
}

// In the typical case, all nodes connected to the same sids have 0 area
// and thus the weighted average voltage is just sum RHS / sum D.
// However, nrn_multisplit_nocap_v is somewhat of a misnomer because one or
// more compartments (but not connected together with the same sid) may have
// non-zero area and thus that voltage is correct and must be transferred
// to all the connecting (with same sid) zero-area nodes. Furthermore
// the sum RHS (of the zero area nodes) must be added to the RHS of non-zero
// area node (after that non-zero RHS has been computed back in the caller).
// Lastly, that sum of RHS has to be adjusted using the sum of D of the
// zero area nodes so that the RHS is proper with respect to the correct
// value of V.
// So. When there is no non-zero area node for a sid, the voltage is
// sum rhs / sum d. For a non-zero area node the V must be sent and the
// sum rhs and sum d from the others must be received and the true rhs
// put into adjust_rhs_ using rhs - d*v. For a zero-area
// node (with same sid as non-zero area node), rhs and d must be sent, and
// voltage can be computed after receiving the changed rhs and d by
// rhs/d.

void nrn_multisplit_nocap_v() {
    msc_->multisplit_nocap_v_part1(nrn_threads);
    msc_->multisplit_nocap_v_part2(nrn_threads);
    msc_->multisplit_nocap_v_part3(nrn_threads);
}
void nrn_multisplit_nocap_v_part1(NrnThread* nt) {
    msc_->multisplit_nocap_v_part1(nt);
}
void nrn_multisplit_nocap_v_part2(NrnThread* nt) {
    msc_->multisplit_nocap_v_part2(nt);
}
void nrn_multisplit_nocap_v_part3(NrnThread* nt) {
    msc_->multisplit_nocap_v_part3(nt);
}

void MultiSplitControl::multisplit_nocap_v_part1(NrnThread* _nt) {
    int i;
    // scale the non-zero area node elements since those already
    // have the answer.

    // the problem here is that non-zero area v is correct,
    // but rhs ends up wrong for
    // non-zero area nodes (because current from zero area not added)
    // so encode v into D and sum of zero-area rhs will end up in
    // rhs.
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    auto* const vec_v = _nt->node_voltage_storage();
    if (_nt->id == 0)
        for (i = 0; i < narea2buf_; ++i) {
            Area2Buf& ab = area2buf_[i];
            vec_d[ab.inode] = 1e50;  // sentinal
            vec_rhs[ab.inode] = vec_v[ab.inode] * 1e50;
        }
    // also scale the non-zero area elements on this host
    for (i = 0; i < narea2rt_; ++i) {
        Area2RT& ar = area2rt_[i];
        if (_nt->id == ar.ms->ithread) {
            vec_d[ar.inode] = 1e50;
            vec_rhs[ar.inode] = vec_v[ar.inode] * 1e50;
        }
    }
}
void MultiSplitControl::multisplit_nocap_v_part2(NrnThread* _nt) {
    if (_nt->id == 0) {
        matrix_exchange_nocap();
    }
}
void MultiSplitControl::multisplit_nocap_v_part3(NrnThread* _nt) {
    // at this point, for zero area nodes, D is
    // 1.0, and the voltage is RHS/D).
    // So zero-area node information is fine.
    // But for non-zero area nodes, D is the sum of all zero-area
    // node d, and RHS is the sum of all zero-area node rhs.
    int i;
    auto* const vec_area = _nt->node_area_storage();
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    auto* const vec_v = _nt->node_voltage_storage();
    if (_nt->id == 0)
        for (i = 0; i < narea2buf_; ++i) {
            Area2Buf& ab = area2buf_[i];
            int j = ab.inode;
            double afac = 100. / vec_area[j];
            ab.adjust_rhs_ = (vec_rhs[j] - vec_d[j] * vec_v[j]) * afac;
            // printf("%d nz1 %d D=%g RHS=%g V=%g afac=%g adjust=%g\n",
            // nrnmpi_myid, i, D(i), RHS(i), vec_v[j], afac, ab.adjust_rhs_);
        }
    for (i = 0; i < narea2rt_; ++i) {
        Area2RT& ar = area2rt_[i];
        if (_nt->id == ar.ms->ithread) {
            int j = ar.inode;
            double afac = 100. / vec_area[j];
            ar.adjust_rhs_ = (vec_rhs[j] - vec_d[j] * vec_v[j]) * afac;
            // printf("%d nz2 %d D=%g RHS=%g V=%g afac=%g adjust=%g\n",
            // nrnmpi_myid, i, D(i), RHS(i), vec_v[j], afac, ar.adjust_rhs_);
        }
    }
}

void nrn_multisplit_adjust_rhs(NrnThread* nt) {
    msc_->multisplit_adjust_rhs(nt);
}

void MultiSplitControl::multisplit_adjust_rhs(NrnThread* _nt) {
    int i;
    auto* const vec_rhs = _nt->node_rhs_storage();
    if (_nt->id == 0)
        for (i = 0; i < narea2buf_; ++i) {
            Area2Buf& ab = area2buf_[i];
            vec_rhs[ab.inode] += ab.adjust_rhs_;
        }
    // also scale the non-zero area elements on this host
    for (i = 0; i < narea2rt_; ++i) {
        Area2RT& ar = area2rt_[i];
        if (_nt->id == ar.ms->ithread) {
            // printf("%d adjust %d %g %g\n",
            // nrnmpi_myid, ar.inode, ar.adjust_rhs_, VEC_RHS(ar.inode));
            vec_rhs[ar.inode] += ar.adjust_rhs_;
        }
    }
}

// Two phases: Send all the info from long backbones and receive
// all the short backbone info for this machine.
// Then process the 2x2 short backbone and send remaining (short)
// backbone and receive the rest (long) of the backbone info.

// The ReducedTree solve fits into this strategy receiving from long at the
// same time as receiving from short and
// sending to long at the same time as sending from short

// The MultiSplitTransferInfo order is
// this -> other
// long -> short  0, ihost_long_long-1
// long -> long   ihost_long_long, ihost_long_reduced-1
// long -> reduced ihost_long_reduced, ihost_reduced_long-1
// reduced -> long ihost_reduced_long, ihost_short_long-1
// short -> long  ihost_short_long, nthost_

void MultiSplitControl::matrix_exchange() {
    int i, j, jj, k;
    double* tbuf;
    double rttime;
    double wt = nrnmpi_wtime();
    NrnThread* _nt;
    // the mpi strategy is copied from the
    // cvode/examples_par/pvkxb.cpp exchange strategy

#define EXCHANGE_ON 1
#if EXCHANGE_ON
    // post all the receives
    for (i = 0; i < nthost_; ++i) {
        int tag;
        MultiSplitTransferInfo& mt = msti_[i];
        tag = mt.tag_;
        // receiving the result from a reduced tree is tag 4.
        if (tag == 3 && nrnmpi_myid != mt.rthost_) {
            tag = 4;
        }
        nrnmpi_postrecv_doubles(trecvbuf_ + mt.displ_, mt.size_, mt.host_, tag, &mt.request_);
#if 0
printf("%d post receive %d displ=%d size=%d host=%d tag=%d\n",
nrnmpi_myid, i, mt.displ_, mt.size_, mt.host_, tag);
#endif
    }

    // Note that we are assuming only one thread (_nt) when
    // mpi is used. (In order for D and RHS to make sense)
    // fill the send buffer with the long backbone info
    // i.e. long->short, long -> long, long -> reduced
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        j = 0;
        tbuf = tsendbuf_ + mt.displ_;
        for (jj = 0; jj < mt.nnode_; ++jj) {
            k = mt.nodeindex_[jj];
            _nt = nrn_threads + mt.nodeindex_th_[jj];
            tbuf[j++] = _nt->actual_d(k);
            tbuf[j++] = _nt->actual_rhs(k);
        }
        // each sent backbone will have added 2 to mt.nnode_rt_
        for (jj = 0; jj < mt.nnode_rt_; ++jj) {
            tbuf[j++] = *mt.offdiag_[jj];
        }
#if 0
//if (nrnmpi_myid == 4) {
printf("%d send to %d nnode=%d nnode_rt=%d size=%d tag=%d\n",
nrnmpi_myid, mt.host_, mt.nnode_, mt.nnode_rt_, mt.size_, mt.tag_);
//}
#endif
#if 0
//if (nrnmpi_myid == 4) {
		for (j = 0; j < mt.nnode_; ++j) {
printf("%d send to %d tbuf[%d] = %g  tbuf[%d] = %g from node %d\n",
nrnmpi_myid, mt.host_, 2*j, tbuf[2*j], 2*j+1, tbuf[2*j+1], mt.nodeindex_[j]);
		}
		for (j = 0; j < mt.nnode_rt_; ++j) {
			int jj = 2*mt.nnode_ + j;
printf("%d send to %d offdiag tbuf[%d] = %g\n",
nrnmpi_myid, mt.host_, jj, tbuf[jj]);
		}
//)
#endif
    }

    // adjust area for any D, RHS, sid1A, sid1B going to a ReducedTree host
    for (i = 0; i < narea2buf_; ++i) {
        Area2Buf& ab = area2buf_[i];
        _nt = nrn_threads + ab.ms->ithread;
        double afac = 0.01 * _nt->node_area_storage()[ab.inode];
        tbuf = tsendbuf_;
        for (j = 0; j < ab.n; ++j) {
            tbuf[ab.ibuf[j]] *= afac;
#if 0
printf("%d area2buf * afac=%g i=%d j=%d node=%d ibuf=%d buf=%g\n", nrnmpi_myid, afac,
i, j, ab.inode, ab.ibuf[j], tbuf[ab.ibuf[j]]);
#endif
        }
    }

    // send long backbone info (long->short, long->long)
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        tbuf = tsendbuf_ + mt.displ_;
        nrnmpi_send_doubles(tbuf, mt.size_, mt.host_, mt.tag_);
#if 0
printf("%d post send %d displ=%d size=%d host=%d tag=%d\n",
nrnmpi_myid, i, mt.displ_, mt.size_, mt.host_, mt.tag_);
#endif
    }

#if 0
	for (i=ihost_reduced_long_; i < nthost_; ++i) {
		MultiSplitTransferInfo& mt = msti_[i];
printf("%d will send result to %d size=%d tag=%d\n", nrnmpi_myid, mt.host_, mt.size_, mt.tag_);
	}
#endif
    // handle the reduced trees and short backbones

    // wait for receives from the long backbones needed by reduced tree
    // and short backbones to complete (reduced <- long, short <- long)
    for (i = ihost_reduced_long_; i < nthost_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
#if 0
printf("%d wait one %d\n", nrnmpi_myid, mt.host_);
#endif
        nrnmpi_wait(&mt.request_);
#if 0
printf("%d receive from %d nnode=%d nnode_rt=%d size=%d tag=%d displ=%d\n",
nrnmpi_myid, mt.host_, mt.nnode_, mt.nnode_rt_, mt.size_, mt.tag_, mt.displ_);
//for (j=0; j < mt.size_; ++j) { printf("%d receive tbuf[%d]=%g\n", nrnmpi_myid, j, trecvbuf_[mt.displ_ + j]);}
#endif
    }
#if 0
for (i=0; i < tbsize_; ++i) { printf("%d trecvbuf[%d] = %g\n", nrnmpi_myid, i, trecvbuf_[i]);}
#endif

    // measure reducedtree,short backbone computation time
    rttime = nrnmpi_wtime();

    // adjust area in place for any D, RHS, sid1A, sid1B on this host
    // going to ReducedTree on this host
    // if (narea2rt_) printf("%d adjust area in place\n", nrnmpi_myid);
    for (i = 0; i < narea2rt_; ++i) {
        Area2RT& ar = area2rt_[i];
        NrnThread* _nt = nrn_threads + ar.ms->ithread;
        double afac = 0.01 * _nt->node_area_storage()[ar.inode];
        for (j = 0; j < ar.n; ++j) {
            *ar.pd[j] *= afac;
        }
    }
#endif  // EXCHANGE_ON

    for (i = 0; i < nrtree_; ++i) {
        rtree_[i]->solve();
    }

#if EXCHANGE_ON
    // measure reducedtree,short backbone computation time
    nrnmpi_rtcomp_time_ += nrnmpi_wtime() - rttime;

    // send reduced and short backbone info (reduced -> long, short -> long)
    for (i = ihost_reduced_long_; i < nthost_; ++i) {
        int tag;
        MultiSplitTransferInfo& mt = msti_[i];
        tbuf = tsendbuf_ + mt.displ_;
        // printf("%d send result to %d size=%d tag=%d\n", nrnmpi_myid, mt.host_, mt.size_,
        // mt.tag_);
        tag = mt.tag_;
        // sending result from reduced tree means tag is 4
        if (tag == 3) {
            tag = 4;
        }
        nrnmpi_send_doubles(tbuf, mt.size_, mt.host_, tag);
        // only moderately wasteful (50%) in sending back irrelevant
        // off diag info. Required is only 2*mt.nnode_. But these
        // messages are pretty short, anyway.
    }

    // handle the long backbones

    // wait for all remaining receives to complete
    // long <- short, long <- long, long <- reduced
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
#if 0
printf("%d wait two %d\n", nrnmpi_myid, mt.host_);
#endif
        nrnmpi_wait(&mt.request_);
#if 0
printf("%d receive from %d nnode=%d nnode_rt=%d size=%d tag=%d\n",
nrnmpi_myid, mt.host_, mt.nnode_, mt.nnode_rt_, mt.size_, mt.tag_);
#endif
    }

    // add to matrix (long <- long)
    // and also (long <- short) even though in principle should
    // not add those since already solved on short backbone machine
    // we can add since the ones sent were scaled by 1e30 which
    // will eliminate the effect of existing D, RHS, and S1A or S1B
    // as well as making the possible area scaling irrelevant
    // being able to handle them together means we do not need
    // 0 to ihost_long_long - 1  to refer to long <-> short
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        j = 0;
        tbuf = trecvbuf_ + mt.displ_;
        for (jj = 0; jj < mt.nnode_; ++jj) {
            k = mt.nodeindex_[jj];
            _nt = nrn_threads + mt.nodeindex_th_[jj];
            _nt->actual_d(k) += tbuf[j++];
            _nt->actual_rhs(k) += tbuf[j++];
        }
#if 0
if (nrnmpi_myid == 4) {
		for (j = 0; j < mt.nnode_; ++j) {
printf("%d received from %d tbuf[%d] = %g  tbuf[%d] = %g added to node %d\n",
nrnmpi_myid, mt.host_, 2*j, tbuf[2*j], 2*j+1, tbuf[2*j+1], mt.nodeindex_[j]);
		}
}
#endif
    }
#endif  // EXCHANGE_ON

#if NRNMPI
    nrnmpi_splitcell_wait_ += nrnmpi_wtime() - wt;
#endif
    errno = 0;
}

void MultiSplitControl::matrix_exchange_nocap() {  // copy of matrix_exchange() and modified
    int i, j, jj, k;
    double* tbuf;
    double rttime;
    double wt = nrnmpi_wtime();
    NrnThread* _nt;
    // the mpi strategy is copied from the
    // cvode/examples_par/pvkxb.cpp exchange strategy

#define EXCHANGE_ON 1
#if EXCHANGE_ON
    // post all the receives
    for (i = 0; i < nthost_; ++i) {
        int tag;
        MultiSplitTransferInfo& mt = msti_[i];
        tag = mt.tag_;
        // receiving the result from a reduced tree is tag 4.
        if (tag == 3 && nrnmpi_myid != mt.rthost_) {
            tag = 4;
        }
        nrnmpi_postrecv_doubles(trecvbuf_ + mt.displ_, mt.size_, mt.host_, tag, &mt.request_);
#if 0
printf("%d post receive %d displ=%d size=%d host=%d tag=%d\n",
nrnmpi_myid, i, mt.displ_, mt.size_, mt.host_, tag);
#endif
    }

    // fill the send buffer with the long backbone info
    // i.e. long->short, long -> long, long -> reduced
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        j = 0;
        tbuf = tsendbuf_ + mt.displ_;
        for (jj = 0; jj < mt.nnode_; ++jj) {
            k = mt.nodeindex_[jj];
            _nt = nrn_threads + mt.nodeindex_th_[jj];
            tbuf[j++] = _nt->actual_d(k);
            tbuf[j++] = _nt->actual_rhs(k);
        }
        // each sent backbone will have added 2 to mt.nnode_rt_
        for (jj = 0; jj < mt.nnode_rt_; ++jj) {
            tbuf[j++] = *mt.offdiag_[jj];
        }
#if 0
//if (nrnmpi_myid == 4) {
printf("%d send to %d nnode=%d nnode_rt=%d size=%d tag=%d\n",
nrnmpi_myid, mt.host_, mt.nnode_, mt.nnode_rt_, mt.size_, mt.tag_);
//}
#endif
#if 0
//if (nrnmpi_myid == 4) {
		for (j = 0; j < mt.nnode_; ++j) {
printf("%d send to %d tbuf[%d] = %g  tbuf[%d] = %g from node %d\n",
nrnmpi_myid, mt.host_, 2*j, tbuf[2*j], 2*j+1, tbuf[2*j+1], mt.nodeindex_[j]);
		}
		for (j = 0; j < mt.nnode_rt_; ++j) {
			int jj = 2*mt.nnode_ + j;
printf("%d send to %d offdiag tbuf[%d] = %g\n",
nrnmpi_myid, mt.host_, jj, tbuf[jj]);
		}
//}
#endif
    }

    // send long backbone info (long->short, long->long)
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        tbuf = tsendbuf_ + mt.displ_;
        nrnmpi_send_doubles(tbuf, mt.size_, mt.host_, mt.tag_);
#if 0
printf("%d post send %d displ=%d size=%d host=%d tag=%d\n",
nrnmpi_myid, i, mt.displ_, mt.size_, mt.host_, mt.tag_);
#endif
    }

#if 0
	for (i=ihost_reduced_long_; i < nthost_; ++i) {
		MultiSplitTransferInfo& mt = msti_[i];
printf("%d will send result to %d size=%d tag=%d\n", nrnmpi_myid, mt.host_, mt.size_, mt.tag_);
	}
#endif
    // handle the reduced trees and short backbones

    // wait for receives from the long backbones needed by reduced tree
    // and short backbones to complete (reduced <- long, short <- long)
    for (i = ihost_reduced_long_; i < nthost_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
#if 0
printf("%d wait one %d\n", nrnmpi_myid, mt.host_);
#endif
        nrnmpi_wait(&mt.request_);
#if 0
printf("%d receive from %d nnode=%d nnode_rt=%d size=%d tag=%d displ=%d\n",
nrnmpi_myid, mt.host_, mt.nnode_, mt.nnode_rt_, mt.size_, mt.tag_, mt.displ_);
//for (j=0; j < mt.size_; ++j) { printf("%d receive tbuf[%d]=%g\n", nrnmpi_myid, j, trecvbuf_[mt.displ_ + j]);}
#endif
    }
#if 0
for (i=0; i < tbsize_; ++i) { printf("%d trecvbuf[%d] = %g\n", nrnmpi_myid, i, trecvbuf_[i]);}
#endif

    // measure reducedtree,short backbone computation time
    rttime = nrnmpi_wtime();

#endif  // EXCHANGE_ON

    // remember V may be encoded in D
    for (i = 0; i < nrtree_; ++i) {
        rtree_[i]->nocap();
    }

#if EXCHANGE_ON
    // replace in matrix
    // for zero area nodes, D is 1.0 and RHS is V from zero area node.
    // for non-zero area nodes, they are sum from zero area.
    for (i = ihost_short_long_; i < nthost_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        j = 0;
        tbuf = trecvbuf_ + mt.displ_;
        for (jj = 0; jj < mt.nnode_; ++jj) {
            k = mt.nodeindex_[jj];
            _nt = nrn_threads + mt.nodeindex_th_[jj];
            _nt->actual_d(k) = tbuf[j++];
            _nt->actual_rhs(k) = tbuf[j++];
        }
#if 0
		for (j = 0; j < mt.nnode_; ++j) {
printf("%d received from %d tbuf[%d] = %g  tbuf[%d] = %g added to node %d\n",
nrnmpi_myid, mt.host_, 2*j, tbuf[2*j], 2*j+1, tbuf[2*j+1], mt.nodeindex_[j]);
		}
#endif
    }
    // measure reducedtree,short backbone computation time
    nrnmpi_rtcomp_time_ += nrnmpi_wtime() - rttime;

    // send reduced and short backbone info (reduced -> long, short -> long)
    for (i = ihost_reduced_long_; i < nthost_; ++i) {
        int tag;
        MultiSplitTransferInfo& mt = msti_[i];
        tbuf = tsendbuf_ + mt.displ_;
        // printf("%d send result to %d size=%d tag=%d\n", nrnmpi_myid, mt.host_, mt.size_,
        // mt.tag_);
        tag = mt.tag_;
        // sending result from reduced tree means tag is 4
        if (tag == 3) {
            tag = 4;
        }
        nrnmpi_send_doubles(tbuf, mt.size_, mt.host_, tag);
        // only moderately wasteful (50%) in sending back irrelevant
        // off diag info. Required is only 2*mt.nnode_. But these
        // messages are pretty short, anyway.
    }

    // handle the long backbones

    // wait for all remaining receives to complete
    // long <- short, long <- long, long <- reduced
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
#if 0
printf("%d wait two %d\n", nrnmpi_myid, mt.host_);
#endif
        nrnmpi_wait(&mt.request_);
#if 0
printf("%d receive from %d nnode=%d nnode_rt=%d size=%d tag=%d\n",
nrnmpi_myid, mt.host_, mt.nnode_, mt.nnode_rt_, mt.size_, mt.tag_);
#endif
    }

    // replace in matrix
    // for zero area nodes, D is 1.0 and RHS is V from zero area node.
    // for non-zero area nodes, they are sum from zero area.
    for (i = 0; i < ihost_reduced_long_; ++i) {
        MultiSplitTransferInfo& mt = msti_[i];
        j = 0;
        tbuf = trecvbuf_ + mt.displ_;
        for (jj = 0; jj < mt.nnode_; ++jj) {
            k = mt.nodeindex_[jj];
            _nt = nrn_threads + mt.nodeindex_th_[jj];
            _nt->actual_d(k) = tbuf[j++];
            _nt->actual_rhs(k) = tbuf[j++];
        }
#if 0
if (nrnmpi_myid == 4) {
		for (j = 0; j < mt.nnode_; ++j) {
printf("%d received from %d tbuf[%d] = %g  tbuf[%d] = %g added to node %d\n",
nrnmpi_myid, mt.host_, 2*j, tbuf[2*j], 2*j+1, tbuf[2*j+1], mt.nodeindex_[j]);
		}
}
#endif
    }
#endif  // EXCHANGE_ON

#if NRNMPI
    nrnmpi_splitcell_wait_ += nrnmpi_wtime() - wt;
#endif
    errno = 0;
}

ReducedTree::ReducedTree(MultiSplitControl* ms, int rank, int mapsize) {
    // printf("ReducedTree::ReducedTree(%d, %d)\n", rank, mapsize);
    int i;
    msc = ms;
    n = rank;
    assert(n > 0);
    assert(mapsize > 0);
    ip = new int[n];
    rhs = new double[4 * n];
    d = rhs + n;
    a = d + n;
    b = a + n;

    n2 = 2 * n;
    n4 = 4 * n;
    nmap = mapsize;
    smap = new double*[nmap];
    rmap = new double*[nmap];
    ismap = new int[nmap];
    irmap = new int[nmap];
    nzindex = new int[n];
    rmap2smap_index = new int[nmap];
    v = new double[n];
    nsmap = 0;
    irfill = 0;
    for (i = 0; i < nmap; ++i) {
        smap[i] = 0;
        ismap[i] = -1;
        rmap[i] = 0;
        irmap[i] = -1;
        rmap2smap_index[i] = -1;
    }
}

ReducedTree::~ReducedTree() {
    delete[] ip;
    delete[] rhs;
    delete[] smap;
    delete[] ismap;
    delete[] rmap;
    delete[] irmap;
    delete[] nzindex;
    delete[] v;
    delete[] rmap2smap_index;
}

void ReducedTree::nocap() {
    // remember that info from zero area nodes is D=d and RHS=rhs
    // and from non-zero area is D=1e50 and RHS=v*1e50
    // on return, the info for zero area if no nonzero should be
    // D=sum d and RHS=sum rhs
    // the info for zero area if nonzero should be D=1.0 and RHS=v
    // and the info for nonzero should be D=sum d and RHS=sum rhs.
    int i;
    for (i = 0; i < n; ++i) {
        rhs[i] = 0.0;
        d[i] = 0.0;
        nzindex[i] = -1;
    }
    // not supposed to add when D=1e50. Note that order in rmap is
    // rhs followed by d
    for (i = 0; i < nmap; i += 2) {
        int j = irmap[i];  // reduced matrix row index for rhs
        if (*rmap[i + 1] == 1e50) {
            v[j] = *rmap[i] * 1e-50;
            nzindex[j] = rmap2smap_index[i];
        } else {
            rhs[j] += *rmap[i];
            d[j] += *rmap[i + 1];
        }
    }
#if 0
	for (i=0; i < n; ++i) {
		printf("%d ReducedTree %2d %12.5g %12.5g      %d %12.5g\n",
		nrnmpi_myid, i, d[i], rhs[i], nzindex[i], v[i]);
	}
#endif
    for (i = 0; i < nsmap; i += 2) {
        int j = ismap[i];  // reduced matrix matrix row index for rhs
        if (nzindex[j] == -1 || i == nzindex[j]) {
            // for the non-zero area nodes and zero area nodes
            // when no non-zero area node involved
            *smap[i] = rhs[j];
            *smap[i + 1] = d[j];
        } else {
            // for the zero area nodes
            *smap[i] = v[j];
            *smap[i + 1] = 1.0;
        }
    }
}

void ReducedTree::solve() {
    int i;
    double p;
    gather();
#if 0
	for (i=0; i < n; ++i) {
		printf("%d ReducedTree %2d %12.5g %12.5g %2d %12.5g   %12.5g\n",
		nrnmpi_myid, i, b[i], d[i], ip[i], ip[i] >= 0?a[i]:0., rhs[i]);
	}
#endif
    // triangularization
    for (i = n - 1; i > 0; --i) {
        p = a[i] / d[i];
        d[ip[i]] -= p * b[i];
        rhs[ip[i]] -= p * rhs[i];
    }
    // back substitution
    rhs[0] /= d[0];
    for (i = 1; i < n; ++i) {
        rhs[i] -= b[i] * rhs[ip[i]];
        rhs[i] /= d[i];
    }
#if 0
	for (i=0; i < n; ++i) {
		printf("%d Result %2d %12.5g\n", nrnmpi_myid, i, rhs[i]);
	}
#endif
    scatter();
}

void ReducedTree::gather() {
    int i;
    for (i = 0; i < n4; ++i) {
        rhs[i] = 0.0;
    }
    for (i = 0; i < nmap; ++i) {
        rhs[irmap[i]] += *rmap[i];
    }
#if 0
for (i=0; i < nmap; ++i){
printf("%d %p gather i=%d ie=%d %g\n", nrnmpi_myid, this, i, irmap[i], *rmap[i]);
}
#endif
}

void ReducedTree::scatter() {
    int i;
    // scatter the result for this host
    // fill the send buffer for other hosts
#if 0
for (i=0; i < nsmap; i += 2) {
printf("%d enter scatter %d %p %g %p %g\n", nrnmpi_myid, i,
smap[i], *smap[i], smap[i+1], *smap[i+1]);
}
#endif
    // printf("nsmap=%d\n", nsmap);
    for (i = 0; i < nsmap; i += 2) {
        // printf("i=%d ismap[i]=%d\n", i, ismap[i]);
        *smap[i] = 1e30 * rhs[ismap[i]];
        *smap[i + 1] = 1e30;
    }
#if 0
for (i=0; i < nsmap; i += 2) if (i > 10){
printf("%d leave scatter %d %p %g %p %g\n", nrnmpi_myid, i,
smap[i], *smap[i], smap[i+1], *smap[i+1]);
}
#endif
}

void ReducedTree::pr_map(int tsize, double* trbuf) {
    int i;
    Printf("  rmap\n");
    for (i = 0; i < nmap; ++i) {
        for (int it = 0; it < nrn_nthread; ++it) {
            NrnThread* nt = nrn_threads + it;
            MultiSplitThread& t = msc_->mth_[it];
            int nb = t.backbone_end - t.backbone_begin;
            if (rmap[i] >= trbuf && rmap[i] < (trbuf + tsize)) {
                Printf(" %2d rhs[%2d] += tbuf[%ld]\n", i, irmap[i], rmap[i] - trbuf);
            }
            if (rmap[i] >= nt->node_rhs_storage() && rmap[i] < (nt->node_rhs_storage() + nt->end)) {
                Node* nd = nt->_v_node[rmap[i] - nt->node_rhs_storage()];
                Printf(" %2d rhs[%2d] rhs[%d] += rhs[%ld] \t%s{%d}\n",
                       i,
                       irmap[i],
                       irmap[i],
                       rmap[i] - nt->node_rhs_storage(),
                       secname(nd->sec),
                       nd->sec_node_index_);
            }
            if (rmap[i] >= nt->node_d_storage() && rmap[i] < (nt->node_d_storage() + nt->end)) {
                Printf(" %2d rhs[%2d]   d[%d] += d[%ld]\n",
                       i,
                       irmap[i],
                       irmap[i] - n,
                       rmap[i] - nt->node_d_storage());
            }
            if (rmap[i] >= t.sid1A && rmap[i] < (t.sid1A + nb)) {
                Printf(" %2d rhs[%2d]   a[%d] += sid1A[%ld]",
                       i,
                       irmap[i],
                       irmap[i] - 2 * n,
                       rmap[i] - t.sid1A);
                int j = (rmap[i] - t.sid1A) + t.backbone_begin;
                Node* nd = nt->_v_node[j];
                Printf(" \tA(%d) %s{%d}", j, secname(nd->sec), nd->sec_node_index_);
                Printf("\n");
            }
            if (rmap[i] >= t.sid1B && rmap[i] < (t.sid1B + nb)) {
                Printf(" %2d rhs[%2d]   b[%d] += sid1B[%ld]",
                       i,
                       irmap[i],
                       irmap[i] - 3 * n,
                       rmap[i] - t.sid1B);
                int j = (rmap[i] - t.sid1B) + t.backbone_begin;
                Node* nd = nt->_v_node[j];
                Printf("\tB(%d) %s{%d}", j, secname(nd->sec), nd->sec_node_index_);
                Printf("\n");
            }
        }
    }
}

void ReducedTree::reorder(int j, int nt, int* mark, int* allbbr, int* allsid) {
    // specify ip and modify s2rt so they have tree order consistency
    // there should be n-1 edges, so
    if (n == 1) {
        ip[0] = -1;
        return;
    }
    int* e1 = new int[n - 1];
    int* e2 = new int[n - 1];
    int* order = new int[n];
    int* sid = new int[n];
    int singlesid = -1;
    int i;
    for (i = 0; i < n; ++i) {
        order[i] = -1;
    }
    // fill in the edges.
    int ne = n - 1;
    int ie = 0;
    for (i = 0; i < nt; ++i) {
        if (mark[i] == j && allbbr[i] == 2) {
            singlesid = allsid[i];
        }
        if (mark[i] == j && allbbr[i] > 2 && allsid[i] < allbbr[i] - 3) {
            // printf("i=%d ie=%d ne=%d mark=%d allsid=%d allbbr=%d\n", i, ie, ne, mark[i],
            // allsid[i], allbbr[i]-3);
            assert(ie < ne);
            const auto& e1ieiter = s2rt->find(allsid[i]);
            nrn_assert(e1ieiter != s2rt->end());
            e1[ie] = e1ieiter->second;
            sid[e1[ie]] = allsid[i];
            const auto& e2ieiter = s2rt->find(allbbr[i] - 3);
            nrn_assert(e2ieiter != s2rt->end());
            e2[ie] = e2ieiter->second;
            sid[e2[ie]] = allbbr[i] - 3;
            ++ie;
        }
    }
    assert(ie == ne);
    if (ne == 0) {
        assert(singlesid >= 0);
        sid[0] = singlesid;
    }
    // order the elements starting at 0 // also fill in the ip vector
    // should replace by a O(n) or at worst a O(nlogn) algorithm
    ip[0] = -1;
    order[0] = 0;
    int ordered = 1;
    while (ordered < n) {
        int old = ordered;
        for (i = 0; i < ne; ++i) {
            if (e1[i] >= 0) {
                if (order[e1[i]] >= 0) {
                    assert(order[e2[i]] == -1);
                    ip[ordered] = order[e1[i]];
                    order[e2[i]] = ordered++;
                    e1[i] = -1;  // edge handled
                    e2[i] = -1;
                } else if (order[e2[i]] >= 0) {
                    assert(order[e1[i]] == -1);
                    ip[ordered] = order[e2[i]];
                    order[e1[i]] = ordered++;
                    e1[i] = -1;  // edge handled
                    e2[i] = -1;
                }
            }
        }
        assert(ordered > old);  // if tree then accomplished something
    }

    // reset the s2rt ranks for the sids
    for (i = 0; i < n; ++i) {
        (*s2rt)[sid[i]] = order[i];
    }

#if 0
	printf("%d ReducedTree n=%d nmap=%d\n", nrnmpi_myid, n, nmap);
	for (i=0; i < n; ++i) {
		printf("%d i=%d  sid=%d sid2i=%d\n", nrnmpi_myid, i, sid[i], (*s2rt)[sid[i]]);
	}
	for (i=0; i < n; ++i) {
		printf("%d i=%d ip=%d\n", nrnmpi_myid, i, ip[i]);
	}
#endif

    delete[] e1;
    delete[] e2;
    delete[] order;
    delete[] sid;
}

void ReducedTree::fillrmap(int sid1, int sid2, double* pd) {
    const auto& sid1_iter = s2rt->find(sid1);
    nrn_assert(sid1_iter != s2rt->end());
    const int i = sid1_iter->second;
    int j;

    // type order is RHS, D, A, B
    if (sid2 < 0) {  // RHS
        j = i;
    } else if (sid2 == sid1) {  // D
        j = i + n;
    } else {  // A or B?
        const auto& sid2_iter = s2rt->find(sid2);
        nrn_assert(sid2_iter != s2rt->end());
        j = sid2_iter->second;
        if (ip[i] == j) {  // A
            j = i + 2 * n;
        } else if (ip[j] == i) {  // B
            j = j + 3 * n;
        } else {
            assert(0);
        }
    }
    // in most cases we could have done RHS and D together since they
    // are adjacent in the receive buffer. However they are not adjacent
    // for the sids on this machine.
    // printf("%d fillrmap sid1=%d sid2=%d i=%d j=%d irfill=%d\n", nrnmpi_myid,  sid1, sid2, i, j,
    // irfill);
    irmap[irfill] = j;
    rmap[irfill] = pd;
    rmap2smap_index[irfill] = nsmap;
    irfill += 1;
}

void ReducedTree::fillsmap(int sid, double* prhs, double* pd) {
    const auto& sid_iter = s2rt->find(sid);
    nrn_assert(sid_iter != s2rt->end());
    const int i = sid_iter->second;

    // printf("%d fillsmap sid=%d i=%d nsmap=%d\n", nrnmpi_myid, sid, i, nsmap);
    ismap[nsmap] = i;
    smap[nsmap] = prhs;
    ismap[nsmap + 1] = i;
    smap[nsmap + 1] = pd;
    nsmap += 2;
}

// triangularize all subtrees, results in a backbone tri-diagonal
void MultiSplitThread::triang_subtree2backbone(NrnThread* _nt) {
    int i, ip;
    double p;
    auto* const vec_a = _nt->node_a_storage();
    auto* const vec_b = _nt->node_b_storage();
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    // eliminate a of the subtrees
    for (i = i3 - 1; i >= backbone_end; --i) {
        ip = _nt->_v_parent_index[i];
        p = vec_a[i] / vec_d[i];
        vec_d[ip] -= p * vec_b[i];
        RHS(ip) -= p * RHS(i);
    }
#if 0
printf("end of triang_subtree2backbone\n");
for (i=i1; i < backbone_end; ++i) {
	printf("i=%d D=%g RHS=%g\n", i, D(i), RHS(i));
}
#endif
}

// backbone starts as tridiagonal, ends up in form of N.
void MultiSplitThread::triang_backbone(NrnThread* _nt) {
    int i, j, ip, jp;
    double p;
    // begin the backbone triangularization. This eliminates a and fills in
    // sid1A column. Begin with pivot equation adjacent to sid1.
    auto* const vec_a = _nt->node_a_storage();
    for (i = backbone_sid1_begin; i < backbone_end; ++i) {
        // what is the equation index for A(i)
        j = _nt->_v_parent_index[i] - backbone_begin;
        S1A(j) = vec_a[i];
    }
    auto* const vec_b = _nt->node_b_storage();
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    for (i = backbone_sid1_begin - 1; i >= backbone_interior_begin; --i) {
        ip = _nt->_v_parent_index[i];
        j = i - backbone_begin;
        jp = ip - backbone_begin;
        p = vec_a[i] / D(i);
        D(ip) -= p * vec_b[i];
        RHS(ip) -= p * RHS(i);
        S1A(jp) = -p * S1A(j);
        // printf("iter i=%d ip=%d j=%d jp=%d D(ip)=%g RHS(ip)=%g S1A(ip)=%g\n",
        // i, ip, j, jp, D(ip), RHS(ip), S1A(jp));
    }

    // complete the backbone triangularization to form the N shaped matrix
    // This eliminates b and fills in the sid1B column. Begin with
    // pivot equation adjacent to sid0 (the root)
    for (i = backbone_interior_begin; i < backbone_sid1_begin; ++i) {
        ip = _nt->_v_parent_index[i];
        j = i - backbone_begin;
        if (ip < backbone_interior_begin) {
            S1B(j) = vec_b[i];
            continue;
        }
        jp = ip - backbone_begin;
        p = vec_b[i] / D(ip);
        RHS(i) -= p * RHS(ip);
        S1A(j) -= p * S1A(jp);
        S1B(j) = -p * S1B(jp);
        // printf("iter2 i=%d j=%d D(i)=%g RHS(i)=%g S1A(j)=%g S1B(j)=%g\n",
        // i, j, D(i), RHS(i), S1A(j), S1B(j));
    }
    // exactly like above but S1A happens to be D
    for (i = backbone_sid1_begin; i < backbone_end; ++i) {
        ip = _nt->_v_parent_index[i];
        j = i - backbone_begin;
        if (ip < backbone_interior_begin) {
            S1B(j) = vec_b[i];
            continue;
        }
        jp = ip - backbone_begin;
        p = vec_b[i] / D(ip);
        RHS(i) -= p * RHS(ip);
        D(i) -= p * S1A(jp);
        S1B(j) = -p * S1B(jp);
        // printf("iter3 i=%d j=%d D(i)=%g RHS(i)=%g S1B(j)=%g\n",
        // i, j, D(i), RHS(i), S1B(j));
    }
#if 0
printf("%d end of triang_backbone\n", nrnmpi_myid);
for (i=i1; i < backbone_end; ++i) {
	printf("%d i=%d D=%g RHS=%g\n", nrnmpi_myid, i, D(i), RHS(i));
}
#endif
}

// exchange of d and rhs of sids has taken place and we can solve for the
// backbone nodes
void MultiSplitThread::bksub_backbone(NrnThread* _nt) {
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    int i, j;
    double a, b, p, vsid1;
    // need to solve the 2x2 consisting of sid0 and sid1 points
    // this part has already been done for short backbones
    j = backbone_long_sid1_begin;
    // printf("%d, backbone %d %d %d %d %d %d\n", nrnmpi_myid,
    // backbone_begin, backbone_long_begin, backbone_interior_begin,
    // backbone_sid1_begin, backbone_long_sid1_begin, backbone_end);
    for (i = backbone_long_begin; i < backbone_interior_begin; ++i) {
        a = S1A(i - backbone_begin);
        b = S1B(j - backbone_begin);
        p = b / D(i);
        D(j) -= p * a;
        RHS(j) -= p * RHS(i);
        RHS(j) /= D(j);
        RHS(i) -= a * RHS(j);
        RHS(i) /= D(i);
        ++j;
    }
    // now sid0 and sid1 values give us the column values
    // do in two sweeps. Wish we could keep a single cell backbone
    // contiguous.
    // Do the S1A sweep on a per cell basis.
    for (i = backbone_sid1_begin; i < backbone_end; ++i) {
        vsid1 = RHS(i);
        for (j = _nt->_v_parent_index[i]; j >= backbone_interior_begin;
             j = _nt->_v_parent_index[j]) {
            RHS(j) -= S1A(j - backbone_begin) * vsid1;
        }
    }
    // For the S1B sweep we use sid0 root info for each interior node
    for (i = backbone_interior_begin; i < backbone_sid1_begin; ++i) {
        j = i - backbone_begin;
        RHS(i) -= S1B(j) * RHS(sid0i[j]);
        RHS(i) /= D(i);
    }
#if 0
printf("%d end of bksub_backbone\n", nrnmpi_myid);
for (i=i1; i < backbone_end; ++i) {
	printf("%d i=%d RHS=%g\n", nrnmpi_myid, i, RHS(i));
}
#endif
}

void MultiSplitThread::bksub_short_backbone_part1(NrnThread* _nt) {
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    int i, j;
    double a, b, p;
    // solve the 2x2 consisting of sid0 and sid1 points.
    j = backbone_sid1_begin;
    for (i = backbone_begin; i < backbone_long_begin; ++i) {
        a = S1A(i - backbone_begin);
        b = S1B(j - backbone_begin);
#if 0
printf("%d part1 i=%d j=%d\n",
nrnmpi_myid, i, j);
printf("%d part1 d=%12.5g a=%12.5g rhs=%12.5g\n",
nrnmpi_myid,D(i), a, RHS(i));
printf("%d part1 b=%12.5g d=%12.5g rhs=%12.5g\n",
nrnmpi_myid, b, D(j), RHS(j));
#endif
        p = b / D(i);
        D(j) -= p * a;
        RHS(j) -= p * RHS(i);
        RHS(j) /= D(j);
        RHS(i) -= a * RHS(j);
        RHS(i) /= D(i);
#if 0
printf("%d part1 result %12.5g %12.5g\n",
nrnmpi_myid, RHS(i), RHS(j));
#endif
        ++j;
    }
    // Note that it no longer makes sense to add D and RHS
    // to the other machines since RHS is the solution itself
    // at the other machine corresponding points. In fact
    // we don't need to send D at all. Just replace the
    // RHS, skip that equation, and go on. But for
    // code uniformity and since skipping is more complex
    // it seems simpler to just setup the right equation
    // on the other machine. i.e. 1 * v = rhs
}

// solve the subtrees,  rhs on the backbone is already solved
void MultiSplitThread::bksub_subtrees(NrnThread* _nt) {
    auto* const vec_b = _nt->node_b_storage();
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    int i, ip;
    // solve all rootnodes not part of a backbone
    for (i = i1; i < backbone_begin; ++i) {
        RHS(i) /= D(i);
    }
    // solve the subtrees
    for (i = backbone_end; i < i3; ++i) {
        ip = _nt->_v_parent_index[i];
        RHS(i) -= vec_b[i] * RHS(ip);
        RHS(i) /= D(i);
    }
#if 0
printf("%d end of bksub_subtrees (just the backbone)\n", nrnmpi_myid);
for (i=i1; i < backbone_end; ++i) {
	printf("%d i=%d RHS=%g\n", nrnmpi_myid, i, RHS(i));
}
#endif
}

// fill the v_node, v_parent node vectors in the proper order and
// determine backbone index values. See the NOTE above. The relevant order
// is:
// 1) all the roots of cells with 0 or 1 sid (no backbone involved)
// 2) all the sid0 when there are two sids in a cell. 1) + 2) = rootnodecount
// 3) the interior backbone nodes (does not include sid0 or sid1 nodes)
// 4) all the sid1 nodes
// 5) all remaining nodes
// backbone_begin is the index for 2).
// backbone_long_begin ; earlier are short backbones, later are long
// backbone_interior_begin is one more than the last index for 2)
// backbone_sid1_begin is the index for 4)
// backbone_long_sid1_begin ; ealier are short, later are long
// backbone_end is the index for 5.
// We allow the single or pair sid nodes to be different from a rootnode.
// Note: generally speaking, the first sid will probably always be
// a classical root node. However, we do want to handle the case where
// many branches connect to soma(0.5), so do the whole tree ordering
// as the general case.

void multisplit_v_setup() {
    msc_->v_setup();
}

void MultiSplitControl::v_setup() {
    if (!classical_root_to_multisplit_) {
        return;
    }
    // the typical case is that this comes from the beginning of exchange_setup
    // through recalc_diam since exhange_setup needs the proper
    // thread nt->_v_node and ms->ithread. Hence anything that
    // changes the overall structure
    // requires a complete start over from the point prior to splitting.
    assert(!use_sparse13);
    int i;
    // first time through, nth_ = 0
    if (nth_ == 0) {
        // printf("MultiSplitControl::v_setup due to multisplit()\n");
        assert(mth_ == 0);
        nth_ = nrn_nthread;
        mth_ = new MultiSplitThread[nth_];
        for (i = 0; i < nrn_nthread; ++i) {
            mth_[i].v_setup(nrn_threads + i);
        }
    } else {
        if (nth_ != nrn_nthread) {
            hoc_execerror(
                "ParallelContext.nthread() was changed after ParallelContext.multisplit()", 0);
        }
        // pointers have changed and need to reorder and update reduced tree maps
        // printf("MultiSplitControl::v_setup due to pointer change\n");
        for (i = 0; i < nrn_nthread; ++i) {
            mth_[i].v_setup(nrn_threads + i);
        }
    }
}

void MultiSplitThread::v_setup(NrnThread* nt) {
    // Precondition:
    // v_node and v_parent node vectors are in their classical tree order
    // and Node.v_node_index also is consistent with that.
    // calls to pc.multisplit() define the implicit connections
    // between splitcell
    // Postcondition: v_node and v_parent node vectors are in their multisplit
    // tree order. That info can be subsequently used to re-allocate
    // the actual v
    i1 = 0;
    i2 = i1 + nt->ncell;
    i3 = nt->end;
    int nnode = i3 - i1;

    MultiSplitTable* classical_root_to_multisplit_ = msc_->classical_root_to_multisplit_.get();
    MultiSplitList* multisplit_list_ = msc_->multisplit_list_;

    // node vs v_node relation is always with an i1 offset
    Node** node = new Node*[nnode];
    Node** parent = new Node*[nnode];
    Node* nd;
    int i, j, i0, ii, in, ip, nback, ib, ibl, ibs;
    int is0, is1, k0, k1, iss0, iss1, isl0, isl1;

#if 0
printf("multisplit_v_setup %d\n", nnode);
	for (i=i1; i < i3; ++i) {
		assert(nt->_v_node[i]->v_node_index == i);
		assert(nt->_v_parent[i] == 0 || nt->_v_parent[i]->v_node_index < i);
	}
#endif
#if 0
printf("multisplit_v_setup %d\n", nnode);
printf("\nclassical order\n");
for (i=i1; i < i3; ++i) {
	printf("%d %d %d %s %d\n", i, nt->_v_node[i]->v_node_index,
		nt->_v_parent[i] ? nt->_v_parent[i]->v_node_index : -1, secname(nt->v_node[i]->sec),
		nt->_v_node[i]->sec_node_index_);
}
#endif
    del_sidA();

    // the first phase is to transform from the classical tree order
    // to a tree order where the sid0 are roots. In splitcell, this
    // was done at the hoc level (rerooting a tree involves reversing
    // the parent child relationship on the line between the old and
    // new roots). However at the hoc level it is only possible to
    // have split points at section boundaries and
    // that meant that soma(0.5) could not
    // be a root. We want to allow that. It is also much simpler
    // to deal with the backbone between sid0 and sid1
    // if sid0 is already a root.

    // how many sid0 backbone (long and short) roots are there
    backbone_begin = i2;
    backbone_long_begin = backbone_begin;
    if (classical_root_to_multisplit_) {
        for (MultiSplit* ms: *multisplit_list_) {
            if (ms->nd[1]) {
                if (ms->nd[1]->_nt != nt) {
                    continue;
                }
                --backbone_begin;
                if (ms->backbone_style != 1) {
                    --backbone_long_begin;
                }
                if (ms->backbone_style == 2) {
                    ++nbackrt_;
                }
            }
        }
    }
    backbone_interior_begin = i2;
    if (nbackrt_) {
        backsid_ = new int[nbackrt_];
        backAindex_ = new int[nbackrt_];
        backBindex_ = new int[nbackrt_];
    }

    // reorder the trees so sid0 is the root, at least with respect to v_parent
    for (i = i1; i < i2; ++i) {
        Node* oldroot = nt->_v_node[i];
        if (classical_root_to_multisplit_) {
            const auto& msiter = classical_root_to_multisplit_->find(oldroot);
            if (msiter != classical_root_to_multisplit_->end()) {
                nd = msiter->second->nd[0];
                if (nd == oldroot) {  // the cell tree is fine
                    // the way it is (the usual case)
                    nt->_v_parent[nd->v_node_index] = 0;
                } else {
                    // need to reverse the parent/child relationship
                    // on the path from nd to oldroot
                    in = nd->v_node_index;
                    nd = 0;
                    while (in > i) {
                        ip = nt->_v_parent[in]->v_node_index;
                        nt->_v_parent[in] = nd;
                        nd = nt->_v_node[in];
                        in = ip;
                    }
                    nt->_v_parent[in] = nd;
                }
            }
        }
    }

    // Now the only problem is that assert(v_parent[i] < v_node[i])
    // is false. But we can take care of that at the end.
    // the sid0 are now root nodes with respect to v_parent

#if 0
printf("\nsid0 is a root\n");
for (i=i1; i < i3; ++i) {
	printf("%d %d %d %s %d\n", i, nt->_v_node[i]->v_node_index,
		nt->_v_parent[i] ? nt->_v_parent[i]->v_node_index : -1,
		secname(nt->_v_node[i]->sec), nt->_v_node[i]->sec_node_index_);
}
#endif
    // Index all the classical rootnodes and sid rootnodes
    // If the classical rootnode has only 1 sid, it
    // replaces it as a rootnode. If there are 2 the first sid node
    // gets shifted to the backbone_begin indices in the proper
    // short or long section.
    backbone_end = i2;
    ii = i1;
    ibl = backbone_long_begin;
    ibs = backbone_begin;
    for (i = i1; i < i2; ++i) {
        nd = nt->_v_node[i];
        if (classical_root_to_multisplit_ &&
            classical_root_to_multisplit_->find(nd) != classical_root_to_multisplit_->end()) {
            MultiSplit* ms = classical_root_to_multisplit_->operator[](nd);
            if (ms->nd[1]) {
                ib = ms->backbone_style >= 1 ? ibs : ibl;
                node[ib - i1] = ms->nd[0];
                parent[ib - i1] = 0;
                if (ms->backbone_style >= 1) {
                    ++ibs;
                } else {
                    ++ibl;
                }
                // here we can get a bit greedy and
                // start counting how many indices are
                // needed for the backbones
                // how many nodes between nd[0] and nd[1]
                // note that nd[0] IS a root.
                i0 = ms->nd[0]->v_node_index;
                int iii = ms->nd[1]->v_node_index;
                while (i0 != iii) {
                    iii = nt->_v_parent[iii]->v_node_index;
                    ++backbone_end;
                }
            } else {
                node[ii - i1] = ms->nd[0];
                parent[ii - i1] = 0;
                ++ii;
            }
        } else {
            node[ii - i1] = nd;
            parent[ii - i1] = 0;
            ++ii;
        }
    }
    backbone_sid1_begin = backbone_end - (backbone_interior_begin - backbone_begin);
    backbone_long_sid1_begin = backbone_sid1_begin + (backbone_long_begin - backbone_begin);
// printf("backbone begin=%d  interior=%d sid1=%d end=%d\n",
// backbone_begin, backbone_interior_begin, backbone_sid1_begin, backbone_end);
#if 0
	for (i=i1; i < backbone_interior_begin; ++i) {
		assert(parent[i-i1] == 0);
	}
#endif
    nback = backbone_end - backbone_begin;
    if (nback) {
        sid0i = new int[nback];
        sid1A = new double[nback];
        sid1B = new double[nback];
        for (i = 0; i < nback; ++i) {
            sid1A[i] = sid1B[i] = 0.;
            sid0i[i] = -1;
        }
    }
    iss0 = backbone_begin;
    iss1 = backbone_sid1_begin;
    isl0 = backbone_long_begin;
    isl1 = backbone_long_sid1_begin;
    ib = backbone_sid1_begin;
    // order backbone_sid1_begin to backbone_begin (same order as
    // backbone_begin to backbone_interior)
    // again, short are before long
    // the backbone interior can be any order
    int ibrt = 0;
    for (i = i1; i < i2; ++i) {
        nd = nt->_v_node[i];
        if (classical_root_to_multisplit_) {
            MultiSplit* ms;
            const auto& msiter = classical_root_to_multisplit_->find(nd);
            if (msiter != classical_root_to_multisplit_->end()) {
                ms = msiter->second;
                ms->ithread = nt->id;
                if (ms->nd[1]) {
                    if (ms->backbone_style >= 1) {
                        is0 = iss0++;
                        is1 = iss1++;
                    } else {
                        is0 = isl0++;
                        is1 = isl1++;
                    }
                    i0 = ms->nd[0]->v_node_index;
                    ii = ms->nd[1]->v_node_index;
                    node[is1 - i1] = ms->nd[1];
                    parent[is1 - i1] = nt->_v_parent[ii];
                    ii = nt->_v_parent[ii]->v_node_index;
                    while (i0 != ii) {
                        --ib;
                        node[ib - i1] = nt->_v_node[ii];
                        parent[ib - i1] = nt->_v_parent[ii];
                        // printf("sid0i[%d] = %d\n", ib-backbone_begin, is0);
                        sid0i[ib - backbone_begin] = is0;
                        ii = nt->_v_parent[ii]->v_node_index;
                    }
                    if (ms->backbone_style == 2) {
                        backsid_[ibrt] = ms->sid[0];
                        ms->back_index = ibrt;
                        backAindex_[ibrt] = is0 - backbone_begin;
                        backBindex_[ibrt] = is1 - backbone_begin;
                        // printf("backAindex[%d] = %d sid=%d Bindex=%d\n",
                        // ibrt, backAindex_[ibrt], backsid_[ibrt], backBindex_[ibrt]);
                        ++ibrt;
                    }
                }
            }
        }
    }
    // printf("backbone order complete\n");

    // most of the rest are already in tree order and have the correct
    // parents. They just need to be copied into node and parent
    // so that the parent index is alway < the node index
    // However, sid0 becoming root can cause many places where
    // parent_index > node_index and these need to be relocated
    for (i = i1; i < i3; ++i) {
        // can exploit this variable because it will be set later
        nt->_v_node[i]->eqn_index_ = -1;
    }
    for (i = i1; i < backbone_end; ++i) {
#if 0
printf("backbone i=%d %d %s %d", i, node[i]->v_node_index, secname(node[i]->sec), node[i]->sec_node_index_);
printf("  ->  %s %d\n", parent[i]?secname(parent[i]->sec):"root",
parent[i]?parent[i]->sec_node_index_:-1);
#endif
        node[i - i1]->eqn_index_ = i;
    }
    // the rest are in order
    j = backbone_end;
    for (i = i1; i < i3; ++i) {
        k0 = 0;
        k1 = i;
        // printf("i=%d\n", i);
        while (nt->_v_node[k1]->eqn_index_ < 0) {
            // printf("counting i=%d k1=%d k0=%d\n", i, k1, k0);
            ++k0;
            if (!nt->_v_parent[k1]) {
                break;
            }
            k1 = nt->_v_parent[k1]->v_node_index;
        }
        // printf("i=%d k0=%d\n", i, k0);
        k1 = i;
        j += k0;
        k0 = j - 1;
        while (nt->_v_node[k1]->eqn_index_ < 0) {
            // printf("ordering i=%d k1=%d k0=%d parent=%p\n", i, k1, k0, nt->_v_parent[k1]);
            node[k0 - i1] = nt->_v_node[k1];
            parent[k0 - i1] = nt->_v_parent[k1];
            node[k0 - i1]->eqn_index_ = k0;
            --k0;
            if (!nt->_v_parent[k1]) {
                break;
            }
            k1 = nt->_v_parent[k1]->v_node_index;
        }
    }
    for (i = i1; i < i3; ++i) {
        assert(i == node[i - i1]->eqn_index_);
        nt->_v_node[i] = node[i - i1];
        nt->_v_parent[i] = parent[i - i1];
        nt->_v_node[i]->v_node_index = i;
    }
    delete[] node;
    delete[] parent;

#if 0
printf("\nmultisplit reordering\n");
printf("backbone begin=%d  long=%d interior=%d sid1=%d long=%d end=%d\n",
backbone_begin, backbone_long_begin, backbone_interior_begin,
backbone_sid1_begin, backbone_long_sid1_begin, backbone_end);
for (i=i1; i < i3; ++i) {
	printf("%d %d %s %d ", nt->_v_node[i]->v_node_index,
		nt->_v_parent[i] ? nt->_v_parent[i]->v_node_index : -1,
		secname(nt->_v_node[i]->sec), nt->_v_node[i]->sec_node_index_);
	if (nt->_v_parent[i]) {
		printf(" -> %s %d\n", secname(nt->_v_parent[i]->sec), nt->_v_parent[i]->sec_node_index_);
	}else{
		printf(" root\n");
	}
}
#endif
    for (i = i1; i < i3; ++i)
        if (nt->_v_parent[i] && nt->_v_parent[i]->v_node_index >= i) {
            printf("i=%d parent=%d\n", i, nt->_v_parent[i]->v_node_index);
        }
    for (i = i1; i < i3; ++i) {
        assert(nt->_v_node[i]->v_node_index == i);
        assert(nt->_v_parent[i] == 0 || nt->_v_parent[i]->v_node_index < i);
    }

    // freeing sid1A... invalidated any offdiag_ pointers.
    for (i = 0; i < msc_->nthost_; ++i) {
        MultiSplitTransferInfo& msti = msc_->msti_[i];
        for (j = 0; j < msti.nnode_rt_; j += 2) {
            msti.offdiag_[j] = sid1A + msti.ioffdiag_[j];
            msti.offdiag_[j + 1] = sid1B + msti.ioffdiag_[j + 1];
        }
    }
    // sid1A, sid1B pointers in reducedtree map will be updated by
    // rt_map_update along with d and rhs
}

void MultiSplitControl::pmat(bool full) {
    int it, i, is;
    Printf("\n");
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* _nt = nrn_threads + it;
        MultiSplitThread& t = mth_[it];
        for (i = 0; i < _nt->end; ++i) {
            is = _nt->_v_node[i]->_classical_parent ? _nt->_v_node[i]->sec_node_index_ : -1;
            Printf("%d %d %s %d",
                   _nt->_v_node[i]->v_node_index,
                   _nt->_v_parent[i] ? _nt->_v_parent[i]->v_node_index : -1,
                   secname(_nt->_v_node[i]->sec),
                   is);
            if (_nt->_v_parent[i]) {
                is = _nt->_v_parent[i]->_classical_parent ? _nt->_v_parent[i]->sec_node_index_ : -1;
                Printf("  ->  %s %d", secname(_nt->_v_parent[i]->sec), is);
                Printf("\t %10.5g  %10.5g", NODEB(_nt->_v_node[i]), NODEA(_nt->_v_node[i]));
            } else {
                Printf(" root\t\t %10.5g  %10.5g", 0., 0.);
            }

            if (full) {
                Printf("  %10.5g  %10.5g", NODED(_nt->_v_node[i]), NODERHS(_nt->_v_node[i]));
                if (t.sid0i && i >= t.backbone_begin && i < t.backbone_end) {
                    Printf("  %10.5g  %10.5g",
                           t.S1B(i - t.backbone_begin),
                           t.S1A(i - t.backbone_begin));
                }
            }
            Printf("\n");
        }
    }
}

void MultiSplitControl::pmatf(bool full) {
    int it, i, is;
    FILE* f;
    char fname[100];

    Sprintf(fname, "pmat.%04d", nrnmpi_myid);
    f = fopen(fname, "w");
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* _nt = nrn_threads + it;
        MultiSplitThread& t = mth_[it];
        fprintf(f, "%d %d\n", it, _nt->end);
        for (i = 0; i < _nt->end; ++i) {
            is = _nt->_v_node[i]->_classical_parent ? _nt->_v_node[i]->sec_node_index_ : -1;
            fprintf(f,
                    "%d %d %s %d",
                    _nt->_v_node[i]->v_node_index,
                    _nt->_v_parent[i] ? _nt->_v_parent[i]->v_node_index : -1,
                    secname(_nt->_v_node[i]->sec),
                    is);
            if (_nt->_v_parent[i]) {
                is = _nt->_v_parent[i]->_classical_parent ? _nt->_v_parent[i]->sec_node_index_ : -1;
                fprintf(f, "  ->  %s %d", secname(_nt->_v_parent[i]->sec), is);
                fprintf(f, "\t %10.5g  %10.5g", NODEB(_nt->_v_node[i]), NODEA(_nt->_v_node[i]));
            } else {
                fprintf(f, " root\t\t %10.5g  %10.5g", 0., 0.);
            }

            if (full) {
                fprintf(f, "  %10.5g  %10.5g", NODED(_nt->_v_node[i]), NODERHS(_nt->_v_node[i]));
                if (t.sid0i && i >= t.backbone_begin && i < t.backbone_end) {
                    fprintf(f,
                            "  %10.5g  %10.5g",
                            t.S1B(i - t.backbone_begin),
                            t.S1A(i - t.backbone_begin));
                }
            }
            fprintf(f, "\n");
        }
    }
    fclose(f);
}

void MultiSplitControl::pmat1(const char* s) {
    int it;
    double a, b, d, rhs;
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* _nt = nrn_threads + it;
        auto* const vec_d = _nt->node_d_storage();
        auto* const vec_rhs = _nt->node_rhs_storage();
        MultiSplitThread& t = mth_[it];
        int i1 = 0;
        int i3 = _nt->end;
        for (MultiSplit* ms: *multisplit_list_) {
            int i = ms->nd[0]->v_node_index;
            if (i < i1 || i >= i3) {
                continue;
            }
            d = D(i);
            rhs = RHS(i);
            a = b = 0.;
            if (ms->nd[1]) {
                a = mth_[it].S1A(0);
            }
            Printf("%2d %s sid=%d %12.5g %12.5g %12.5g %12.5g\n",
                   nrnmpi_myid,
                   s,
                   ms->sid[0],
                   b,
                   d,
                   a,
                   rhs);
            if (ms->nd[1]) {
                d = D(ms->nd[1]->v_node_index);
                rhs = RHS(ms->nd[1]->v_node_index);
                a = 0.;
                b = t.S1B(t.backbone_sid1_begin - t.backbone_begin);
                Printf("%2d %s sid=%d %12.5g %12.5g %12.5g %12.5g\n",
                       nrnmpi_myid,
                       s,
                       ms->sid[1],
                       b,
                       d,
                       a,
                       rhs);
            }
        }
    }
}

double* nrn_classicalNodeA(Node* nd) {
    int i = nd->v_node_index;
    Node* pnd = nd->_classical_parent;
    NrnThread* _nt = nd->_nt;
    if (_nt->_v_parent[i] == pnd) {
        return &NODEA(nd);
    } else if (pnd == 0) {
        return 0;
    } else if (_nt->_v_parent[pnd->v_node_index] == nd) {
        return &NODEB(pnd);
    } else {
        assert(0);
    }
    return 0;
}

double* nrn_classicalNodeB(Node* nd) {
    int i = nd->v_node_index;
    Node* pnd = nd->_classical_parent;
    NrnThread* _nt = nd->_nt;
    if (_nt->_v_parent[i] == pnd) {
        return &NODEB(nd);
    } else if (pnd == 0) {
        return 0;
    } else if (_nt->_v_parent[pnd->v_node_index] == nd) {
        return &NODEA(pnd);
    } else {
        assert(0);
    }
    return 0;
}
