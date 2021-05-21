/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#ifndef cellorder_h
#define cellorder_h

#include "coreneuron/utils/memory.h"
#include <algorithm>
namespace coreneuron {
/**
 * \brief Function that performs the permutation of the cells such that the
 *        execution threads access coalesced memory.
 *
 * \param ith NrnThread to access
 * \param ncell number of cells in NrnThread
 * \param nnode number of compartments in the ncells
 * \param parent parent indices of cells
 *
 * \return int* order, interleaved order of the cells
 */
int* interleave_order(int ith, int ncell, int nnode, int* parent);

void create_interleave_info();
void destroy_interleave_info();

/**
 *
 * \brief Solve the Hines matrices based on the interleave_permute_type (1 or 2).
 *
 * For interleave_permute_type == 1 : Naive interleaving -> Each execution thread deals with one
 * Hines matrix (cell) For interleave_permute_type == 2 : Advanced interleaving -> Each Hines matrix
 * is solved by multiple execution threads (with coalesced memory access as well)
 */
extern void solve_interleaved(int ith);

class InterleaveInfo {
  public:
    InterleaveInfo() = default;
    InterleaveInfo(const InterleaveInfo&);
    InterleaveInfo& operator=(const InterleaveInfo&);
    ~InterleaveInfo();
    int nwarp = 0;  // used only by interleave2
    int nstride = 0;
    int* stridedispl = nullptr;  // interleave2: nwarp+1
    int* stride = nullptr;       // interleave2: stride  length is stridedispl[nwarp]
    int* firstnode = nullptr;    // interleave2: rootbegin nwarp+1 displacements
    int* lastnode = nullptr;     // interleave2: nodebegin nwarp+1 displacements
    int* cellsize = nullptr;     // interleave2: ncycles nwarp

    // statistics (nwarp of each)
    size_t* nnode = nullptr;
    size_t* ncycle = nullptr;
    size_t* idle = nullptr;
    size_t* cache_access = nullptr;
    size_t* child_race = nullptr;

  private:
    void swap(InterleaveInfo& info);
};

/**
 * \brief Function that returns a permutation of length nnode.
 *
 * There are two permutation strategies:
 * For interleave_permute_type == 1 : Naive interleaving -> Each execution thread deals with one
 * Hines matrix (cell) For interleave_permute_type == 2 : Advanced interleaving -> Each Hines matrix
 * is solved by multiple execution threads (with coalesced memory access as well)
 *
 * \param ncell number of cells
 * \param nnode number of compartments in the ncells
 * \param parents parent indices of the cells
 * \param nwarp number of warps
 * \param nstride nstride is the maximum cell size (not counting root)
 * \param stride stride[i] is the number of cells with an ith node:
 *               using stride[i] we know how many positions to move in order to
 *               access the next element of the same cell (given that the cells are
 *               ordered with the treenode_order).
 * \param firstnode firstnode[i] is the index of the first nonroot node of the cell
 * \param lastnode lastnode[i] is the index of the last node of the cell
 * \param cellsize cellsize is the number of nodes in the cell not counting root.
 * \param stridedispl
 * \return int* : a permutation of length nnode
 */
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

// copy src array to dest with NRN_SOA_BYTE_ALIGN ecalloc_align allocation
template <typename T>
void copy_align_array(T*& dest, T* src, size_t n) {
    dest = (T*) ecalloc_align(n, sizeof(T));
    std::copy(src, src + n, dest);
}

#define INTERLEAVE_DEBUG 0

#if INTERLEAVE_DEBUG
void mk_cell_indices();
#endif
}  // namespace coreneuron
#endif
