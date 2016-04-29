#include <stdio.h>
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/cellorder.h"

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string.h>

using namespace std;

class TNode {
  public:
  TNode(int ix);
  virtual ~TNode();
  TNode* parent;
  vector<TNode*> children;
  size_t mkhash();
  size_t hash;
  size_t treesize;
  size_t nodevec_index;
  size_t treenode_order;
  size_t level;
  size_t cellindex;
  int nodeindex;
};

static bool tnode_earlier(TNode* a, TNode* b) {
  bool result = false;
  if (a->treesize < b->treesize) {// treesize dominates
    result = true;
  }else if (a->treesize == b->treesize) {
    if (a->hash < b->hash) { // if treesize same, keep identical trees together
      result = true;
    }else if (a->hash == b->hash) {
      result = a->nodeindex < b->nodeindex; // identical trees ordered by nodeindex
    }
  }
  return result;
}

static bool ptr_tnode_earlier(TNode* a, TNode* b) {
  return tnode_earlier(a, b);
}

TNode::TNode(int ix) {
  nodeindex = ix;
  cellindex = 0;
  level = -1;
  hash = 0;
  treesize = 1;
  nodevec_index = -1;
  treenode_order = -1;
  parent = NULL;
  children.reserve(2);
}

TNode::~TNode() {} 

size_t TNode::mkhash() { // call on all nodes in leaf to root order
  // concept from http://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
  std::sort(children.begin(), children.end(), ptr_tnode_earlier);
  hash = children.size();
  treesize = 1;
  for (size_t i=0; i < children.size(); ++i) { // need sorted by child hash
    hash ^= children[i]->hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    treesize += children[i]->treesize;
  }
  return hash;  // hash of leaf nodes is 0
}

static void tree_analysis(int* parent, int nnode, int ncell, vector<TNode*>&);
static void node_interleave_order(int ncell, vector<TNode*>&);
static void par_ordering(vector<TNode*>&);
static void admin(int ncell, vector<TNode*>& nodevec,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize);
static void check(vector<TNode*>&);
static void prtree(vector<TNode*>&);

typedef std::pair<TNode*, int> TNI;
typedef std::map<size_t, pair<TNode*, int> > HashCnt;
typedef vector<TNI> TNIVec;

static bool tnivec_cmp(const TNI& a, const TNI& b) {
  bool result = false;
  if (a.second < b.second) {
    result = true;
  }else if (a.second == b.second) {
    result = b.first->treesize < a.first->treesize;
  }
  return result;
}

static char* stree(TNode* nd) {
  char s[1000];

  if (nd->treesize > 100) { return strdup(""); }
  s[0] = '(';
  s[1] = '\0';
  for (size_t i=0; i < nd->children.size(); ++i) { // need sorted by child hash
    char* sr = stree(nd->children[i]);
    strcat(s, sr);
    free(sr);
  }
  strcat(s, ")");
  return strdup(s);
}

static void exper1(vector<TNode*>& nodevec) {
  printf("nodevec.size = %ld\n", nodevec.size());
  HashCnt hashcnt;
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    size_t h = nd->hash;
    HashCnt::iterator search = hashcnt.find(h);
    if (search != hashcnt.end()) {
      search->second.second += 1;
    }else{
      hashcnt[h] = pair<TNode*, int>(nd, 1);
    }
  }
  TNIVec tnivec;
  for (HashCnt::iterator i = hashcnt.begin(); i != hashcnt.end(); ++i) {
    tnivec.push_back(i->second);
  }
  std::sort(tnivec.begin(), tnivec.end(), tnivec_cmp);

  // I am a child of <n> parent patterns (parallel to tnivec)
  map<size_t, set<size_t> > parpat;
  for (size_t i = 0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    parpat[nd->hash].insert(nd->parent ? nd->parent->hash : 0);
  }

  for (TNIVec::iterator i = tnivec.begin(); i != tnivec.end(); ++i) {
    char* sr = stree(i->first);
    printf("%20ld %5d %3ld %4ld %s\n", i->first->hash, i->second, i->first->treesize,
      parpat[i->first->hash].size(), sr);
    free(sr);
  }
}

