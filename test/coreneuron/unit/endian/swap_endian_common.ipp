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
void run_test_array(T *buf,size_t N,const T &fill_value,const T &check_value) {
    fill(buf,buf+N,fill_value);
    endian::swap_endian_range(buf,buf+N);
    for (size_t i=0;i<N;++i) {
        BOOST_REQUIRE_EQUAL(buf[i],check_value);
    }
}

template <typename T>
void run_test_list(size_t N,const T &fill_value,const T &check_value) {
    std::list<T> data(N,fill_value);
    endian::swap_endian_range(data.begin(),data.end());
    for (typename std::list<T>::const_iterator i=data.begin();i!=data.end();++i) {
        BOOST_REQUIRE_EQUAL(*i,check_value);
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

BOOST_AUTO_TEST_CASE_TEMPLATE(small_collection,T,test_value_types) {
    check_small_containers<T,10>(sample_fill_value<T>(),sample_check_value<T>());
}

/* run test over longer (unrollable) array; using N_guard items at front and back of the
 * buffer to check against overwrites */

template <typename T>
void run_longer_test_array(T *buf,size_t N,size_t N_guard,
                           const T &fill_value,const T &check_value,const T &guard_value)
{
    if (N_guard*2>=N) return;

    T *start=buf+N_guard;
    size_t count=N-2*N_guard;

    fill(buf,buf+N_guard,guard_value);
    fill(start,start+count,fill_value);
    fill(start+count,buf+N,guard_value);

    endian::swap_endian_range(start,start+count);

    for (size_t i=0;i<N_guard;++i) {
        BOOST_REQUIRE_EQUAL(buf[i],guard_value);
        BOOST_REQUIRE_EQUAL(start[count+i],guard_value);
    }

    for (size_t i=0;i<count;++i) {
        BOOST_REQUIRE_EQUAL(start[i],check_value);
    }
}

static const size_t unroll_multiple=123;

template <typename T>
void check_longer_unroll(const T &fill_value,const T &check_value,const T &guard_value) {
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
void check_unroll_badalign(const T &fill_value,const T &check_value,const T &guard_value) {
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
    check_longer_unroll<T>(sample_fill_value<T>(),sample_check_value<T>(),guard_value);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unaligned_array,T,test_value_types) {
    T guard_value;
    fill_with_guard_byte(&guard_value);
    check_unroll_badalign<T>(sample_fill_value<T>(),sample_check_value<T>(),guard_value);
}



