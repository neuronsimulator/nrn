#include <functional>
#include <algorithm>
#include <queue>

#include "coreneuron/nrnconf.h"  // for size_t
#include "coreneuron/utils/lpt.hpp"
#include "coreneuron/utils/nrn_assert.h"

using P = std::pair<size_t, size_t>;

// always want the largest remaining piece
bool piece_comp(const P& a, const P& b) {
    return a.second > b.second;
}

// always want the smallest bag
struct bag_comp {
    bool operator()(const P& a, const P& b) {
        return a.second > b.second;
    }
};

// lpt Least Processing Time algorithm.
// Largest piece goes into least size bag.
// in: number of bags, vector of sizes
// return: a new vector of bag indices parallel to the vector of sizes.

std::vector<size_t>* lpt(size_t nbag, std::vector<size_t>& pieces, double* bal) {
    nrn_assert(nbag > 0);
    nrn_assert(pieces.size() > 0);

    std::vector<P> pvec;
    for (size_t i = 0; i < pieces.size(); ++i) {
        pvec.push_back(P(i, pieces[i]));
    }
    std::sort(pvec.begin(), pvec.end(), piece_comp);

    std::vector<size_t>* bagindices = new std::vector<size_t>(pieces.size());

    std::priority_queue<P, std::vector<P>, bag_comp> bagq;
    for (size_t i = 0; i < nbag; ++i) {
        bagq.push(P(i, 0));
    }

    for (size_t i = 0; i < pvec.size(); ++i) {
        P& p = pvec[i];
        P bagqitem = bagq.top();
        bagq.pop();
        (*bagindices)[p.first] = bagqitem.first;
        bagqitem.second += p.second;
        bagq.push(bagqitem);
    }

    // load balance average/max (1.0 is perfect)
    std::vector<size_t> v(bagq.size());
    for (size_t i = 1; i < nbag; ++i) {
        v[i] = bagq.top().second;
        bagq.pop();
    }
    double b = load_balance(v);
    if (bal) {
        *bal = b;
    } else {
        printf("load balance = %g for %ld pieces in %ld bags\n", b, pieces.size(), nbag);
    }

    return bagindices;
}

double load_balance(std::vector<size_t>& v) {
    size_t sum = 0;
    size_t max = 1;
    for (size_t i = 0; i < v.size(); ++i) {
        size_t val = v[i];
        sum += val;
        if (max < val) {
            max = val;
        }
    }
    return (double(sum) / v.size()) / max;
}
