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
    0xFC,0x20,0x04,0x8E,  // mffs f1
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a3\n"
    "    6: GET_ALIAS  r6, a4\n"
    "    7: ANDI       r7, r5, -522241\n"
    "    8: SLLI       r8, r6, 12\n"
    "    9: OR         r9, r7, r8\n"
    "   10: ZCAST      r10, r9\n"
    "   11: BITCAST    r11, r10\n"
    "   12: SET_ALIAS  a2, r11\n"
    "   13: LOAD_IMM   r12, 4\n"
    "   14: SET_ALIAS  a1, r12\n"
    "   15: GET_ALIAS  r13, a3\n"
    "   16: GET_ALIAS  r14, a4\n"
    "   17: ANDI       r15, r13, -522241\n"
    "   18: SLLI       r16, r14, 12\n"
    "   19: OR         r17, r15, r16\n"
    "   20: SET_ALIAS  a3, r17\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
