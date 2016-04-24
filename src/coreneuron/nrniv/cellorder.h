#ifndef cellorder_h
#define cellorder_h

int* interleave_order(int ith, int ncell, int nnode, int* parent);

void create_interleave_info();
void destroy_interleave_info();

class InterleaveInfo {
  public:
  InterleaveInfo();
  virtual ~InterleaveInfo();
  int nstride;
  int* stride;
  int* firstnode;
  int* lastnode;
  int* cellsize;
};

// interleaved from cellorder2.cpp
int* node_order(int ncell, int nnode, int* parents,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize
);

#define INTERLEAVE_DEBUG 0

#if INTERLEAVE_DEBUG
void mk_cell_indices();
#endif

#endif


