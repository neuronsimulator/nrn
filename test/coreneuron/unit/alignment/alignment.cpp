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


#define BOOST_TEST_MODULE PaddingCheck
#define BOOST_TEST_MAIN

#include <cstring>
#include <stdint.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "coreneuron/utils/memory.h"

template<class T, int n=1>
struct data{
  typedef T value_type;
  static const int chunk=n;
};

typedef boost::mpl::list<
                          data<double>,
                          data<long long int>
                        > chunk_default_data_type;

typedef boost::mpl::list<
                          data<double, 2>,
                          data<double, 4>,
                          data<double, 8>,
                          data<double, 16>,
                          data<double, 32>, 
                          data<int, 2>,
                          data<int, 4>,
                          data<int, 8>,
                          data<int, 16>,
                          data<int, 32>
                        > chunk_data_type;

BOOST_AUTO_TEST_CASE(padding_simd) {

  /** AOS test */
  int pad = coreneuron::soa_padded_size<1>(11, 1);
  BOOST_CHECK_EQUAL(pad,11);	

  /** SOA tests with 11 */
  pad = coreneuron::soa_padded_size<1>(11, 0);
  BOOST_CHECK_EQUAL(pad,11);	

  pad = coreneuron::soa_padded_size<2>(11, 0);
  BOOST_CHECK_EQUAL(pad,12);	

  pad = coreneuron::soa_padded_size<4>(11, 0);
  BOOST_CHECK_EQUAL(pad,12);	

  pad = coreneuron::soa_padded_size<8>(11, 0);
  BOOST_CHECK_EQUAL(pad,16);	

  pad = coreneuron::soa_padded_size<16>(11, 0);
  BOOST_CHECK_EQUAL(pad,16);	

  pad = coreneuron::soa_padded_size<32>(11, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  /** SOA tests with 32 */
  pad = coreneuron::soa_padded_size<1>(32, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  pad = coreneuron::soa_padded_size<2>(32, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  pad = coreneuron::soa_padded_size<4>(32, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  pad = coreneuron::soa_padded_size<8>(32, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  pad = coreneuron::soa_padded_size<16>(32, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  pad = coreneuron::soa_padded_size<32>(32, 0);
  BOOST_CHECK_EQUAL(pad,32);	

  /** SOA tests with 33 */
  pad = coreneuron::soa_padded_size<1>(33, 0);
  BOOST_CHECK_EQUAL(pad,33);	

  pad = coreneuron::soa_padded_size<2>(33, 0);
  BOOST_CHECK_EQUAL(pad,34);	

  pad = coreneuron::soa_padded_size<4>(33, 0);
  BOOST_CHECK_EQUAL(pad,36);	

  pad = coreneuron::soa_padded_size<8>(33, 0);
  BOOST_CHECK_EQUAL(pad,40);	

  pad = coreneuron::soa_padded_size<16>(33, 0);
  BOOST_CHECK_EQUAL(pad,48);	

  pad = coreneuron::soa_padded_size<32>(33, 0);
  BOOST_CHECK_EQUAL(pad,64);	

}

/// Even number is randomly depends of the TYPE!!! and the number of elements.
/// This test work for 64 bits type not for 32 bits.
BOOST_AUTO_TEST_CASE_TEMPLATE(memory_alignment_simd_false, T, chunk_default_data_type){
  const int c = T::chunk;
  int total_size_chunk = coreneuron::soa_padded_size<c>(247, 0);
  int ne = 6*total_size_chunk;

  typename T::value_type* data = (typename T::value_type*)coreneuron::ecalloc_align(ne, sizeof(typename T::value_type), 16);

  for(int i=1; i < 6; i+=2){
      bool b = coreneuron::is_aligned((data+i*total_size_chunk),16);
      BOOST_CHECK_EQUAL(b,0);
  }

  for(int i=0; i < 6; i+=2){
      bool b = coreneuron::is_aligned((data+i*total_size_chunk),16);
      BOOST_CHECK_EQUAL(b,1);
  }

  free_memory(data);

}

BOOST_AUTO_TEST_CASE_TEMPLATE(memory_alignment_simd_true, T, chunk_data_type){
  const int c = T::chunk;
  int total_size_chunk = coreneuron::soa_padded_size<c>(247, 0);
  int ne = 6*total_size_chunk;

  typename T::value_type* data = (typename T::value_type*)coreneuron::ecalloc_align(ne, sizeof(typename T::value_type), 16);

  for(int i=0; i < 6; ++i){
      bool b = coreneuron::is_aligned((data+i*total_size_chunk),16);
      BOOST_CHECK_EQUAL(b,1);
  }

  free_memory(data);
}
