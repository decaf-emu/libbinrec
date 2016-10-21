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
    0x7C,0x83,0x2E,0x70,  // srawi r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: SRAI       r4, r3, 5\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: ANDI       r5, r3, 31\n"
    "    6: SGTUI      r6, r5, 0\n"
    "    7: SLTSI      r7, r3, 0\n"
    "    8: AND        r8, r6, r7\n"
    "    9: GET_ALIAS  r9, a4\n"
    "   10: BFINS      r10, r9, r8, 29, 1\n"
    "   11: SET_ALIAS  a4, r10\n"
    "   12: LOAD_IMM   r11, 4\n"
    "   13: SET_ALIAS  a1, r11\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
