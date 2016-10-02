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
    0xBB,0x80,0x00,0x10,  // lmw r28,16(0)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_BR    r3, 16(r2)\n"
    "    4: LOAD_BR    r4, 20(r2)\n"
    "    5: LOAD_BR    r5, 24(r2)\n"
    "    6: LOAD_BR    r6, 28(r2)\n"
    "    7: SET_ALIAS  a2, r3\n"
    "    8: SET_ALIAS  a3, r4\n"
    "    9: SET_ALIAS  a4, r5\n"
    "   10: SET_ALIAS  a5, r6\n"
    "   11: LOAD_IMM   r7, 4\n"
    "   12: SET_ALIAS  a1, r7\n"
    "   13: LABEL      L2\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 368(r1)\n"
    "Alias 3: int32 @ 372(r1)\n"
    "Alias 4: int32 @ 376(r1)\n"
    "Alias 5: int32 @ 380(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,12] --> 2\n"
    "Block 2: 1 --> [13,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
