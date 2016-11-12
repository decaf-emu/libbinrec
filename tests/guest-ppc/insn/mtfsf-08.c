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
    0xFC,0x10,0x0D,0x8E,  // mtfsf 8,f1
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: GET_ALIAS  r8, a4\n"
    "    9: SRLI       r9, r7, 12\n"
    "   10: ANDI       r10, r9, 15\n"
    "   11: ANDI       r11, r8, 112\n"
    "   12: OR         r12, r11, r10\n"
    "   13: SET_ALIAS  a4, r12\n"
    "   14: LOAD_IMM   r13, 4\n"
    "   15: SET_ALIAS  a1, r13\n"
    "   16: GET_ALIAS  r14, a3\n"
    "   17: GET_ALIAS  r15, a4\n"
    "   18: ANDI       r16, r14, -1611134977\n"
    "   19: SLLI       r17, r15, 12\n"
    "   20: OR         r18, r16, r17\n"
    "   21: SET_ALIAS  a3, r18\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
