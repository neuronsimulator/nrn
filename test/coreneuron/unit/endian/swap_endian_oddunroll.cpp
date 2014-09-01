/* Run endian tests with an unusual, odd unroll factor.
 *
 * In practice, unroll should be left at the default value or set
 * to a power of two. This test ensures correctness even if this
 * is not the case.
 */

#define SWAP_ENDIAN_CONFIG OddUnroll

#define SWAP_ENDIAN_MAX_UNROLL 5
#include "swap_endian_common.ipp"

