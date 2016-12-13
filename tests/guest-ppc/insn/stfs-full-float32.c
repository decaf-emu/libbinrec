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
    0xD0,0x23,0x00,0x10,  // stfs f1,16(r3)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, -16(r5)\n"
    "    6: ZCAST      r7, r3\n"
    "    7: ADD        r8, r2, r7\n"
    "    8: STORE_BR   16(r8), r6\n"
    "    9: FGETSTATE  r9\n"
    "   10: FCMP       r10, r6, r6, UN\n"
    "   11: FCVT       r11, r6\n"
    "   12: SET_ALIAS  a4, r11\n"
    "   13: GOTO_IF_Z  r10, L1\n"
    "   14: BITCAST    r12, r6\n"
    "   15: ANDI       r13, r12, 4194304\n"
    "   16: GOTO_IF_NZ r13, L1\n"
    "   17: BITCAST    r14, r11\n"
    "   18: LOAD_IMM   r15, 0x8000000000000\n"
    "   19: XOR        r16, r14, r15\n"
    "   20: BITCAST    r17, r16\n"
    "   21: SET_ALIAS  a4, r17\n"
    "   22: LABEL      L1\n"
    "   23: GET_ALIAS  r18, a4\n"
    "   24: FSETSTATE  r9\n"
    "   25: STORE      408(r1), r18\n"
    "   26: SET_ALIAS  a3, r18\n"
    "   27: LOAD_IMM   r19, 8\n"
    "   28: SET_ALIAS  a1, r19\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64 @ 400(r1)\n"
    "Alias 4: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,13] --> 1,3\n"
    "Block 1: 0 --> [14,16] --> 2,3\n"
    "Block 2: 1 --> [17,21] --> 3\n"
    "Block 3: 2,0,1 --> [22,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
