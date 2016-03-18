#ifndef cellorder_h
#define cellorder_h

int* calculate_endnodes(int ncell, int nnode, int* parent);
int* interleave_permutations(int ncell, int* endnode,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize);

int* interleave_order(int ith, int ncell, int nnode, int* parent);

void create_interleave_info();
void destroy_interleave_info();

#define INTERLEAVE_DEBUG 0
#if INTERLEAVE_DEBUG
void mk_cell_indices();
#endif

#endif


