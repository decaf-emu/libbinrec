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
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ANDI       r4, r3, 33031936\n"
    "    4: SGTUI      r5, r4, 0\n"
    "    5: BFEXT      r6, r3, 25, 4\n"
    "    6: SLLI       r7, r5, 4\n"
    "    7: SRLI       r8, r3, 3\n"
    "    8: OR         r9, r6, r7\n"
    "    9: AND        r10, r9, r8\n"
    "   10: ANDI       r11, r10, 31\n"
    "   11: SGTUI      r12, r11, 0\n"
    "   12: SLLI       r13, r12, 30\n"
    "   13: SLLI       r14, r5, 29\n"
    "   14: OR         r15, r13, r14\n"
    "   15: OR         r16, r3, r15\n"
    "   16: ZCAST      r17, r16\n"
    "   17: BITCAST    r18, r17\n"
    "   18: SET_ALIAS  a2, r18\n"
    "   19: LOAD_IMM   r19, 4\n"
    "   20: SET_ALIAS  a1, r19\n"
    "   21: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
