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
        auto [it, jt] = pm_[i];
        *ptree_[i] += fac * m_.getval(it, jt);
    }
}

void MatrixMap::alloc(int start, int nnode, Node** nodes, int* layer) {
    NrnThread* _nt = nrn_threads;
    mmfree();

    plen_ = 0;
    std::vector<std::pair<int, int>> nzs = m_.nonzeros();
    pm_.resize(nzs.size());
    ptree_.resize(nzs.size());
    for (const auto [i, j]: nzs) {
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
        pm_[plen_] = std::make_pair(i, j);
        if (j < nnode) {
            jt = nodes[j]->eqn_index_ + layer[j];
            if (layer[j] > 0 && !nodes[j]->extnode) {
                jt = 0;
            }
        } else {
            jt = start + j - nnode;
        }
        ptree_[plen_] = spGetElement(_nt->_sp13mat, it, jt);
        ++plen_;
    }
}
