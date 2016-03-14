#ifndef cellorder_h
#define cellorder_h

int* calculate_endnodes(int ncell, int nnode, int* parent);
int* interleave_permutations(int ncell, int* endnode,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize);

int* interleave_order(int ncell, int nnode, int* parent);

#endif


