#ifndef cellorder_h
#define cellorder_h

#include <algorithm>
namespace coreneuron {
int* interleave_order(int ith, int ncell, int nnode, int* parent);

void create_interleave_info();
void destroy_interleave_info();

class InterleaveInfo {
  public:
    InterleaveInfo();
    InterleaveInfo(const InterleaveInfo&);
    InterleaveInfo& operator=(const InterleaveInfo&);
    virtual ~InterleaveInfo();
    int nwarp;  // used only by interleave2
    int nstride;
    int* stridedispl;  // interleave2: nwarp+1
    int* stride;       // interleave2: stride  length is stridedispl[nwarp]
    int* firstnode;    // interleave2: rootbegin nwarp+1 displacements
    int* lastnode;     // interleave2: nodebegin nwarp+1 displacements
    int* cellsize;     // interleave2: ncycles nwarp

    // statistics (nwarp of each)
    size_t* nnode;
    size_t* ncycle;
    size_t* idle;
    size_t* cache_access;
    size_t* child_race;

  private:
    void swap(InterleaveInfo& info);
};

// interleaved from cellorder2.cpp
int* node_order(int ncell,
                int nnode,
                int* parents,
                int& nwarp,
                int& nstride,
                int*& stride,
                int*& firstnode,
                int*& lastnode,
                int*& cellsize,
                int*& stridedispl);

// copy src array to dest with new allocation
template <typename T>
void copy_array(T*& dest, T* src, size_t n) {
    dest = new T[n];
    std::copy(src, src + n, dest);
}

#define INTERLEAVE_DEBUG 0

#if INTERLEAVE_DEBUG
void mk_cell_indices();
#endif
}  // namespace coreneuron
#endif
