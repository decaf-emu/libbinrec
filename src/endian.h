/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef SRC_ENDIAN_H
#define SRC_ENDIAN_H

#ifndef SRC_COMMON_H
    #include "src/common.h"
#endif

/*
 * This header provides functions for converting values between different
 * byte orders.  They are divided into three groups:
 *
 * - bswapN (N = 16, 32, 64): Convert between big-endian and little-endian.
 *
 * - bswap_beN: Convert between big-endian and native byte order.  (Here,
 *   "native" refers to the architecture of the computer on which this
 *   library is running, not the translation target architecture.)
 *
 * - bswap_leN: Convert between little-endian and native byte order.
 *
 * Normally, compiler-specific intrinsic functions are used when possible.
 * To suppress these (for testing the fallback code), define the
 * SUPPRESS_ENDIAN_INTRINSICS preprocessor symbol when compiling.
 *
 * This header also provides the is_little_endian() function which returns
 * true if the native environment is little-endian, false if big-endian.
 */

/*************************************************************************/
/*************************************************************************/

/**
 * is_little_endian:  Return whether the native environment is little-endian.
 */
static ALWAYS_INLINE CONST_FUNCTION bool is_little_endian(void) {
    return *(uint16_t *)"\1" == 1;  // Optimizes to a compile-time constant.
}

/*-----------------------------------------------------------------------*/

/**
 * bswap16:  Reverse the byte order of the given 16-bit value.
 */
static ALWAYS_INLINE CONST_FUNCTION uint16_t bswap16(uint16_t x)
{
    #ifndef SUPPRESS_ENDIAN_INTRINSICS
        #if IS_GCC(4,8)
            return __builtin_bswap16(x);
        #elif IS_MSVC(1,0)
            return _byteswap_ushort(x);
        #endif
    #endif
    return (x >> 8) | (x << 8);
}

/**
 * bswap32:  Reverse the byte order of the given 32-bit value.
 */
static ALWAYS_INLINE CONST_FUNCTION uint32_t bswap32(uint32_t x)
{
    #ifndef SUPPRESS_ENDIAN_INTRINSICS
        #if IS_ICC(1,0)  // Must precede IS_GCC test.
            return _bswap(x);
        #elif IS_GCC(4,3)
            return __builtin_bswap32(x);
        #elif IS_MSVC(1,0)
            return _byteswap_ulong(x);
        #endif
    #endif
    return (x >> 24) | ((x >> 8) & 0xFF00) | ((x & 0xFF00) << 8) | (x << 24);
}

/**
 * bswap64:  Reverse the byte order of the given 64-bit value.
 */
static ALWAYS_INLINE CONST_FUNCTION uint64_t bswap64(uint64_t x)
{
    #ifndef SUPPRESS_ENDIAN_INTRINSICS
        #if IS_GCC(4,3)
            return __builtin_bswap64(x);
        #elif IS_MSVC(1,0)
            return _byteswap_uint64(x);
        #endif
    #endif
    return (x >> 56)
        | ((x >> 40) & 0xFF00)
        | ((x >> 24) & 0xFF0000)
        | ((x >>  8) & 0xFF000000)
        | ((x & 0xFF000000) << 8)
        | ((x & 0xFF0000) << 24)
        | ((x & 0xFF00) << 40)
        |  (x << 56);
}

/*-----------------------------------------------------------------------*/

/**
 * bswap_be16:  Convert the given 16-bit value between big-endian and
 * native byte order.
 */
static ALWAYS_INLINE CONST_FUNCTION uint16_t bswap_be16(uint16_t x) {
    return is_little_endian() ? bswap16(x) : x;
}

/**
 * bswap_be32:  Convert the given 32-bit value between big-endian and
 * native byte order.
 */
static ALWAYS_INLINE CONST_FUNCTION uint32_t bswap_be32(uint32_t x) {
    return is_little_endian() ? bswap32(x) : x;
}

/**
 * bswap_be64:  Convert the given 64-bit value between big-endian and
 * native byte order.
 */
static ALWAYS_INLINE CONST_FUNCTION uint64_t bswap_be64(uint64_t x) {
    return is_little_endian() ? bswap64(x) : x;
}

/*-----------------------------------------------------------------------*/

/**
 * bswap_le16:  Convert the given 16-bit value between big-endian and
 * native byte order.
 */
static ALWAYS_INLINE CONST_FUNCTION uint16_t bswap_le16(uint16_t x) {
    return is_little_endian() ? x : bswap16(x);
}

/**
 * bswap_le32:  Convert the given 32-bit value between big-endian and
 * native byte order.
 */
static ALWAYS_INLINE CONST_FUNCTION uint32_t bswap_le32(uint32_t x) {
    return is_little_endian() ? x : bswap32(x);
}

/**
 * bswap_le64:  Convert the given 64-bit value between big-endian and
 * native byte order.
 */
static ALWAYS_INLINE CONST_FUNCTION uint64_t bswap_le64(uint64_t x) {
    return is_little_endian() ? x : bswap64(x);
}

/*************************************************************************/
/*************************************************************************/

#endif  // SRC_ENDIAN_H
