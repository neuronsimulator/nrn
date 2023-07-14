/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/utils/memory.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <cstdint>
#include <cstring>
#include <tuple>

template <class T, int n = 1>
struct data {
    typedef T value_type;
    static const int chunk = n;
};

typedef std::tuple<data<double>, data<long long int>> chunk_default_data_type;

typedef std::tuple<data<double, 2>,
                   data<double, 4>,
                   data<double, 8>,
                   data<double, 16>,
                   data<double, 32>,
                   data<int, 2>,
                   data<int, 4>,
                   data<int, 8>,
                   data<int, 16>,
                   data<int, 32>>
    chunk_data_type;

TEST_CASE("padding_simd", "[PaddingCheck]") {
    /** AOS test */
    int pad = coreneuron::soa_padded_size<1>(11, 1);
    REQUIRE(pad == 11);

    /** SOA tests with 11 */
    pad = coreneuron::soa_padded_size<1>(11, 0);
    REQUIRE(pad == 11);

    pad = coreneuron::soa_padded_size<2>(11, 0);
    REQUIRE(pad == 12);

    pad = coreneuron::soa_padded_size<4>(11, 0);
    REQUIRE(pad == 12);

    pad = coreneuron::soa_padded_size<8>(11, 0);
    REQUIRE(pad == 16);

    pad = coreneuron::soa_padded_size<16>(11, 0);
    REQUIRE(pad == 16);

    pad = coreneuron::soa_padded_size<32>(11, 0);
    REQUIRE(pad == 32);

    /** SOA tests with 32 */
    pad = coreneuron::soa_padded_size<1>(32, 0);
    REQUIRE(pad == 32);

    pad = coreneuron::soa_padded_size<2>(32, 0);
    REQUIRE(pad == 32);

    pad = coreneuron::soa_padded_size<4>(32, 0);
    REQUIRE(pad == 32);

    pad = coreneuron::soa_padded_size<8>(32, 0);
    REQUIRE(pad == 32);

    pad = coreneuron::soa_padded_size<16>(32, 0);
    REQUIRE(pad == 32);

    pad = coreneuron::soa_padded_size<32>(32, 0);
    REQUIRE(pad == 32);

    /** SOA tests with 33 */
    pad = coreneuron::soa_padded_size<1>(33, 0);
    REQUIRE(pad == 33);

    pad = coreneuron::soa_padded_size<2>(33, 0);
    REQUIRE(pad == 34);

    pad = coreneuron::soa_padded_size<4>(33, 0);
    REQUIRE(pad == 36);

    pad = coreneuron::soa_padded_size<8>(33, 0);
    REQUIRE(pad == 40);

    pad = coreneuron::soa_padded_size<16>(33, 0);
    REQUIRE(pad == 48);

    pad = coreneuron::soa_padded_size<32>(33, 0);
    REQUIRE(pad == 64);
}

/// Even number is randomly depends of the TYPE!!! and the number of elements.
/// This test work for 64 bits type not for 32 bits.
TEMPLATE_LIST_TEST_CASE("memory_alignment_simd_false",
                        "[memory_alignment_simd_false]",
                        chunk_default_data_type) {
    const int c = TestType::chunk;
    int total_size_chunk = coreneuron::soa_padded_size<c>(247, 0);
    int ne = 6 * total_size_chunk;

    typename TestType::value_type* data = (typename TestType::value_type*)
        coreneuron::ecalloc_align(ne, sizeof(typename TestType::value_type), 16);

    for (int i = 1; i < 6; i += 2) {
        bool b = coreneuron::is_aligned((data + i * total_size_chunk), 16);
        REQUIRE_FALSE(b);
    }

    for (int i = 0; i < 6; i += 2) {
        bool b = coreneuron::is_aligned((data + i * total_size_chunk), 16);
        REQUIRE(b);
    }

    free_memory(data);
}

TEMPLATE_LIST_TEST_CASE("memory_alignment_simd_true",
                        "[memory_alignment_simd_true]",
                        chunk_data_type) {
    const int c = TestType::chunk;
    int total_size_chunk = coreneuron::soa_padded_size<c>(247, 0);
    int ne = 6 * total_size_chunk;

    typename TestType::value_type* data = (typename TestType::value_type*)
        coreneuron::ecalloc_align(ne, sizeof(typename TestType::value_type), 16);

    for (int i = 0; i < 6; ++i) {
        bool b = coreneuron::is_aligned((data + i * total_size_chunk), 16);
        REQUIRE(b);
    }

    free_memory(data);
}
