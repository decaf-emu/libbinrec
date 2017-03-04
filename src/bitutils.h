/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef SRC_BITUTILS_H
#define SRC_BITUTILS_H

#ifndef SRC_COMMON_H
    #include "src/common.h"
#endif
#ifndef SRC_ENDIAN_H
    #include "src/endian.h"
#endif

/*
 * Similarly to the endian conversion functions, the compiler-specific
 * intrinsics used here can be suppressed for testing by defining the
 * preprocessor symbol SUPPRESS_BITUTILS_INTRINSICS.
 */

/*************************************************************************/
/*************************************************************************/

/**
 * bitrev32:  Reverse the order of the bits in the given value.
 */
static ALWAYS_INLINE CONST_FUNCTION uint32_t bitrev32(uint32_t x)
{
    x = bswap32(x);
    x = (x & 0xF0F0F0F0) >> 4 | (x & 0x0F0F0F0F) << 4;
    x = (x & 0xCCCCCCCC) >> 2 | (x & 0x33333333) << 2;
    x = (x & 0xAAAAAAAA) >> 1 | (x & 0x55555555) << 1;
    return x;
}

/*-----------------------------------------------------------------------*/

/**
 * clz32, clz64:  Return the number of leading (high-end) zero bits in the
 * given value.  If the value is zero, 32 or 64 (as appropriate) is returned.
 */
static ALWAYS_INLINE CONST_FUNCTION int clz32(uint32_t x)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if IS_GCC(3,4) || CLANG_HAS_BUILTIN(__builtin_clz)
            STATIC_ASSERT(sizeof(int) == 4, "Non-32-bit int is not supported");
            if (x) {
                return __builtin_clz(x);
            } else {
                return 32;
            }
        #elif defined(_MSC_VER)
            unsigned long n;
            if (_BitScanReverse(&n, x)) {
                return 31 - n;
            } else {
                return 32;
            }
        #endif
    #endif
    int count = 0;
    if (!(x & 0xFFFF0000)) {
        count += 16;
        x <<= 16;
    }
    if (!(x & 0xFF000000)) {
        count += 8;
        x <<= 8;
    }
    if (!(x & 0xF0000000)) {
        count += 4;
        x <<= 4;
    }
    if (!(x & 0xC0000000)) {
        count += 2;
        x <<= 2;
    }
    if (!(x & 0x80000000)) {
        count += 1;
        x <<= 1;
    }
    if (!(x & 0x80000000)) {
        count += 1;
    }
    return count;
}

static ALWAYS_INLINE CONST_FUNCTION int clz64(uint64_t x)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if IS_GCC(3,4) || CLANG_HAS_BUILTIN(__builtin_clzll)
            if (x) {
                return __builtin_choose_expr(
                    sizeof(long) == 8,
                    __builtin_clzl(x),
                    __extension__({
                        STATIC_ASSERT(sizeof(long long) == 8,
                                      "Non-64-bit long long is not supported");
                        __builtin_clzll(x);
                    }));
            } else {
                return 64;
            }
        #elif defined(_MSC_VER)
            unsigned long n;
            if (_BitScanReverse64(&n, x)) {
                return 63 - n;
            } else {
                return 64;
            }
        #endif
    #endif
    if (x >> 32 == 0) {
        return 32 + clz32((uint32_t)x);
    } else {
        return clz32(x >> 32);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * ctz32, ctz64:  Return the number of leading (high-end) zero bits in the
 * given value.  If the value is zero, 32 or 64 (as appropriate) is returned.
 */
static ALWAYS_INLINE CONST_FUNCTION int ctz32(uint32_t x)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if IS_GCC(3,4) || CLANG_HAS_BUILTIN(__builtin_ctz)
            STATIC_ASSERT(sizeof(int) == 4, "Non-32-bit int is not supported");
            if (x) {
                return __builtin_ctz(x);
            } else {
                return 32;
            }
        #elif defined(_MSC_VER)
            unsigned long n;
            if (_BitScanForward(&n, x)) {
                return n;
            } else {
                return 32;
            }
        #endif
    #endif
    int count = 0;
    if (!(x & 0x0000FFFF)) {
        count += 16;
        x >>= 16;
    }
    if (!(x & 0x000000FF)) {
        count += 8;
        x >>= 8;
    }
    if (!(x & 0x0000000F)) {
        count += 4;
        x >>= 4;
    }
    if (!(x & 0x00000003)) {
        count += 2;
        x >>= 2;
    }
    if (!(x & 0x00000001)) {
        count += 1;
        x >>= 1;
    }
    if (!(x & 0x00000001)) {
        count += 1;
    }
    return count;
}

