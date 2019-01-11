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
    0x7C,0x64,0x2C,0x14,  // addco r3,r4,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 16\n"
        "[info] r1 death rolled back to 14\n"
    #endif
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
    "   14: GET_ALIAS  r14, a5\n"
    "   15: BFINS      r15, r14, r13, 29, 1\n"
    "   16: NOP\n"
    "   17: XORI       r16, r8, 1\n"
    "   18: XOR        r17, r6, r9\n"
    "   19: AND        r18, r16, r17\n"
    "   20: ANDI       r19, r15, -1073741825\n"
    "   21: LOAD_IMM   r20, 0xC0000000\n"
    "   22: SELECT     r21, r20, r18, r18\n"
    "   23: OR         r22, r19, r21\n"
    "   24: SET_ALIAS  a5, r22\n"
    "   25: LOAD_IMM   r23, 4\n"
    "   26: SET_ALIAS  a1, r23\n"
    "   27: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