/*
assess the quality of the ordering. The measure is the size of a contiguous
list of nodes whose parents have the same order. How many contiguous lists
have that same size. How many nodes participate in that size list.
Modify the quality measure from experience with performance. Start with
list of (nnode, size_participation)
*/
static void quality(vector<TNode*>& nodevec, size_t max = 32) {
  size_t qcnt=0; // how many contiguous nodes have contiguous parents

  // first ncell nodes are by definition in contiguous order
  for (size_t i = 0; i < nodevec.size(); ++i) {
    if (nodevec[i]->parent != NULL) {
      break;
    }
    qcnt += 1;
  }
  size_t ncell = qcnt;

  // key is how many parents in contiguous order
  // value is number of nodes that participate in that
  map<size_t, size_t> qual;
  size_t ip_last = 10000000000;
  for (size_t i = ncell; i < nodevec.size(); ++i) {
    size_t ip = nodevec[i]->parent->nodevec_index;
    if (ip == ip_last + 1) { // contiguous
      qcnt += 1;
    }else{
      if (qcnt == 1) {
        printf("unique %ld p=%ld ix=%d\n", i, ip, nodevec[i]->nodeindex);
      }
      qual[max] += (qcnt/max) * max;
      size_t x = qcnt%max;
      if (x) { qual[x] += x; }
      qcnt = 1;
    }
    ip_last = ip;
  }
  qual[qcnt] += qcnt;

  // print result
  qcnt = 0;
  for (map<size_t, size_t>::iterator it = qual.begin(); it != qual.end(); ++it) {
    qcnt += it->second;
    printf("%6ld %6ld\n", it->first, it->second);
  }
  printf("qual.size=%ld  qual total nodes=%ld  nodevec.size=%ld\n",
    qual.size(), qcnt, nodevec.size());
  
}

// for cells with same size, keep identical trees together

// parent is (unpermuted)  nnode length vector of parent node indices.
// return a permutation (of length nnode) which orders cells of same
// size so that identical trees are grouped together.
// Note: cellorder[ncell:nnode] are the identify permutation.

int* node_order(int ncell, int nnode, int* parent,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize
) {

  vector<TNode*> nodevec;
  if (0) prtree(nodevec); // avoid unused warning

  // nodevec[0:ncell] in increasing size, with identical trees together,
  // and otherwise nodeindex order
  tree_analysis(parent, nnode, ncell, nodevec);
  check(nodevec);

  // nodevec[ncell:nnode] cells are interleaved in nodevec[0:ncell] cell order
  node_interleave_order(ncell, nodevec);
  check(nodevec);
  par_ordering(nodevec);
  check(nodevec);

#if 0
  for (int i=0; i < ncell; ++i) {
    TNode& nd = *nodevec[i];
    printf("%d size=%ld hash=%ld ix=%d\n", i, nd.treesize, nd.hash, nd.nodeindex);
  }
#endif

  // the permutation
  int* nodeorder = new int[nnode];
  for (int i=0; i < nnode; ++i) {
    TNode& nd = *nodevec[i];
    nodeorder[nd.nodeindex] = i;
  }

  // administrative statistics for gauss elimination
  admin(ncell, nodevec, nstride, stride, firstnode, lastnode, cellsize);

  if (0) {exper1(nodevec);}
  quality(nodevec);

#if 1
  int ntopol = 1;
  for (int i = 1; i < ncell; ++i) {
    if (nodevec[i-1]->hash != nodevec[i]->hash) {
      ntopol += 1;
    }
  }
  printf("%d distinct tree topologies\n", ntopol);
#endif

  for (size_t i =0; i < nodevec.size(); ++i) {
    delete nodevec[i];
  }

  return nodeorder;
}

