/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

static const uint8_t input[] = {
    0x7C,0x62,0x2D,0xAA,  // stswi r3,r2,5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: ZCAST      r4, r3\n"
    "    5: ADD        r5, r2, r4\n"
    "    6: LOAD_U8    r6, 271(r1)\n"
    "    7: LOAD_U8    r7, 270(r1)\n"
    "    8: LOAD_U8    r8, 269(r1)\n"
    "    9: LOAD_U8    r9, 268(r1)\n"
    "   10: STORE_I8   0(r5), r6\n"
    "   11: STORE_I8   1(r5), r7\n"
    "   12: STORE_I8   2(r5), r8\n"
    "   13: STORE_I8   3(r5), r9\n"
    "   14: LOAD_U8    r10, 275(r1)\n"
    "   15: STORE_I8   4(r5), r10\n"
    "   16: LOAD_IMM   r11, 4\n"
    "   17: SET_ALIAS  a1, r11\n"
    "   18: LABEL      L2\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 264(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,17] --> 2\n"
    "Block 2: 1 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"