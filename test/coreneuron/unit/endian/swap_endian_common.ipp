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


/* Wish to run unit tests for swap_endian code under multiple configurations:
 * 1. default: SWAP_ENDIAN_DISABLE_ASM and SWAP_ENDIAN_MAX_UNROLL are undef.
 * 2. using fallback methods, with SWAP_ENDIAN_DISABLE_ASM defined.
 * 3. using a strange unroll factor, with SWAP_ENDIAN_DISABLE_ASM set to 7.
 *
 * This file presents the common code for testing, to be included in
 * the specific test code for each of the above scenarios.
 */

#define SWAP_ENDIAN_ASSERT
#include "coreneuron/utils/swap_endian.h"

#ifndef SWAP_ENDIAN_CONFIG
#define SWAP_ENDIAN_CONFIG _
#endif
#define CONCAT(a,b) a##b
#define CONCAT2(a,b) CONCAT(a,b)

#define BOOST_TEST_MODULE CONCAT2(SwapEndian,SWAP_ENDIAN_CONFIG)
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <list>
#include <vector>
#include <cstring>

#include <stdint.h>

#ifdef SWAP_ENDIAN_BROKEN_MEMCPY
#define memcpy(d,s,n) ::endian::impl::safe_memcpy((d),(s),(n))
#endif

/* Some of the tests require an unaligned fill: can't ask std::fill to
 * do this and not expect issues with alignment */
template <typename T>
void fill(T *b,T *e,T value) {
    while (b<e) memcpy(b++,&value,sizeof(T));
}

template <typename T>
void run_test_array(T *buf,size_t N,T (*fill_value)(unsigned),T (*check_value)(unsigned)) {
    for (size_t i=0;i<N;++i) buf[i]=fill_value(i);
    endian::swap_endian_range(buf,buf+N);
    for (size_t i=0;i<N;++i) {
        BOOST_REQUIRE_EQUAL(buf[i],check_value(i));
    }
}

template <typename T>
void run_test_list(size_t N,T (*fill_value)(unsigned),T (*check_value)(unsigned)) {
    std::list<T> data(N);
    unsigned j=0;
    for (typename std::list<T>::iterator i=data.begin();i!=data.end();++i) *i=fill_value(j++);

    endian::swap_endian_range(data.begin(),data.end());

    j=0;
    for (typename std::list<T>::const_iterator i=data.begin();i!=data.end();++i) {
        BOOST_REQUIRE_EQUAL(*i,check_value(j++));
    }
}

template <typename T,size_t MAXN>
void check_small_containers(T (*fill_value)(unsigned),T (*check_value)(unsigned)) {
    std::vector<T> data(MAXN);

    for (size_t i=0;i<MAXN;++i) {
        run_test_array(&data[0],i,fill_value,check_value);
        run_test_list(i,fill_value,check_value);
    }
}

typedef boost::mpl::list<double,float,char,uint16_t,uint64_t,uint32_t> test_value_types;

template <typename V> V sample_fill_value(unsigned i=0);
template <typename V> V sample_check_value(unsigned i=0);

#define NELEM(arr) (sizeof(arr)/sizeof(arr[0]))

template <> double sample_fill_value<double>(unsigned i)  {
    double table[]={
        -0x1.dab89674523c1p+45,
        0x1.2312f1e0dfcebp+309,
        -0x1.cd13e90023347p-169,
        0x1.6e72aa9c692b5p+92,
        0x1.5634ec623974fp-331
    };
    return table[i%NELEM(table)];
}
template <> double sample_check_value<double>(unsigned i) {
    double table[]={
        -0x1.3456789abcdc2p+19,
        -0x1.c0d1e2f314253p+704,
        0x1.302903ed16cb5p+116,
        -0x1.2c6a92ae7b645p-166,
        0x1.723c64e63452bp+250
    };
    return table[i%NELEM(table)];
}

template <> uint64_t sample_fill_value<uint64_t>(unsigned i)  {
    uint64_t table[]={
        0x123456789abcdef0ull,
        0x5342312f1e0dfcebull,
        0xb56cd13e90023347ull,
        0x45b6e72aa9c692b5ull,
        0x2b45634ec623974full
    };
    return table[i%NELEM(table)];
}

template <> uint64_t sample_check_value<uint64_t>(unsigned i) {
    uint64_t table[]={
        0xf0debc9a78563412ull,
        0xebfc0d1e2f314253ull,
        0x473302903ed16cb5ull,
        0xb592c6a92ae7b645ull,
        0x4f9723c64e63452bull
    };
    return table[i%NELEM(table)];
}


template <> uint32_t sample_fill_value<uint32_t>(unsigned i)  {
    uint32_t table[]={
        0x1234567ful,
        0x89abcde0ul,
        0x11223344ul
    };
    return table[i%NELEM(table)];
}

