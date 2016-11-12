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
    0xFD,0x80,0x01,0x0C,  // mtfsfi 3,0
    0x0E,0x03,0x00,0x00,  // twlti r3,0
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a3\n"
    "    6: ANDI       r6, r5, -524289\n"
    "    7: SET_ALIAS  a3, r6\n"
    "    8: GET_ALIAS  r7, a4\n"
    "    9: ANDI       r8, r7, 15\n"
    "   10: SET_ALIAS  a4, r8\n"
    "   11: LOAD_IMM   r9, 0\n"
    "   12: GET_ALIAS  r10, a2\n"
    "   13: SLTS       r11, r10, r9\n"
    "   14: GOTO_IF_Z  r11, L1\n"
    "   15: ANDI       r12, r6, -1611134977\n"
    "   16: SLLI       r13, r8, 12\n"
    "   17: OR         r14, r12, r13\n"
    "   18: SET_ALIAS  a3, r14\n"
    "   19: LOAD_IMM   r15, 4\n"
    "   20: SET_ALIAS  a1, r15\n"
    "   21: LOAD       r16, 984(r1)\n"
    "   22: CALL       @r16, r1\n"
    "   23: RETURN\n"
    "   24: LABEL      L1\n"
    "   25: LOAD_IMM   r17, 8\n"
    "   26: SET_ALIAS  a1, r17\n"
    "   27: GET_ALIAS  r18, a3\n"
    "   28: GET_ALIAS  r19, a4\n"
    "   29: ANDI       r20, r18, -1611134977\n"
    "   30: SLLI       r21, r19, 12\n"
    "   31: OR         r22, r20, r21\n"
    "   32: SET_ALIAS  a3, r22\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,14] --> 1,2\n"
    "Block 1: 0 --> [15,23] --> <none>\n"
    "Block 2: 0 --> [24,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