void check(vector<TNode*>& nodevec) {
  //printf("check\n");
  size_t nnode = nodevec.size();
  size_t ncell = 0;
  for (size_t i=0; i < nnode; ++i) {
    nodevec[i]->nodevec_index = i;
    if (nodevec[i]->parent == NULL) { ncell++; }
  }
  for (size_t i=0; i < ncell; ++i) {
    nrn_assert(nodevec[i]->parent == NULL);
  }
  for (size_t i=ncell; i < nnode; ++i) {
    TNode& nd = *nodevec[i];
    if (nd.parent->nodevec_index >= nd.nodevec_index) {
      printf("error i=%ld nodevec_index=%ld parent=%ld\n", i, nd.nodevec_index, nd.parent->nodevec_index);
    }
    nrn_assert(nd.nodevec_index > nd.parent->nodevec_index);
  }
  
}

void prtree(vector<TNode*>& nodevec) {
  size_t nnode = nodevec.size();
  for (size_t i=0; i < nnode; ++i) {
    nodevec[i]->nodevec_index = i;
  }
  for (size_t i=0; i < nnode; ++i) {
    TNode& nd = *nodevec[i];
    printf("%ld p=%ld   c=%ld l=%ld o=%ld   ix=%d pix=%d\n",
      i, nd.parent ? nd.parent->nodevec_index : -1,
      nd.cellindex, nd.level, nd.treenode_order,
       nd.nodeindex, nd.parent ? nd.parent->nodeindex : -1);
  }
}

void tree_analysis(int* parent, int nnode, int ncell, vector<TNode*>& nodevec) {

//  vector<TNode*> nodevec;

  // create empty TNodes (knowing only their index)
  nodevec.reserve(nnode);
  for (int i=0; i < nnode; ++i) {
    nodevec.push_back(new TNode(i));
  }

  // determine the (sorted by hash) children of each node
  for (int i = nnode -1; i >= ncell; --i) {
    nodevec[i]->parent = nodevec[parent[i]];
    nodevec[i]->mkhash();
    nodevec[parent[i]]->children.push_back(nodevec[i]);
  }

  // determine hash of the cells
  for (int i = 0; i < ncell; ++i) {
    nodevec[i]->mkhash();
  }

  std::sort(nodevec.begin(), nodevec.begin() + ncell, tnode_earlier);
}

#if 0
static bool contig_comp(TNode* a, TNode* b) {
  bool result = false;
  if (a->cellindex < b->cellindex) {
    result = true;
  }else if (a->cellindex == b->cellindex) {
    if (a->treenode_order < b->treenode_order) {
      result = true;
    }
  }
  return result;
}
#endif

static bool interleave_comp(TNode* a, TNode* b) {
  bool result = false;
  if (a->treenode_order < b->treenode_order) {
    result = true;
  }else if (a->treenode_order == b->treenode_order) {
    if (a->cellindex < b->cellindex) {
      result = true;
    }
  }
  return result;
}

// sort so nodevec[ncell:nnode] cell instances are contiguous. Keep the
// secondary ordering with respect to treenode_order so each cell is still a tree.

// sort so nodevec[ncell:nnode] cell instances are interleaved. Keep the
// secondary ordering with respect to treenode_order so each cell is still a tree.

void node_interleave_order(int ncell, vector<TNode*>& nodevec) {
  int* order = new int[ncell];
  for (int i=0; i < ncell; ++i) {
    nodevec[i]->cellindex = i;
    order[i] = 0;
    nodevec[i]->treenode_order = order[i]++;
  }
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode& nd = *nodevec[i];
    for (size_t j=0; j < nd.children.size(); ++j) {
      TNode* cnode = nd.children[j];
      cnode->cellindex = nd.cellindex;
      cnode->treenode_order = order[nd.cellindex]++;
    }
  }
  delete [] order;

//  std::sort(nodevec.begin() + ncell, nodevec.end(), contig_comp);
  std::sort(nodevec.begin() + ncell, nodevec.end(), interleave_comp);

