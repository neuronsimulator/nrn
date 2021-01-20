/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#define BOOST_TEST_MODULE cmdline_interface
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "coreneuron/permute/cellorder.hpp"
using namespace coreneuron;
BOOST_AUTO_TEST_CASE(interleave_info_test) {
    size_t nwarp = 4;
    size_t nstride = 6;

    InterleaveInfo info1;

    int data1[] = {11, 37, 45, 2, 18, 37, 7, 39, 66, 33};
    size_t data2[] = {111, 137, 245, 12, 118, 237, 199, 278, 458};

    info1.nwarp = nwarp;
    info1.nstride = nstride;

    // to avoid same values, different sub-array is used to initialize different members
    copy_array(info1.stridedispl, data1, nwarp + 1);
    copy_array(info1.stride, data1 + 1, nstride);
    copy_array(info1.firstnode, data1 + 1, nwarp + 1);
    copy_array(info1.lastnode, data1 + 1, nwarp + 1);

    // check if copy_array works
    BOOST_CHECK_NE(info1.firstnode, info1.lastnode);
    BOOST_CHECK_EQUAL_COLLECTIONS(info1.firstnode,
                                  info1.firstnode + nwarp + 1,
                                  info1.lastnode,
                                  info1.lastnode + nwarp + 1);

    copy_array(info1.cellsize, data1 + 4, nwarp);
    copy_array(info1.nnode, data2, nwarp);
    copy_array(info1.ncycle, data2 + 1, nwarp);
    copy_array(info1.idle, data2 + 2, nwarp);
    copy_array(info1.cache_access, data2 + 3, nwarp);
    copy_array(info1.child_race, data2 + 4, nwarp);

    // copy constructor
    InterleaveInfo info2(info1);

    // assignment operator
    InterleaveInfo info3;
    info3 = info1;

    std::vector<InterleaveInfo*> infos;

    infos.push_back(&info2);
    infos.push_back(&info3);

    // test few members
    for (size_t i = 0; i < infos.size(); i++) {
        BOOST_CHECK_EQUAL(info1.nwarp, infos[i]->nwarp);
        BOOST_CHECK_EQUAL(info1.nstride, infos[i]->nstride);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.stridedispl,
                                      info1.stridedispl + nwarp + 1,
                                      infos[i]->stridedispl,
                                      infos[i]->stridedispl + nwarp + 1);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.stride,
                                      info1.stride + nstride,
                                      infos[i]->stride,
                                      infos[i]->stride + nstride);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.cellsize,
                                      info1.cellsize + nwarp,
                                      infos[i]->cellsize,
                                      infos[i]->cellsize + nwarp);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.child_race,
                                      info1.child_race + nwarp,
                                      infos[i]->child_race,
                                      infos[i]->child_race + nwarp);
    }
}
