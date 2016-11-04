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
    0xFF,0x81,0x10,0x00,  // fcmp cr7,f1,f2
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

// FIXME: will need to be updated for FPSCR writes
static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: FCMP       r5, r3, r4, LT\n"
    "    5: FCMP       r6, r3, r4, GT\n"
    "    6: FCMP       r7, r3, r4, EQ\n"
    "    7: FGETSTATE  r8\n"
    "    8: FTESTEXC   r9, r8, INVALID\n"
    "    9: SET_ALIAS  a4, r5\n"
    "   10: SET_ALIAS  a5, r6\n"
    "   11: SET_ALIAS  a6, r7\n"
    "   12: SET_ALIAS  a7, r9\n"
    "   13: LOAD_IMM   r10, 4\n"
    "   14: SET_ALIAS  a1, r10\n"
    "   15: GET_ALIAS  r11, a8\n"
    "   16: ANDI       r12, r11, -16\n"
    "   17: GET_ALIAS  r13, a4\n"
    "   18: SLLI       r14, r13, 3\n"
    "   19: OR         r15, r12, r14\n"
    "   20: GET_ALIAS  r16, a5\n"
    "   21: SLLI       r17, r16, 2\n"
    "   22: OR         r18, r15, r17\n"
    "   23: GET_ALIAS  r19, a6\n"
    "   24: SLLI       r20, r19, 1\n"
    "   25: OR         r21, r18, r20\n"
    "   26: GET_ALIAS  r22, a7\n"
    "   27: OR         r23, r21, r22\n"
    "   28: SET_ALIAS  a8, r23\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
