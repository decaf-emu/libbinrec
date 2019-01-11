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
    0x7C,0x64,0x01,0x90,  // subfze r3,r4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: NOT        r4, r3\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: BFEXT      r6, r5, 29, 1\n"
    "    6: ADD        r7, r4, r6\n"
    "    7: SET_ALIAS  a2, r7\n"
    "    8: SRLI       r8, r4, 31\n"
    "    9: SRLI       r9, r7, 31\n"
    "   10: XORI       r10, r9, 1\n"
    "   11: AND        r11, r8, r10\n"
    "   12: BFINS      r12, r5, r11, 29, 1\n"
    "   13: SET_ALIAS  a4, r12\n"
    "   14: LOAD_IMM   r13, 4\n"
    "   15: SET_ALIAS  a1, r13\n"
    "   16: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