template <> uint32_t sample_check_value<uint32_t>(unsigned i) {
    uint32_t table[]={
        0x7f563412ul,
        0xe0cdab89ul,
        0x44332211ul
    };
    return table[i%NELEM(table)];
}


template <> float sample_fill_value<float>(unsigned i)  {
    float table[]={
        (float)0x1.8a4782p+79,
        (float)0x1.68acfp-91,
        (float)-0x1.06e8d8p-90
    };
    return table[i%NELEM(table)];
}

template <> float sample_check_value<float>(unsigned i) {
    float table[]={
        (float)-0x1.468acep+3,
        (float)0x1.ac6824p+113,
        (float)0x1.e90724p+89
    };
    return table[i%NELEM(table)];
}


template <> uint16_t sample_fill_value<uint16_t>(unsigned i)  {
    uint16_t table[]={
        0xb2c6u,
        0xa391u,
        0x3456u
    };
    return table[i%NELEM(table)];
}

template <> uint16_t sample_check_value<uint16_t>(unsigned i) {
    uint16_t table[]={
        0xc6b2u,
        0x91a3u,
        0x5634u
    };
    return table[i%NELEM(table)];
}


template <> char sample_fill_value<char>(unsigned i)  {
    char table[]={
        'z','a','/'
    };
    return table[i%NELEM(table)];
}

template <> char sample_check_value<char>(unsigned i) {
    char table[]={
        'z','a','/'
    };
    return table[i%NELEM(table)];
}


BOOST_AUTO_TEST_CASE_TEMPLATE(small_collection,T,test_value_types) {
    check_small_containers<T,10>(sample_fill_value<T>,sample_check_value<T>);
}

/* run test over longer (unrollable) array; using N_guard items at front and back of the
 * buffer to check against overwrites */

template <typename T>
void run_longer_test_array(T *buf,size_t N,size_t N_guard,
                           T (*fill_value)(unsigned),T (*check_value)(unsigned),const T &guard_value)
{
    if (N_guard*2>=N) return;

    T *start=buf+N_guard;
    size_t count=N-2*N_guard;

    fill(buf,buf+N_guard,guard_value);
    for (size_t i=0;i<count;++i)
        start[i]=fill_value(i);

    fill(start+count,buf+N,guard_value);

    endian::swap_endian_range(start,start+count);

    for (size_t i=0;i<N_guard;++i) {
        BOOST_REQUIRE_EQUAL(buf[i],guard_value);
        BOOST_REQUIRE_EQUAL(start[count+i],guard_value);
    }

    for (size_t i=0;i<count;++i) {
        BOOST_REQUIRE_EQUAL(start[i],check_value(i));
    }
}

static const size_t unroll_multiple=123;

template <typename T>
void check_longer_unroll(T (*fill_value)(unsigned),T (*check_value)(unsigned),const T &guard_value) {
    std::vector<T> data((1+unroll_multiple)*SWAP_ENDIAN_MAX_UNROLL);
    
    size_t n_guard=8;
    for (size_t n_over=0;n_over<SWAP_ENDIAN_MAX_UNROLL;++n_over) {
        run_longer_test_array(&data[0],unroll_multiple*SWAP_ENDIAN_MAX_UNROLL+n_over,n_guard,fill_value,check_value,guard_value);
    }

    for (size_t n_off=1;n_off<SWAP_ENDIAN_MAX_UNROLL;++n_off) {
        run_longer_test_array(&data[0]+n_off,unroll_multiple*SWAP_ENDIAN_MAX_UNROLL,n_guard,fill_value,check_value,guard_value);
    }
}


template <typename T>
void check_unroll_badalign(T (*fill_value)(unsigned),T (*check_value)(unsigned),const T &guard_value) {
    static const size_t n_item=unroll_multiple*SWAP_ENDIAN_MAX_UNROLL;
    std::vector<unsigned char> bytes(sizeof(T)*(n_item+1));
    
    size_t n_guard=3;
    for (size_t offset=0;offset<sizeof(T);++offset) {
        T *data=(T *)(void *)(&bytes[offset]);
        run_longer_test_array(data,n_item,n_guard,fill_value,check_value,guard_value);
    }
}

template <typename T>
void fill_with_guard_byte(T *value) {
    unsigned char bytes[sizeof(T)];
    for (size_t i=0;i<sizeof(T);++i) bytes[i]=(unsigned char)(0x5a+i);
    memcpy(value,bytes,sizeof(T));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unrolled_array,T,test_value_types) {
    T guard_value;
    fill_with_guard_byte(&guard_value);
    check_longer_unroll<T>(sample_fill_value<T>,sample_check_value<T>,guard_value);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unaligned_array,T,test_value_types) {
    T guard_value;
    fill_with_guard_byte(&guard_value);
    check_unroll_badalign<T>(sample_fill_value<T>,sample_check_value<T>,guard_value);
}



