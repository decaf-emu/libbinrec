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
    0x7C,0x64,0x05,0x94,  // addzeo r3,r4
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 12\n"
        "[info] r1 death rolled back to 6\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: BFEXT      r5, r4, 29, 1\n"
    "    5: ADD        r6, r3, r5\n"
    "    6: SET_ALIAS  a2, r6\n"
    "    7: SRLI       r7, r3, 31\n"
    "    8: SRLI       r8, r6, 31\n"
    "    9: XORI       r9, r8, 1\n"
    "   10: AND        r10, r7, r9\n"
    "   11: BFINS      r11, r4, r10, 29, 1\n"
    "   12: NOP\n"
    "   13: XORI       r12, r7, 1\n"
    "   14: AND        r13, r12, r8\n"
    "   15: ANDI       r14, r11, -1073741825\n"
    "   16: LOAD_IMM   r15, 0xC0000000\n"
    "   17: SELECT     r16, r15, r13, r13\n"
    "   18: OR         r17, r14, r16\n"
    "   19: SET_ALIAS  a4, r17\n"
    "   20: LOAD_IMM   r18, 4\n"
    "   21: SET_ALIAS  a1, r18\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
