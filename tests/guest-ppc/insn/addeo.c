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
    0x7C,0x64,0x2D,0x14,  // addeo r3,r4,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 18\n"
        "[info] r1 death rolled back to 8\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: GET_ALIAS  r6, a5\n"
    "    6: BFEXT      r7, r6, 29, 1\n"
    "    7: ADD        r8, r5, r7\n"
    "    8: SET_ALIAS  a2, r8\n"
    "    9: SRLI       r9, r3, 31\n"
    "   10: SRLI       r10, r4, 31\n"
    "   11: XOR        r11, r9, r10\n"
    "   12: SRLI       r12, r8, 31\n"
    "   13: XORI       r13, r12, 1\n"
    "   14: AND        r14, r9, r10\n"
    "   15: AND        r15, r11, r13\n"
    "   16: OR         r16, r14, r15\n"
    "   17: BFINS      r17, r6, r16, 29, 1\n"
    "   18: NOP\n"
    "   19: XORI       r18, r11, 1\n"
    "   20: XOR        r19, r9, r12\n"
    "   21: AND        r20, r18, r19\n"
    "   22: ANDI       r21, r17, -1073741825\n"
    "   23: LOAD_IMM   r22, 0xC0000000\n"
    "   24: SELECT     r23, r22, r20, r20\n"
    "   25: OR         r24, r21, r23\n"
    "   26: SET_ALIAS  a5, r24\n"
    "   27: LOAD_IMM   r25, 4\n"
    "   28: SET_ALIAS  a1, r25\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
