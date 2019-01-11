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
    0xFC,0x20,0x0D,0x8E,  // mtfsf 16,f1
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: GET_ALIAS  r8, a3\n"
    "    9: ANDI       r9, r7, 524288\n"
    "   10: ANDI       r10, r8, -524289\n"
    "   11: OR         r11, r10, r9\n"
    "   12: SET_ALIAS  a3, r11\n"
    "   13: GET_ALIAS  r12, a4\n"
    "   14: SRLI       r13, r7, 12\n"
    "   15: ANDI       r14, r13, 112\n"
    "   16: ANDI       r15, r12, 15\n"
    "   17: OR         r16, r15, r14\n"
    "   18: SET_ALIAS  a4, r16\n"
    "   19: LOAD_IMM   r17, 4\n"
    "   20: SET_ALIAS  a1, r17\n"
    "   21: GET_ALIAS  r18, a3\n"
    "   22: GET_ALIAS  r19, a4\n"
    "   23: ANDI       r20, r18, -1611134977\n"
    "   24: SLLI       r21, r19, 12\n"
    "   25: OR         r22, r20, r21\n"
    "   26: SET_ALIAS  a3, r22\n"
    "   27: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