#if 0
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode& nd = *nodevec[i];
    printf("%ld cell=%ld ix=%d\n",  i, nd.cellindex, nd.nodeindex);
  }
#endif
}

static bool contiglevel_comp(TNode* a, TNode* b) {
  bool result = false;
  if (a->cellindex < b->cellindex) {
    result = true;
  }else if (a->cellindex == b->cellindex) {
    if (a->level < b->level) {
      result = true;
    }else if (a->level == b->level) {
      if (a->treenode_order < b->treenode_order) {
        result = true;
      }
    }
  }
  return result;
}

static void chkorder1(size_t ncell, vector<TNode*>& nodevec) {
  size_t cellindex = 0;
  size_t level = 0;
  for (size_t i = ncell; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    nrn_assert(nd->nodevec_index == i);
    nrn_assert(nd->cellindex >= cellindex);
    if (nd->cellindex == cellindex) {
      nrn_assert(nd->level >= level);
    }
    cellindex = nd->cellindex;
    level = nd->level;
  }
  //printf("chkorder1 true\n");
}

static bool level_node_comp(TNode* a, TNode* b) {
  bool result = false;
  if (a->treenode_order < b->treenode_order) {
    result = true;
  }
  return result;
}

// sort so nodevec[ncell:nnode] cell instances are contiguous. Keep the
// secondary ordering with respect to treenode_order so each cell is still a tree.

// sort so nodevec[ncell:nnode] cell instances are interleaved. Keep the
// secondary ordering with respect to treenode_order so each cell is still a tree.

// treenode order is according to levels (all children of nodes at level i
// have level i+1)  Within a level are all the first children, followed by
// all the second children, etc. The groups of first, second,... children
// have the same order as their parents.

static void par_ordering(vector<TNode*>& nodevec) {
  size_t ncell = 0;
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    if (nd->parent != NULL) { break; }
    nd->cellindex = i;
    nd->treenode_order = 0; // starts out as order_in_level and not contiguous
    nd->level = 0;
    nd->nodevec_index = i;
    ncell++;
  }

  int* order0 = new int[ncell];
  for (size_t i=0; i < ncell; ++i) {
    order0[i] = 0;
  }

  // levels and cell indices
  for (size_t i=ncell; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    nd->level = nd->parent->level + 1;
    nd->cellindex = nd->parent->cellindex;
    nd->treenode_order = ++order0[nd->parent->cellindex];
    nd->nodevec_index = i; // an order but...
  }
    
//  std::sort(nodevec.begin() + ncell, nodevec.end(), contig_comp);
//  std::sort(nodevec.begin() + ncell, nodevec.end(), interleave_comp);

  // sort cells contiguous, levels contiguous, order_in_level nonsense order
  std::sort(nodevec.begin() + ncell, nodevec.end(), contiglevel_comp);

  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    nd->nodevec_index = i; // current order, need to maintain after sort
  }

