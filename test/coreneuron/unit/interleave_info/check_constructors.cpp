/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/permute/cellorder.hpp"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <array>

using namespace coreneuron;

TEST_CASE("interleave_info_test", "[interleave_info]") {
    const size_t nwarp = 4;
    const size_t nstride = 6;

    InterleaveInfo info1;

    std::array<int, 10> data1 = {11, 37, 45, 2, 18, 37, 7, 39, 66, 33};
    std::array<size_t, 9> data2 = {111, 137, 245, 12, 118, 237, 199, 278, 458};

    info1.nwarp = nwarp;
    info1.nstride = nstride;

    // to avoid same values, different sub-array is used to initialize different members
    copy_align_array(info1.stridedispl, data1.data(), nwarp + 1);
    copy_align_array(info1.stride, data1.data() + 1, nstride);
    copy_align_array(info1.firstnode, data1.data() + 1, nwarp + 1);
    copy_align_array(info1.lastnode, data1.data() + 1, nwarp + 1);

    // check if copy_array works
    REQUIRE(info1.firstnode != info1.lastnode);
    REQUIRE(std::equal(
        info1.firstnode, info1.firstnode + nwarp + 1, info1.lastnode, info1.lastnode + nwarp + 1));

    copy_align_array(info1.cellsize, data1.data() + 4, nwarp);
    copy_array(info1.nnode, data2.data(), nwarp);
    copy_array(info1.ncycle, data2.data() + 1, nwarp);
    copy_array(info1.idle, data2.data() + 2, nwarp);
    copy_array(info1.cache_access, data2.data() + 3, nwarp);
    copy_array(info1.child_race, data2.data() + 4, nwarp);

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
        REQUIRE(info1.nwarp == infos[i]->nwarp);
        REQUIRE(info1.nstride == infos[i]->nstride);

        REQUIRE(
            std::equal(info1.stridedispl, info1.stridedispl + nwarp + 1, infos[i]->stridedispl));
        REQUIRE(std::equal(info1.stride, info1.stride + nstride, infos[i]->stride));
        REQUIRE(std::equal(info1.cellsize, info1.cellsize + nwarp, infos[i]->cellsize));
        REQUIRE(std::equal(info1.child_race, info1.child_race + nwarp, infos[i]->child_race));
    }
}
