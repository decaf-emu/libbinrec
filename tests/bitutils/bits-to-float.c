/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/bitutils.h"
#include "tests/common.h"
#include <math.h>


int main(int argc, char **argv)
{
    EXPECT(bits_to_float(0) == 0.0f);
    EXPECT(bits_to_float(0x3F800000) == 1.0f);
    EXPECT(bits_to_float(0xBF800000) == -1.0f);
    EXPECT(fpclassify(bits_to_float(0x7F800000)) == FP_INFINITE);
    EXPECT(fpclassify(bits_to_float(0x7F800001)) == FP_NAN);

    return EXIT_SUCCESS;
}
