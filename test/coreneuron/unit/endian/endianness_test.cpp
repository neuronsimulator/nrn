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
/* confirm that reported native endianness is big- or little-endian, and that
 * this corresponds with our expectations across 16-, 32- and 64-bit integers,
 * and floats and doubles (assumes IEEE 4- and 8- byte representation) */

#define BOOST_TEST_MODULE NativeEndianCheck
#define BOOST_TEST_MAIN

#include <cstring>
#include <stdint.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "coreneuron/utils/endianness.h"

BOOST_AUTO_TEST_CASE(confirm_big_or_little) {
    BOOST_CHECK(endian::little_endian!=endian::big_endian);
    BOOST_CHECK(endian::little_endian!=endian::mixed_endian);
    BOOST_CHECK(endian::big_endian!=endian::mixed_endian);

    BOOST_CHECK(endian::native_endian==endian::big_endian ||
                endian::native_endian==endian::little_endian);
}

template <typename T>
struct check_data {
    static const unsigned char data[sizeof(T)];
    static const T le,be;

    static void confirm_endian(enum endian::endianness E) {
        union {
            unsigned char bytes[sizeof(T)];
            T value;
        } join;

        memcpy(join.bytes,data,sizeof(T));
        switch (E) {
        case endian::little_endian:
            BOOST_CHECK_EQUAL(join.value,le);
            break;
        case endian::big_endian:
            BOOST_CHECK_EQUAL(join.value,be);
            break;
        default:
            BOOST_ERROR("Endian neither little nor big");
        }
    }
};

typedef boost::mpl::list<double,float,char,uint16_t,uint64_t,uint32_t> test_value_types;

#define CHECK_DATA(T,le_,be_)\
template <> const T check_data<T>::le=le_;\
template <> const T check_data<T>::be=be_;\
template <> const unsigned char check_data<T>::data[]

CHECK_DATA(char,'x','x')={'x'};

CHECK_DATA(float,(float)0x1.8a4782p+79,(float)-0x1.468acep+3)=
    {0xc1,0x23,0x45,0x67};
 
CHECK_DATA(double,-0x1.dab89674523c1p+45,-0x1.3456789abcdc2p+19)=
    {0xc1,0x23,0x45,0x67,0x89,0xab,0xcd,0xc2};

CHECK_DATA(uint16_t,0xf12e,0x2ef1)=
    {0x2e,0xf1};

CHECK_DATA(uint32_t,0x192a3b4c,0x4c3b2a19)=
    {0x4c,0x3b,0x2a,0x19};

CHECK_DATA(uint64_t,0xab13cd24ef350146,0x460135ef24cd13ab)=
    {0x46,0x01,0x35,0xef,0x24,0xcd,0x13,0xab};

BOOST_AUTO_TEST_CASE_TEMPLATE(correct_endianness,T,test_value_types) {
    check_data<T>::confirm_endian(endian::native_endian);
}

