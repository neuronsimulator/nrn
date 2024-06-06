#include <../../nrnconf.h>
#include "matrixmap.h"
#include <vector>

#include <Eigen/Eigen>

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
        auto [j, k] = ptree_[i];
        if (j != -1) {
            // std::cerr << "modifying '" << nrn_threads->_sparseMat->coeff(j -1, k - 1) << "' (" <<
            // j - 1 << ", " << k - 1 << ") with " << fac * (*pm_[i]) << std::endl;
            nrn_threads->_sparseMat->coeffRef(j - 1, k - 1) += fac * (*pm_[i]);
        }
    }
}

void MatrixMap::alloc(int start, int nnode, Node** nodes, int* layer) {
    NrnThread* _nt = nrn_threads;
    mmfree();

    std::vector<int> nonzero_i, nonzero_j;
    m_.nonzeros(nonzero_i, nonzero_j);
    pm_.reserve(nonzero_i.size());
    ptree_.reserve(nonzero_i.size());
    plen_ = nonzero_i.size();
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
        pm_.push_back(m_.mep(i, j));
        if (j < nnode) {
            jt = nodes[j]->eqn_index_ + layer[j];
            if (layer[j] > 0 && !nodes[j]->extnode) {
                jt = 0;
            }
        } else {
            jt = start + j - nnode;
        }
        if (it == 0 || jt == 0) {
            ptree_.emplace_back(-1, -1);
        } else {
            std::cerr << "someone touch me in the heart!: (" << it << ", " << jt << ")"
                      << std::endl;
            ptree_.emplace_back(it, jt);
            _nt->_sparseMat->coeffRef(it - 1, jt - 1) = 0.;
        }
    }
}
