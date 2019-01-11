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
    0xC0,0x23,0xFF,0xF0,  // lfs f1,-16(r3)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, -16(r5)\n"
    "    6: FGETSTATE  r7\n"
    "    7: FCMP       r8, r6, r6, UN\n"
    "    8: FCVT       r9, r6\n"
    "    9: SET_ALIAS  a4, r9\n"
    "   10: GOTO_IF_Z  r8, L1\n"
    "   11: BITCAST    r10, r6\n"
    "   12: ANDI       r11, r10, 4194304\n"
    "   13: GOTO_IF_NZ r11, L1\n"
    "   14: BITCAST    r12, r9\n"
    "   15: LOAD_IMM   r13, 0x8000000000000\n"
    "   16: XOR        r14, r12, r13\n"
    "   17: BITCAST    r15, r14\n"
    "   18: SET_ALIAS  a4, r15\n"
    "   19: LABEL      L1\n"
    "   20: GET_ALIAS  r16, a4\n"
    "   21: FSETSTATE  r7\n"
    "   22: STORE      408(r1), r16\n"
    "   23: SET_ALIAS  a3, r16\n"
    "   24: LOAD_IMM   r17, 4\n"
    "   25: SET_ALIAS  a1, r17\n"
    "   26: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64 @ 400(r1)\n"
    "Alias 4: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,3\n"
    "Block 1: 0 --> [11,13] --> 2,3\n"
    "Block 2: 1 --> [14,18] --> 3\n"
    "Block 3: 2,0,1 --> [19,26] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
