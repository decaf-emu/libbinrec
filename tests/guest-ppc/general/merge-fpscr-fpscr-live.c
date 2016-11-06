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
    0xFC,0x00,0x00,0x4C,  // mtfsb1 0
    0x0E,0x03,0x00,0x00,  // twlti r3,0
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a3\n"
    "    6: ORI        r6, r5, -2147483648\n"
    "    7: SET_ALIAS  a3, r6\n"
    "    8: LOAD_IMM   r7, 0\n"
    "    9: GET_ALIAS  r8, a2\n"
    "   10: SLTS       r9, r8, r7\n"
    "   11: GOTO_IF_Z  r9, L1\n"
    "   12: GET_ALIAS  r10, a4\n"
    "   13: ANDI       r11, r6, -1611134977\n"
    "   14: SLLI       r12, r10, 12\n"
    "   15: OR         r13, r11, r12\n"
    "   16: SET_ALIAS  a3, r13\n"
    "   17: LOAD_IMM   r14, 4\n"
    "   18: SET_ALIAS  a1, r14\n"
    "   19: LOAD       r15, 984(r1)\n"
    "   20: CALL       @r15, r1\n"
    "   21: RETURN\n"
    "   22: LABEL      L1\n"
    "   23: LOAD_IMM   r16, 8\n"
    "   24: SET_ALIAS  a1, r16\n"
    "   25: GET_ALIAS  r17, a3\n"
    "   26: GET_ALIAS  r18, a4\n"
    "   27: ANDI       r19, r17, -1611134977\n"
    "   28: SLLI       r20, r18, 12\n"
    "   29: OR         r21, r19, r20\n"
    "   30: SET_ALIAS  a3, r21\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,21] --> <none>\n"
    "Block 2: 0 --> [22,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
