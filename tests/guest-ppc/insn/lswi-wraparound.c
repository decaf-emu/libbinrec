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
    0x7F,0xE2,0x44,0xAA,  // lswi r31,r2,8
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_U8    r6, 0(r5)\n"
    "    6: LOAD_U8    r7, 1(r5)\n"
    "    7: LOAD_U8    r8, 2(r5)\n"
    "    8: LOAD_U8    r9, 3(r5)\n"
    "    9: STORE_I8   383(r1), r6\n"
    "   10: STORE_I8   382(r1), r7\n"
    "   11: STORE_I8   381(r1), r8\n"
    "   12: STORE_I8   380(r1), r9\n"
    "   13: LOAD_U8    r10, 4(r5)\n"
    "   14: LOAD_U8    r11, 5(r5)\n"
    "   15: LOAD_U8    r12, 6(r5)\n"
    "   16: LOAD_U8    r13, 7(r5)\n"
    "   17: STORE_I8   259(r1), r10\n"
    "   18: STORE_I8   258(r1), r11\n"
    "   19: STORE_I8   257(r1), r12\n"
    "   20: STORE_I8   256(r1), r13\n"
    "   21: LOAD_IMM   r14, 4\n"
    "   22: SET_ALIAS  a1, r14\n"
    "   23: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 264(r1)\n"
    "Alias 4: int32 @ 380(r1)\n"
    "\n"
    "Block 0: <none> --> [0,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
