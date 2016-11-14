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
    0x0E,0xA3,0x12,0x34,  // twi 21,r3,4660
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 4660\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: SGTS       r5, r4, r3\n"
    "    5: XORI       r6, r5, 1\n"
    "    6: SGTU       r7, r4, r3\n"
    "    7: OR         r8, r6, r7\n"
    "    8: GOTO_IF_Z  r8, L1\n"
    "    9: LOAD_IMM   r9, 0\n"
    "   10: SET_ALIAS  a1, r9\n"
    "   11: LOAD       r10, 984(r1)\n"
    "   12: CALL       @r10, r1\n"
    "   13: RETURN\n"
    "   14: LABEL      L1\n"
    "   15: LOAD_IMM   r11, 4\n"
    "   16: SET_ALIAS  a1, r11\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,2\n"
    "Block 1: 0 --> [9,13] --> <none>\n"
    "Block 2: 0 --> [14,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
