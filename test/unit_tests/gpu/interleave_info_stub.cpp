#include "coreneuron/permute/cellorder.hpp"

// Standalone GPU tests do not link cellorder.cpp (InterleaveInfo ctor/dtor).
namespace neuron {

InterleaveInfo::~InterleaveInfo() {
    if (stride) {
        delete[] stride;
        delete[] firstnode;
        delete[] lastnode;
        delete[] cellsize;
        stride = nullptr;
        firstnode = nullptr;
        lastnode = nullptr;
        cellsize = nullptr;
    }
    if (stridedispl) {
        delete[] stridedispl;
        stridedispl = nullptr;
    }
}

}  // namespace neuron