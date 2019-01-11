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
    0x7C,0x64,0x2D,0xD6,  // mullwo r3,r4,r5
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
    "    4: MUL        r5, r3, r4\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: GET_ALIAS  r6, a5\n"
    "    7: ANDI       r7, r6, -1073741825\n"
    "    8: MULHS      r8, r3, r4\n"
    "    9: SRAI       r9, r5, 31\n"
    "   10: XOR        r10, r8, r9\n"
    "   11: LOAD_IMM   r11, 0xC0000000\n"
    "   12: SELECT     r12, r11, r10, r10\n"
    "   13: OR         r13, r7, r12\n"
    "   14: SET_ALIAS  a5, r13\n"
    "   15: LOAD_IMM   r14, 4\n"
    "   16: SET_ALIAS  a1, r14\n"
    "   17: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
