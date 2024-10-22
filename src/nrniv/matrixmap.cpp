#include <../../nrnconf.h>
#include "matrixmap.h"
#include <vector>

#include "spmatrix.h"

MatrixMap::MatrixMap(Matrix& mat)
    : m_(mat) {}

MatrixMap::MatrixMap(Matrix* mat)
    : m_(*mat) {}

void MatrixMap::mmfree() {
    pm_.clear();
    pm_.shrink_to_fit();
    ptree_.clear();
    ptree_.shrink_to_fit();
}

void MatrixMap::add(double fac) {
    for (int i = 0; i < plen_; ++i) {
        *ptree_[i] += fac * (*pm_[i]);
    }
}

void MatrixMap::alloc(int start, int nnode, Node** nodes, int* layer) {
    static double place_holder = 0;
    NrnThread* _nt = nrn_threads;
    mmfree();

    plen_ = 0;
    std::vector<int> nonzero_i, nonzero_j;
    m_.nonzeros(nonzero_i, nonzero_j);
    pm_.resize(nonzero_i.size());
    ptree_.resize(nonzero_i.size());
    for (int k = 0; k < nonzero_i.size(); k++) {
        const int i = nonzero_i[k];
        const int j = nonzero_j[k];
        int it;
        if (i < nnode) {
            it = nodes[i]->eqn_index_ + layer[i];
            if (layer[i] > 0 && !nodes[i]->extnode) {
                it = 0;
            }
        } else {
            it = start + i - nnode;
        }
        int jt;
        pm_[plen_] = m_.mep(i, j);
        if (j < nnode) {
            jt = nodes[j]->eqn_index_ + layer[j];
            if (layer[j] > 0 && !nodes[j]->extnode) {
                jt = 0;
            }
        } else {
            jt = start + j - nnode;
        }
        if (it == 0 || jt == 0) {
            ptree_[plen_] = &place_holder;
        } else {
            ptree_[plen_] = _nt->_sp13mat->mep(it - 1, jt - 1);
        }
        ++plen_;
    }
}
