/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/endian.h"
#include "tests/common.h"


int main(int argc, char **argv)
{
    static const union {uint8_t b[4]; uint32_t i;} value = {
        .b = {0x12, 0x34, 0x56, 0x78}};
    volatile uint32_t test = value.i;
    EXPECT_EQ(bswap_le32(test), 0x78563412);
    return EXIT_SUCCESS;
}
