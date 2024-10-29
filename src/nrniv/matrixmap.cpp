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
    NrnThread* _nt = nrn_threads;
    for (int i = 0; i < pm_.size(); ++i) {
        auto [im, jm] = pm_[i];
        auto [it, jt] = ptree_[i];
        *_nt->_sp13mat->mep(it, jt) += fac * m_.getval(im, jm);
    }
}

int MatrixMap::compute_id(int i, int start, int nnode, Node** nodes, int* layer) const {
    int it;
    if (i < nnode) {
        it = nodes[i]->eqn_index_ + layer[i];
        if (layer[i] > 0 && !nodes[i]->extnode) {
            it = 0;
        }
    } else {
        it = start + i - nnode;
    }
    return it;
}

void MatrixMap::alloc(int start, int nnode, Node** nodes, int* layer) {
    mmfree();

    std::vector<int> nonzero_i, nonzero_j;
    m_.nonzeros(nonzero_i, nonzero_j);
    pm_.reserve(nonzero_i.size());
    ptree_.reserve(nonzero_i.size());
    for (int k = 0; k < nonzero_i.size(); k++) {
        const int i = nonzero_i[k];
        const int j = nonzero_j[k];
        int it = compute_id(i, start, nnode, nodes, layer);
        int jt = compute_id(j, start, nnode, nodes, layer);
        if (it == 0 || jt == 0) {
            continue;
        }
        pm_.emplace_back(i, j);
        ptree_.emplace_back(it - 1, jt - 1);
    }
}
