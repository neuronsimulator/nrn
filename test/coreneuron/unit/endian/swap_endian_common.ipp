/* Wish to run unit tests for swap_endian code under multiple configurations:
 * 1. default: SWAP_ENDIAN_DISABLE_ASM and SWAP_ENDIAN_MAX_UNROLL are undef.
 * 2. using fallback methods, with SWAP_ENDIAN_DISABLE_ASM defined.
 * 3. using a strange unroll factor, with SWAP_ENDIAN_DISABLE_ASM set to 7.
 *
 * This file presents the common code for testing, to be included in
 * the specific test code for each of the above scenarios.
 */

#define SWAP_ENDIAN_ASSERT
#include "utils/swap_endian.h"

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
#include <algorithm>

#include <stdint.h>

template <typename T>
void run_test_array(T *buf,size_t N,const T &fill_value,const T &check_value) {
    std::fill(buf,buf+N,fill_value);
    endian::swap_endian_range(buf,buf+N);
    for (size_t i=0;i<N;++i) {
        BOOST_CHECK_EQUAL(buf[i],check_value);
    }
}

template <typename T>
void run_test_list(size_t N,const T &fill_value,const T &check_value) {
    std::list<T> data(N,fill_value);
    endian::swap_endian_range(data.begin(),data.end());
    for (typename std::list<T>::const_iterator i=data.begin();i!=data.end();++i) {
        BOOST_CHECK_EQUAL(*i,check_value);
    }
}

template <typename T,size_t MAXN>
void check_small_containers(const T &fill_value,const T &check_value) {
    std::vector<T> data(MAXN);

    for (size_t i=0;i<MAXN;++i) {
        run_test_array(&data[0],i,fill_value,check_value);
        run_test_list(i,fill_value,check_value);
    }
}

//typedef boost::mpl::list<double,uint64_t,uint32_t,float,uint16_t,char> test_value_types;
typedef boost::mpl::list<double,float,char,uint16_t,uint64_t,uint32_t> test_value_types;

template <typename V> V sample_fill_value();
template <typename V> V sample_check_value();

template <> double sample_fill_value<double>()  { return -0x1.dab89674523c1p+45; }
template <> double sample_check_value<double>() { return -0x1.3456789abcdc2p+19; }

template <> uint64_t sample_fill_value<uint64_t>()  { return 0x123456789abcdef0ull; }
template <> uint64_t sample_check_value<uint64_t>() { return 0xf0debc9a78563412ull; }

template <> uint32_t sample_fill_value<uint32_t>()  { return 0x1234567ful; }
template <> uint32_t sample_check_value<uint32_t>() { return 0x7f563412ul; }

template <> float sample_fill_value<float>()  { return (float)0x1.8a4782p+79; }
template <> float sample_check_value<float>() { return (float)-0x1.468acep+3; }

template <> uint16_t sample_fill_value<uint16_t>()  { return 0xb2c6u; }
template <> uint16_t sample_check_value<uint16_t>() { return 0xc6b2u; }

template <> char sample_fill_value<char>()  { return 'z'; }
template <> char sample_check_value<char>() { return 'z'; }

BOOST_AUTO_TEST_CASE_TEMPLATE(small_vector,T,test_value_types) {
    check_small_containers<T,10>(sample_fill_value<T>(),sample_check_value<T>());
}

/* TODO: Add long test; long test multiple of UNROLL; long test not multiple; long test with poor alignments */


