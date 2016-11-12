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
    0x7C,0x60,0x24,0xAA,  // lswi r3,0,4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_U8    r3, 0(r2)\n"
    "    3: LOAD_U8    r4, 1(r2)\n"
    "    4: LOAD_U8    r5, 2(r2)\n"
    "    5: LOAD_U8    r6, 3(r2)\n"
    "    6: STORE_I8   271(r1), r3\n"
    "    7: STORE_I8   270(r1), r4\n"
    "    8: STORE_I8   269(r1), r5\n"
    "    9: STORE_I8   268(r1), r6\n"
    "   10: LOAD_IMM   r7, 4\n"
    "   11: SET_ALIAS  a1, r7\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
