/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_GUEST_PPC_EXEC_COMMON_H
#define TESTS_GUEST_PPC_EXEC_COMMON_H

#include "src/endian.h"

static inline void memcpy_be32(void *dest, const uint32_t *src, size_t len)
{
    for (size_t i = 0; i+3 < len; i += 4) {
        ((uint32_t *)dest)[i/4] = bswap_be32(src[i/4]);
    }
}

#endif  // TESTS_GUEST_PPC_EXEC_COMMON_H
