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
    0x30,0x60,0x12,0x34,  // addic r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: ADDI       r3, r2, 4660\n"
    "    4: SET_ALIAS  a3, r3\n"
    "    5: GET_ALIAS  r4, a4\n"
    "    6: SLTUI      r5, r3, 4660\n"
    "    7: BFINS      r6, r4, r5, 29, 1\n"
    "    8: SET_ALIAS  a4, r6\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: LABEL      L2\n"
    "   12: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,10] --> 2\n"
    "Block 2: 1 --> [11,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
