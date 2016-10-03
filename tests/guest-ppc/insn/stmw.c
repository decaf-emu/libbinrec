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
    0xBF,0x64,0xFF,0xF8,  // stmw r27,-8(r4)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: ZCAST      r4, r3\n"
    "    5: ADD        r5, r2, r4\n"
    "    6: GET_ALIAS  r6, a3\n"
    "    7: STORE_BR   -8(r5), r6\n"
    "    8: GET_ALIAS  r7, a4\n"
    "    9: GET_ALIAS  r8, a5\n"
    "   10: GET_ALIAS  r9, a6\n"
    "   11: GET_ALIAS  r10, a7\n"
    "   12: STORE_BR   -4(r5), r7\n"
    "   13: STORE_BR   0(r5), r8\n"
    "   14: STORE_BR   4(r5), r9\n"
    "   15: STORE_BR   8(r5), r10\n"
    "   16: LOAD_IMM   r11, 4\n"
    "   17: SET_ALIAS  a1, r11\n"
    "   18: LABEL      L2\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 272(r1)\n"
    "Alias 3: int32 @ 364(r1)\n"
    "Alias 4: int32 @ 368(r1)\n"
    "Alias 5: int32 @ 372(r1)\n"
    "Alias 6: int32 @ 376(r1)\n"
    "Alias 7: int32 @ 380(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,17] --> 2\n"
    "Block 2: 1 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"