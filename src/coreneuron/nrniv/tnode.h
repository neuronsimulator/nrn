#ifndef tnode_h
#define tnode_h

#include <vector>

//experiment with ordering strategies for Tree Nodes

class TNode;

typedef std::vector<TNode*> VecTNode;

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

#endif
