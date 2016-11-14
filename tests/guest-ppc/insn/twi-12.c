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
    0x0E,0x43,0x12,0x34,  // twi 18,r3,4660
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
    "    4: SLTS       r5, r4, r3\n"
    "    5: SLTU       r6, r4, r3\n"
    "    6: OR         r7, r5, r6\n"
    "    7: GOTO_IF_Z  r7, L1\n"
    "    8: LOAD_IMM   r8, 0\n"
    "    9: SET_ALIAS  a1, r8\n"
    "   10: LOAD       r9, 984(r1)\n"
    "   11: CALL       @r9, r1\n"
    "   12: RETURN\n"
    "   13: LABEL      L1\n"
    "   14: LOAD_IMM   r10, 4\n"
    "   15: SET_ALIAS  a1, r10\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,12] --> <none>\n"
    "Block 2: 0 --> [13,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
