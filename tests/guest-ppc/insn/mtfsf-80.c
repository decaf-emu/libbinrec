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
    0xFD,0x00,0x0D,0x8E,  // mtfsf 128,f1
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
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: GET_ALIAS  r8, a3\n"
    "    9: ANDI       r9, r7, -1879048192\n"
    "   10: ANDI       r10, r8, 805306367\n"
    "   11: OR         r11, r10, r9\n"
    "   12: SRLI       r12, r11, 25\n"
    "   13: SRLI       r13, r11, 3\n"
    "   14: AND        r14, r12, r13\n"
    "   15: ANDI       r15, r14, 31\n"
    "   16: SGTUI      r16, r15, 0\n"
    "   17: SLLI       r17, r16, 30\n"
    "   18: OR         r18, r11, r17\n"
    "   19: SET_ALIAS  a3, r18\n"
    "   20: LOAD_IMM   r19, 4\n"
    "   21: SET_ALIAS  a1, r19\n"
    "   22: GET_ALIAS  r20, a3\n"
    "   23: GET_ALIAS  r21, a4\n"
    "   24: ANDI       r22, r20, -522241\n"
    "   25: SLLI       r23, r21, 12\n"
    "   26: OR         r24, r22, r23\n"
    "   27: SET_ALIAS  a3, r24\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
