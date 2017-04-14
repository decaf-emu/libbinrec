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
    0x7C,0x64,0x2D,0x10,  // subfeo r3,r4,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 19\n"
        "[info] r1 death rolled back to 9\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: NOT        r4, r3\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: ADD        r6, r4, r5\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: BFEXT      r8, r7, 29, 1\n"
    "    8: ADD        r9, r6, r8\n"
    "    9: SET_ALIAS  a2, r9\n"
    "   10: SRLI       r10, r4, 31\n"
    "   11: SRLI       r11, r5, 31\n"
    "   12: XOR        r12, r10, r11\n"
    "   13: SRLI       r13, r9, 31\n"
    "   14: XORI       r14, r13, 1\n"
    "   15: AND        r15, r10, r11\n"
    "   16: AND        r16, r12, r14\n"
    "   17: OR         r17, r15, r16\n"
    "   18: BFINS      r18, r7, r17, 29, 1\n"
    "   19: NOP\n"
    "   20: XORI       r19, r12, 1\n"
    "   21: XOR        r20, r10, r13\n"
    "   22: AND        r21, r19, r20\n"
    "   23: ANDI       r22, r18, -1073741825\n"
    "   24: LOAD_IMM   r23, 0xC0000000\n"
    "   25: SELECT     r24, r23, r21, r21\n"
    "   26: OR         r25, r22, r24\n"
    "   27: SET_ALIAS  a5, r25\n"
    "   28: LOAD_IMM   r26, 4\n"
    "   29: SET_ALIAS  a1, r26\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
