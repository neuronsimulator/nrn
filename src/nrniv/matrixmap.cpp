#include <../../nrnconf.h>
#include "matrixmap.h"
#include <vector>

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
        auto [it, jt] = pm_[i];
        auto [im, jm] = ptree_[i];
        _nt->_sp13mat->coeff(im, jm) += fac * m_(it, jt);
    }
}

int MatrixMap::compute_index(int i, int start, int nnode, Node** nodes, int* layer) const {
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

    std::vector<std::pair<int, int>> nzs = m_.nonzeros();
    pm_.reserve(nzs.size());
    ptree_.reserve(nzs.size());
    for (const auto& [i, j]: nzs) {
        int it = compute_index(i, start, nnode, nodes, layer);
        int jt = compute_index(j, start, nnode, nodes, layer);
        if (it != 0 && jt != 0) {
            pm_.emplace_back(i, j);
            ptree_.emplace_back(it - 1, jt - 1);
        }
    }
}
