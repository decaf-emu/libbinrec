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
    0x41,0x82,0x00,0x08,  // beq 0x8
    0xFD,0xF8,0x0D,0x8E,  // mtfsf 252,f1
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: GET_ALIAS  r5, a3\n"
    "    6: ANDI       r6, r5, 536870912\n"
    "    7: GOTO_IF_NZ r6, L1\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: GET_ALIAS  r8, a2\n"
    "   11: BITCAST    r9, r8\n"
    "   12: ZCAST      r10, r9\n"
    "   13: GET_ALIAS  r11, a4\n"
    "   14: ANDI       r12, r10, -1611135232\n"
    "   15: ANDI       r13, r11, 1611135231\n"
    "   16: OR         r14, r13, r12\n"
    "   17: SET_ALIAS  a4, r14\n"
    "   18: BFEXT      r15, r10, 12, 7\n"
    "   19: SET_ALIAS  a5, r15\n"
    "   20: LOAD_IMM   r16, 8\n"
    "   21: SET_ALIAS  a1, r16\n"
    "   22: LABEL      L1\n"
    "   23: LOAD_IMM   r17, 12\n"
    "   24: SET_ALIAS  a1, r17\n"
    "   25: GET_ALIAS  r18, a4\n"
    "   26: GET_ALIAS  r19, a5\n"
    "   27: ANDI       r20, r18, -1611134977\n"
    "   28: SLLI       r21, r19, 12\n"
    "   29: OR         r22, r20, r21\n"
    "   30: SET_ALIAS  a4, r22\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,21] --> 2\n"
    "Block 2: 1,0 --> [22,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
