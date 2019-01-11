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
    0xFC,0x00,0x01,0x0D,  // mtfsfi. 0,0
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ANDI       r4, r3, 268435455\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: ANDI       r5, r4, 33031936\n"
    "    6: SGTUI      r6, r5, 0\n"
    "    7: BFEXT      r7, r4, 25, 4\n"
    "    8: SLLI       r8, r6, 4\n"
    "    9: SRLI       r9, r4, 3\n"
    "   10: OR         r10, r7, r8\n"
    "   11: AND        r11, r10, r9\n"
    "   12: ANDI       r12, r11, 31\n"
    "   13: SGTUI      r13, r12, 0\n"
    "   14: BFEXT      r14, r4, 31, 1\n"
    "   15: BFEXT      r15, r4, 28, 1\n"
    "   16: GET_ALIAS  r16, a2\n"
    "   17: SLLI       r17, r14, 3\n"
    "   18: SLLI       r18, r13, 2\n"
    "   19: SLLI       r19, r6, 1\n"
    "   20: OR         r20, r17, r18\n"
    "   21: OR         r21, r19, r15\n"
    "   22: OR         r22, r20, r21\n"
    "   23: BFINS      r23, r16, r22, 24, 4\n"
    "   24: SET_ALIAS  a2, r23\n"
    "   25: LOAD_IMM   r24, 4\n"
    "   26: SET_ALIAS  a1, r24\n"
    "   27: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
