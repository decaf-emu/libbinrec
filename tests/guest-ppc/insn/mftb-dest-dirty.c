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
    0x38,0x60,0x00,0x00,  // li r3,0
    0x7C,0x6C,0x42,0xE6,  // mftb r3
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 0\n"
    "    4: SET_ALIAS  a2, r3\n"
    "    5: LOAD       r4, 968(r1)\n"
    "    6: GOTO_IF_Z  r4, L3\n"
    "    7: CALL       r5, @r4, r1\n"
    "    8: ZCAST      r6, r5\n"
    "    9: SET_ALIAS  a2, r6\n"
    "   10: GOTO       L4\n"
    "   11: LABEL      L3\n"
    "   12: LOAD_IMM   r7, 0\n"
    "   13: SET_ALIAS  a2, r7\n"
    "   14: LABEL      L4\n"
    "   15: LOAD_IMM   r8, 8\n"
    "   16: SET_ALIAS  a1, r8\n"
    "   17: LABEL      L2\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,6] --> 2,3\n"
    "Block 2: 1 --> [7,10] --> 4\n"
    "Block 3: 1 --> [11,13] --> 4\n"
    "Block 4: 3,2 --> [14,16] --> 5\n"
    "Block 5: 4 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
