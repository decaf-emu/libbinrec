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
    0xC0,0x23,0x00,0x00,  // lfs f1,0(r3)
    0xC8,0x23,0x00,0x08,  // lfd f1,8(r3)
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, 0(r5)\n"
    "    6: ZCAST      r7, r3\n"
    "    7: ADD        r8, r2, r7\n"
    "    8: LOAD_BR    r9, 8(r8)\n"
    "    9: FCMP       r10, r6, r6, UN\n"
    "   10: FCVT       r11, r6\n"
    "   11: SET_ALIAS  a4, r11\n"
    "   12: GOTO_IF_Z  r10, L1\n"
    "   13: BITCAST    r12, r6\n"
    "   14: ANDI       r13, r12, 4194304\n"
    "   15: GOTO_IF_NZ r13, L1\n"
    "   16: BITCAST    r14, r11\n"
    "   17: LOAD_IMM   r15, 0x8000000000000\n"
    "   18: XOR        r16, r14, r15\n"
    "   19: BITCAST    r17, r16\n"
    "   20: SET_ALIAS  a4, r17\n"
    "   21: LABEL      L1\n"
    "   22: GET_ALIAS  r18, a4\n"
    "   23: STORE      408(r1), r18\n"
    "   24: SET_ALIAS  a3, r9\n"
    "   25: LOAD_IMM   r19, 8\n"
    "   26: SET_ALIAS  a1, r19\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64 @ 400(r1)\n"
    "Alias 4: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,3\n"
    "Block 1: 0 --> [13,15] --> 2,3\n"
    "Block 2: 1 --> [16,20] --> 3\n"
    "Block 3: 2,0,1 --> [21,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
