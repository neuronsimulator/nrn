/* Run endian tests with unroll factor forced to be one.
 */

#define SWAP_ENDIAN_CONFIG NoUnroll
#define SWAP_ENDIAN_MAX_UNROLL 1

#include "swap_endian_common.ipp"

