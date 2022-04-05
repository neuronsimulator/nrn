/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef test_header_hpp
#define test_header_hpp

#include <boost/mpl/list.hpp>
#include "coreneuron/nrniv/sptbinq.h"

template <container C>
struct data {
    static const container cont = C;
};

using full_test_types = boost::mpl::list<data<queueing::spltree>, data<queueing::pq_que>>;

#endif
