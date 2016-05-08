#include <stdio.h>
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/cellorder.h"
#include "coreneuron/nrniv/tnode.h"

#include <map>
#include <set>
#include <algorithm>
#include <string.h>

using namespace std;


// experiment starting with identical cell ordering
// groupindex aleady defined that keeps identical cells together
// begin with leaf to root ordering

typedef VecTNode VTN; // level of nodes
typedef vector<VTN> VVTN;  // group of levels
typedef vector<VVTN> VVVTN; // groups

// first child before second child, etc
// if same parent level, then parent order
// if not same parent, then earlier parent (no parent earlier than parent)
// if same parents, then children order
// if no parents then nodevec_index order.
static bool sortlevel_cmp(TNode* a, TNode* b) {
  // note that leaves are at max level and all roots at level 0
  bool result = false;
  // since cannot have an index < 0, just add 1 to level
  size_t palevel = a->parent ?  1 + a->parent->level : 0;
  size_t pblevel = a->parent ?  1 + a->parent->level : 0;
  if (palevel < pblevel) {
    result = true; // earlier level first
  }else if (palevel == pblevel) {
    if (palevel == 0) { // a and b are roots
      if (a->nodevec_index < b->nodevec_index) {
        result = true;
      }
    }else{ // parent order (already sorted with proper treenode_order
      if (a->parent->treenode_order < b->parent->treenode_order) {
        result = true;
      }else if (a->parent->treenode_order == b->parent->treenode_order) {
        if (a->treenode_order < b->treenode_order) { // children order
          result = true;
        }
      }
    }
  }
  return result;
}

static void sortlevel(VTN& level, VVTN& levels) {
  std::sort(level.begin(), level.end(), sortlevel_cmp);
  for (size_t i=0; i < level.size(); ++i) {
    level[i]->treenode_order = i;
  }
}

static void analyze(VVTN& levels) {
  // sort each level with respect to parent level order
  // earliest parent level first.

  //treenode order can be anything as long as first children < second
  // children etc.. After sorting a level, the order will be correct for
  // that level, ranging from [0:level.size]
  for (size_t i = 0; i < levels.size(); ++i) {
    for (size_t j = 0; j < levels[i].size(); ++j) {
      TNode* nd = levels[i][j];
      for (size_t k = 0; k < nd->children.size(); ++k) {
        nd->children[k]->treenode_order = k;
      }
    }
  }

  for (size_t i=levels.size()-1; ; --i) {
    sortlevel(levels[i], levels);
    if (i == 0) { break; }
  }

}

void prgroupsize(VVVTN& groups) {
  printf("enter prgroupsize\n");
  for (size_t i=0; i < groups[0].size(); ++i) {
    printf("%5ld\n", i);
    for (size_t j=0; j < groups.size(); ++j) {
      printf(" %5ld", groups[j][i].size());   
    }
    printf("\n");
  }
  printf("leave prgroupsize\n");
}

void group_order2(VecTNode& nodevec, size_t groupsize, size_t ncell) {
  printf("enter group_order2\n");
  size_t maxlevel = level_from_root(nodevec);

#if 0
  // reverse the level numbering so leaves are at maxlevel.
  // also make all roots have level 0
  for (size_t i = 0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    nd->level = maxlevel - nd->level;
    if (nd->parent == NULL) {
      nd->level = 0;
    }
  }
#endif
  
  // work on a cellgroup as a vector of levels. ie only possible race is
  // two children in same warpsize
  
  VVVTN groups(ncell/groupsize);
  for (size_t i = 0; i < groups.size(); ++i) {
    groups[i].resize(maxlevel+1);
  }
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    groups[nd->groupindex][nd->level].push_back(nd);
  }

  prgroupsize(groups);

  // deal with each group
  for (size_t i=0; i < groups.size(); ++i) {
    analyze(groups[i]);
  }

}

