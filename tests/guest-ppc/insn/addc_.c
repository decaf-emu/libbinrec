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
    0x7C,0x64,0x28,0x15,  // addc. r3,r4,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: SRLI       r6, r3, 31\n"
    "    7: SRLI       r7, r4, 31\n"
    "    8: XOR        r8, r6, r7\n"
    "    9: SRLI       r9, r5, 31\n"
    "   10: XORI       r10, r9, 1\n"
    "   11: AND        r11, r6, r7\n"
    "   12: AND        r12, r8, r10\n"
    "   13: OR         r13, r11, r12\n"
    "   14: GET_ALIAS  r14, a6\n"
    "   15: BFINS      r15, r14, r13, 29, 1\n"
    "   16: SET_ALIAS  a6, r15\n"
    "   17: SLTSI      r16, r5, 0\n"
    "   18: SGTSI      r17, r5, 0\n"
    "   19: SEQI       r18, r5, 0\n"
    "   20: BFEXT      r19, r15, 31, 1\n"
    "   21: GET_ALIAS  r20, a5\n"
    "   22: SLLI       r21, r16, 3\n"
    "   23: SLLI       r22, r17, 2\n"
    "   24: SLLI       r23, r18, 1\n"
    "   25: OR         r24, r21, r22\n"
    "   26: OR         r25, r23, r19\n"
    "   27: OR         r26, r24, r25\n"
    "   28: BFINS      r27, r20, r26, 28, 4\n"
    "   29: SET_ALIAS  a5, r27\n"
    "   30: LOAD_IMM   r28, 4\n"
    "   31: SET_ALIAS  a1, r28\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
