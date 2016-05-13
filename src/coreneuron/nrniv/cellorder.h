#ifndef cellorder_h
#define cellorder_h

int* interleave_order(int ith, int ncell, int nnode, int* parent);

void create_interleave_info();
void destroy_interleave_info();

class InterleaveInfo {
  public:
  InterleaveInfo();
  virtual ~InterleaveInfo();
  int nwarp; // used only by interleave2
  int nstride;
  int* stridedispl; // interleave2: nwarp+1
  int* stride;    // interleave2: stride  length is stridedispl[nwarp]
  int* firstnode; // interleave2: rootbegin nwarp+1 displacements
  int* lastnode;  // interleave2: nodebegin nwarp+1 displacements
  int* cellsize;  // interleave2: ncycles nwarp
};

// interleaved from cellorder2.cpp
int* node_order(int ncell, int nnode, int* parents, int& nwarp,
  int& nstride, int*& stride, int*& firstnode, int*& lastnode, int*& cellsize,
  int*& stridedispl
);

#define INTERLEAVE_DEBUG 0

#if INTERLEAVE_DEBUG
void mk_cell_indices();
#endif

#endif


