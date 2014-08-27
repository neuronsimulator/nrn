#ifndef endianness_h
#define endianness_h

#include <stdint.h>

namespace endian {

    enum endianness {
        little_endian,big_endian,mixed_endian
    };

    static const union {
        unsigned char bytes[4];
        uint32_t value;
    } endian_check = {{0,1,2,3}};

    static const enum endianness native_endian=
        endian_check.value==0x03020100ul?little_endian:
        endian_check.value==0x00010203ul?big_endian:
        mixed_endian;

    static inline bool is_little_endian() { return native_endian==little_endian; }
    static inline bool is_big_endian() { return native_endian==big_endian; }

} // namespace endian

#endif // ifndef endianness_h
