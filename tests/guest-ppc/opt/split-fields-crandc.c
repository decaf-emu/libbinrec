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
    0x4F,0xD5,0x61,0x02,  // crandc 30,21,12
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BFEXT      r6, r5, 10, 1\n"
    "    7: BFEXT      r7, r5, 19, 1\n"
    "    8: XORI       r8, r7, 1\n"
    "    9: AND        r9, r6, r8\n"
    "   10: SET_ALIAS  a3, r9\n"
    "   11: LOAD_IMM   r10, 4\n"
    "   12: SET_ALIAS  a1, r10\n"
    "   13: GET_ALIAS  r11, a2\n"
    "   14: ANDI       r12, r11, -3\n"
    "   15: GET_ALIAS  r13, a3\n"
    "   16: SLLI       r14, r13, 1\n"
    "   17: OR         r15, r12, r14\n"
    "   18: SET_ALIAS  a2, r15\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
