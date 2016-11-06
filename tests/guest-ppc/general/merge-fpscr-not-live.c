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
    0x0E,0x03,0x00,0x00,  // twlti r3,0
    0xFC,0x00,0x00,0x4C,  // mtfsb1 0
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
    "    5: LOAD_IMM   r5, 0\n"
    "    6: GET_ALIAS  r6, a2\n"
    "    7: SLTS       r7, r6, r5\n"
    "    8: GOTO_IF_Z  r7, L1\n"
    "    9: GET_ALIAS  r8, a3\n"
    "   10: GET_ALIAS  r9, a4\n"
    "   11: ANDI       r10, r8, -1611134977\n"
    "   12: SLLI       r11, r9, 12\n"
    "   13: OR         r12, r10, r11\n"
    "   14: SET_ALIAS  a3, r12\n"
    "   15: LOAD_IMM   r13, 0\n"
    "   16: SET_ALIAS  a1, r13\n"
    "   17: LOAD       r14, 984(r1)\n"
    "   18: CALL       @r14, r1\n"
    "   19: RETURN\n"
    "   20: LABEL      L1\n"
    "   21: LOAD_IMM   r15, 4\n"
    "   22: SET_ALIAS  a1, r15\n"
    "   23: GET_ALIAS  r16, a3\n"
    "   24: ORI        r17, r16, -2147483648\n"
    "   25: SET_ALIAS  a3, r17\n"
    "   26: LOAD_IMM   r18, 8\n"
    "   27: SET_ALIAS  a1, r18\n"
    "   28: GET_ALIAS  r19, a3\n"
    "   29: GET_ALIAS  r20, a4\n"
    "   30: ANDI       r21, r19, -1611134977\n"
    "   31: SLLI       r22, r20, 12\n"
    "   32: OR         r23, r21, r22\n"
    "   33: SET_ALIAS  a3, r23\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,2\n"
    "Block 1: 0 --> [9,19] --> <none>\n"
    "Block 2: 0 --> [20,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
