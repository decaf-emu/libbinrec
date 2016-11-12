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
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 6\n"
        "[info] r5 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: LOAD_IMM   r5, 1\n"
    "    6: NOP\n"
    "    7: LOAD_IMM   r6, 0\n"
    "    8: SET_ALIAS  a3, r6\n"
    "    9: LOAD_IMM   r7, 8\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: GET_ALIAS  r8, a2\n"
    "   12: ANDI       r9, r8, -3\n"
    "   13: GET_ALIAS  r10, a3\n"
    "   14: SLLI       r11, r10, 1\n"
    "   15: OR         r12, r9, r11\n"
    "   16: SET_ALIAS  a2, r12\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