chkorder1(ncell, nodevec);

  // calculate an ordering within a cell level
  // for each level presume previous level is sorted
  size_t begin = ncell;
  size_t cellindex = ncell < nodevec.size() ? nodevec[ncell]->cellindex : 0;
  size_t level = ncell < nodevec.size() ? nodevec[ncell]->level : 0;

  for (size_t i = ncell; i <= nodevec.size(); ++i) { // i==nodevec.size() is special
    TNode* nd = i < nodevec.size() ? nodevec[i] : NULL;
    if (i == nodevec.size() || nd->cellindex != cellindex || nd->level != level) {
      // process the level [begin:i)
//printf("process level c=%ld l=%ld  %ld to %ld --- %ld\n", cellindex, level, begin, i, nodevec.size());
      // previous level is already sorted. find its "begin".      
if (nd == NULL) {
      nd = nodevec[i-1];
}
      size_t p_begin = nodevec[begin]->parent->nodevec_index - nodevec[begin]->parent->treenode_order;
#if 0
printf("nd i=%ld c=%ld l=%ld p=%ld cp=%ld pl=%ld po=%ld\n",
nd->nodevec_index, nd->cellindex, nd->level,
nd->parent->nodevec_index, nd->parent->cellindex, nd->parent->level,
nd->parent->treenode_order);
printf("assert p_begin=%ld c=%ld l=%ld ix=%ld to=%ld\n",
p_begin, nodevec[p_begin]->cellindex, nodevec[p_begin]->level,
nd->parent->nodevec_index, nd->parent->treenode_order);
#endif
      nrn_assert(nodevec[p_begin]->treenode_order == 0);

      // compute treenode_order for this level
      size_t order = 0;
      size_t plevel = nodevec[p_begin]->level;
      for (size_t j = p_begin; j < i; ++j) {
        TNode* pnd = nodevec[j];
        if (pnd->cellindex != cellindex || pnd->level != plevel) {
          break;
        }
        for (size_t k = 0; k < pnd->children.size(); ++k) {
           pnd->children[k]->treenode_order = order + k*1000;
        }
        ++order;
      }
      // sort this level
      std::sort(nodevec.begin() + begin, nodevec.begin() + i, level_node_comp);

      // update nodevec_index and treenode_order contiguous for level
      order = 0;
      for (size_t j = begin; j < i; ++j) {
        TNode* nd = nodevec[j];
        nd->nodevec_index = j;
        nd->treenode_order = order++;
      }

      // check this level
      nrn_assert(nodevec[begin-1]->cellindex < cellindex || nodevec[begin-1]->level < level);
      for (size_t j = begin; j < i; ++j) {
        TNode* nd = nodevec[j];
        nrn_assert(nd->cellindex == cellindex && nd->level == level);
        nrn_assert(nd->treenode_order == j - begin);
        nrn_assert(nd->nodevec_index == j);
      }

      // next level
      begin = i;
      level = nd->level;
      cellindex = nd->cellindex;
    }
  }
chkorder1(ncell, nodevec);
  // recalculate contig treenode_order
  size_t order = 1;
  cellindex = 0;
  for (size_t i=ncell; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    if (nd->cellindex != cellindex) {
      cellindex = nd->cellindex;
      order = 1;
    }
    nd->treenode_order = order++;
  }
  std::sort(nodevec.begin() + ncell, nodevec.end(), interleave_comp);
//prtree(nodevec);

#if 0
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode& nd = *nodevec[i];
    printf("%ld cell=%ld ix=%d\n",  i, nd.cellindex, nd.nodeindex);
  }
#endif
}

static void admin(int ncell, vector<TNode*>& nodevec,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize
){
  // firstnode[i] is the index of the first nonroot node of the cell
  // lastnode[i] is the index of the last node of the cell
  // cellsize is the number of nodes in the cell not counting root.
  // nstride is the maximum cell size (not counting root)
  // stride[i] is the number of cells with an ith node.
  firstnode = new int[ncell];
  lastnode = new int[ncell];
  cellsize = new int[ncell];

  for (int i = 0; i < ncell; ++i) {
    firstnode[i] = -1;
    lastnode[i] = -1;
    cellsize[i] = 0;
  }

  nstride = 0;
  for (size_t i = ncell; i < nodevec.size(); ++i) {
    TNode& nd = *nodevec[i];
    size_t ci = nd.cellindex;
    if (firstnode[ci] == -1) {
      firstnode[ci] = i;
    }
    lastnode[ci] = i;
    cellsize[ci] += 1;
    if (nstride < cellsize[ci]) {
      nstride = cellsize[ci];
    }
  }

  stride = new int[nstride + 1]; // in case back substitution accesses this
  for (int i=0; i <= nstride; ++i) {
    stride[i] = 0;
  }
  for (size_t i = ncell; i < nodevec.size(); ++i) {
    TNode& nd = *nodevec[i];
    stride[nd.treenode_order - 1] += 1; // -1 because treenode order includes root
  }
}

