#ifndef tnode_h
#define tnode_h

#include <vector>

// experiment with ordering strategies for Tree Nodes
namespace coreneuron {
class TNode;

using VecTNode = std::vector<TNode*>;

class TNode {
  public:
    TNode(int ix);
    virtual ~TNode();
    TNode* parent;
    VecTNode children;
    size_t mkhash();
    size_t hash;
    size_t treesize;
    size_t nodevec_index;
    size_t treenode_order;
    size_t level;
    size_t cellindex;
    size_t groupindex;
    int nodeindex;
};

size_t level_from_leaf(VecTNode&);
size_t level_from_root(VecTNode&);

void group_order2(VecTNode&, size_t groupsize, size_t ncell);
size_t dist2child(TNode* nd);

// see balance.cpp
size_t warp_balance(size_t ncell, VecTNode& nodevec);

#define warpsize 32
}  // namespace coreneuron
#endif
