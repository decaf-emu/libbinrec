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
    0x7C,0x64,0x1B,0x78,  // mr r4,r3
    0x7C,0x6C,0x42,0xE6,  // mftb r3
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: LOAD       r4, 968(r1)\n"
    "    4: GOTO_IF_Z  r4, L1\n"
    "    5: CALL       r5, @r4, r1\n"
    "    6: ZCAST      r6, r5\n"
    "    7: SET_ALIAS  a2, r6\n"
    "    8: GOTO       L2\n"
    "    9: LABEL      L1\n"
    "   10: LOAD_IMM   r7, 0\n"
    "   11: SET_ALIAS  a2, r7\n"
    "   12: LABEL      L2\n"
    "   13: SET_ALIAS  a3, r3\n"
    "   14: LOAD_IMM   r8, 8\n"
    "   15: SET_ALIAS  a1, r8\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1,2\n"
    "Block 1: 0 --> [5,8] --> 3\n"
    "Block 2: 0 --> [9,11] --> 3\n"
    "Block 3: 2,1 --> [12,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