static ALWAYS_INLINE CONST_FUNCTION int ctz64(uint64_t x)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if IS_GCC(3,4) || CLANG_HAS_BUILTIN(__builtin_ctzll)
            if (x) {
                return __builtin_choose_expr(
                    sizeof(long) == 8,
                    __builtin_ctzl(x),
                    __extension__({
                        STATIC_ASSERT(sizeof(long long) == 8,
                                      "Non-64-bit long long is not supported");
                        __builtin_ctzll(x);
                    }));
            } else {
                return 64;
            }
        #elif defined(_MSC_VER)
            unsigned long n;
            if (_BitScanForward64(&n, x)) {
                return n;
            } else {
                return 64;
            }
        #endif
    #endif
    if ((uint32_t)x == 0) {
        return 32 + ctz32((uint32_t)(x >> 32));
    } else {
        return ctz32((uint32_t)x);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * popcnt32, popcnt64:  Return the number of set (1) bits in the given value.
 */
static ALWAYS_INLINE CONST_FUNCTION int popcnt32(uint32_t x)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if IS_GCC(3,4) || CLANG_HAS_BUILTIN(__builtin_popcount)
            STATIC_ASSERT(sizeof(int) == 4, "Non-32-bit int is not supported");
            return __builtin_popcount(x);
        #endif
    #endif
    x = (x & 0x55555555) + ((x & 0xAAAAAAAA) >> 1);
    x = (x & 0x33333333) + ((x & 0xCCCCCCCC) >> 2);
    x = (x & 0x0F0F0F0F) + ((x & 0xF0F0F0F0) >> 4);
    x = (x & 0x00FF00FF) + ((x & 0xFF00FF00) >> 8);
    x = (x & 0x0000FFFF) + ((x & 0xFFFF0000) >> 16);
    return (int)x;
}

static ALWAYS_INLINE CONST_FUNCTION int popcnt64(uint64_t x)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if IS_GCC(3,4) || CLANG_HAS_BUILTIN(__builtin_popcountll)
            return __builtin_choose_expr(
                sizeof(long) == 8,
                __builtin_popcountl(x),
                __extension__({
                    STATIC_ASSERT(sizeof(long long) == 8,
                                  "Non-64-bit long long is not supported");
                    __builtin_popcountll(x);
                }));
            return __builtin_popcount(x);
        #endif
    #endif
    x = (x & UINT64_C(0x5555555555555555)) + ((x & UINT64_C(0xAAAAAAAAAAAAAAAA)) >> 1);
    x = (x & UINT64_C(0x3333333333333333)) + ((x & UINT64_C(0xCCCCCCCCCCCCCCCC)) >> 2);
    x = (x & UINT64_C(0x0F0F0F0F0F0F0F0F)) + ((x & UINT64_C(0xF0F0F0F0F0F0F0F0)) >> 4);
    x = (x & UINT64_C(0x00FF00FF00FF00FF)) + ((x & UINT64_C(0xFF00FF00FF00FF00)) >> 8);
    x = (x & UINT64_C(0x0000FFFF0000FFFF)) + ((x & UINT64_C(0xFFFF0000FFFF0000)) >> 16);
    x = (x & UINT64_C(0x00000000FFFFFFFF)) + ((x & UINT64_C(0xFFFFFFFF00000000)) >> 32);
    return (int)x;
}

/*-----------------------------------------------------------------------*/

/**
 * ror32, ror64:  Rotate the given 32-bit or 64-bit value "count" bits to
 * the right.
 */
static ALWAYS_INLINE CONST_FUNCTION uint32_t ror32(uint32_t x, int count)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if defined(__GNUC__)
            /* GCC and Clang lack rotate intrinsics and sometimes have
             * trouble detecting the usual rotate idiom, so use raw
             * assembly to implement the operation. */
            #if defined(__amd64__) || defined(__x86_64__)
                uint32_t result;
                __asm__("rorl %2,%0"
                        : "=r" (result) : "0" (x), "c" ((uint8_t)count));
                return result;
            #elif defined(__mips__)
                uint32_t result;
                __asm__("ror %0,%1,%2" : "=r" (result) : "r" (x), "r" (count));
                return result;
            #endif
        #elif defined(_MSC_VER)
            return _rotr(x, count);
        #endif
    #endif
    return x >> (count & 31) | x << (-count & 31);
}

static ALWAYS_INLINE CONST_FUNCTION uint64_t ror64(uint64_t x, int count)
{
    #ifndef SUPPRESS_BITUTILS_INTRINSICS
        #if defined(__GNUC__)
            #if defined(__amd64__) || defined(__x86_64__)
                uint64_t result;
                __asm__("rorq %2,%0"
                        : "=r" (result) : "0" (x), "c" ((uint8_t)count));
                return result;
            #endif
        #elif defined(_MSC_VER)
            return _rotr64(x, count);
        #endif
    #endif
    return x >> (count & 63) | x << (-count & 63);
}

/*-----------------------------------------------------------------------*/

/**
 * float_to_bits, double_to_bits:  Return the bit pattern corresponding to
 * the given floating-point value.
 */
static ALWAYS_INLINE CONST_FUNCTION uint32_t float_to_bits(float x)
{
    union {uint32_t i; float f;} u = {.f = x};
    return u.i;
}

static ALWAYS_INLINE CONST_FUNCTION uint64_t double_to_bits(double x)
{
    union {uint64_t i; double f;} u = {.f = x};
    return u.i;
}

/*-----------------------------------------------------------------------*/

/**
 * bits_to_float, bits_to_double:  Return the floating-point value
 * corresponding to the given bit pattern.
 */
static ALWAYS_INLINE CONST_FUNCTION float bits_to_float(uint32_t x)
{
    union {uint32_t i; float f;} u = {.i = x};
    return u.f;
}

static ALWAYS_INLINE CONST_FUNCTION double bits_to_double(uint64_t x)
{
    union {uint64_t i; double f;} u = {.i = x};
    return u.f;
}

/*************************************************************************/
/*************************************************************************/

#endif  // SRC_BITUTILS_H
