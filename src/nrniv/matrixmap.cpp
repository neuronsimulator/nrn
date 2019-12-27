#include <../../nrnconf.h>
#include "matrixmap.h"
#include <vector>
using std::vector;

extern "C" {
	#include "spmatrix.h"
}

MatrixMap::MatrixMap(Matrix& mat)
	: m_(mat), plen_(0), ptree_(NULL),	pm_(NULL) {
}

MatrixMap::MatrixMap(Matrix* mat)
	: m_(*mat), plen_(0), ptree_(NULL),	pm_(NULL) {
}

MatrixMap::~MatrixMap() {
	mmfree();
}

void MatrixMap::mmfree() {
	// safe to delete NULL pointers
	delete [] ptree_;
	delete [] pm_;
	ptree_ = NULL;
	pm_ = NULL;
}

void MatrixMap::add(double fac) {
	for (int i=0 ; i < plen_; ++i) {
//printf("i=%d %g += %g * %g\n", i, *ptree_[i], fac, *pm_[i]);
		*ptree_[i] += fac * (*pm_[i]);
	}
}

void MatrixMap::alloc(int start, int nnode, Node** nodes, int* layer) {
	NrnThread* _nt = nrn_threads;
	mmfree();
	// how many elements
	int nrow = m_.nrow();
	int ncol = m_.ncol();
//printf("MatrixMap::alloc nrow=%d ncol=%d\n", nrow, ncol);

	plen_ = 0;
    vector<int> nonzero_i, nonzero_j;
    m_.nonzeros(nonzero_i, nonzero_j);
	pm_ = new double*[nonzero_i.size()];
	ptree_ = new double*[nonzero_i.size()];
    for (int k = 0; k < nonzero_i.size(); k++) {
        const int i = nonzero_i[k];
        const int j = nonzero_j[k];
		int it;
		if (i < nnode) {
			it = nodes[i] -> eqn_index_ + layer[i];
//printf("i=%d it=%d area=%g\n", i, it, nodes[i]->area);
			if (layer[i] > 0 && !nodes[i] -> extnode) {
				it = 0;
			}
		} else {
			it = start + i - nnode;
		}
        int jt;
        pm_[plen_] = m_.mep(i, j);
        if (j < nnode) {
            jt = nodes[j] -> eqn_index_ + layer[j];
            if (layer[j] > 0 && !nodes[j] -> extnode) {
                jt = 0;
            }
        } else {
            jt = start + j - nnode;
        }
//printf("MatrixMap::alloc getElement(%d,%d)\n", it, jt);
        ptree_[plen_] = spGetElement(_nt->_sp13mat, it, jt);
        ++plen_;
    }
}


