/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
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
    BOOST_CHECK_EQUAL_COLLECTIONS(info1.firstnode, info1.firstnode + nwarp + 1, info1.lastnode,
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

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.stridedispl, info1.stridedispl + nwarp + 1,
                                      infos[i]->stridedispl, infos[i]->stridedispl + nwarp + 1);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.stride, info1.stride + nstride, infos[i]->stride,
                                      infos[i]->stride + nstride);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.cellsize, info1.cellsize + nwarp, infos[i]->cellsize,
                                      infos[i]->cellsize + nwarp);

        BOOST_CHECK_EQUAL_COLLECTIONS(info1.child_race, info1.child_race + nwarp,
                                      infos[i]->child_race, infos[i]->child_race + nwarp);
    }
}
