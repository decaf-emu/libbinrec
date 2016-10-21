/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"
#include "src/rtl-internal.h"

static const uint8_t input[] = {
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 3\n"
        "[info] r3 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: NOP\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a2, r4\n"
    "    6: LOAD_IMM   r5, 8\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: GET_ALIAS  r6, a3\n"
    "    9: ANDI       r7, r6, -3\n"
    "   10: GET_ALIAS  r8, a2\n"
    "   11: SLLI       r9, r8, 1\n"
    "   12: OR         r10, r7, r9\n"
    "   13: SET_ALIAS  a3, r10\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
