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
    0xBB,0xC4,0x7F,0xFC,  // lmw r30,32764(r4)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: ADDI       r6, r5, 32764\n"
    "    6: LOAD_BR    r7, 0(r6)\n"
    "    7: LOAD_BR    r8, 4(r6)\n"
    "    8: SET_ALIAS  a3, r7\n"
    "    9: SET_ALIAS  a4, r8\n"
    "   10: LOAD_IMM   r9, 4\n"
    "   11: SET_ALIAS  a1, r9\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 272(r1)\n"
    "Alias 3: int32 @ 376(r1)\n"
    "Alias 4: int32 @ 380(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
