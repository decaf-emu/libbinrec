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
    0x7C,0x64,0x2A,0x14,  // add r3,r4,r5
    0x7C,0xC3,0x2A,0x14,  // add r6,r3,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: ADD        r6, r5, r4\n"
    "    7: SET_ALIAS  a5, r6\n"
    "    8: LOAD_IMM   r7, 8\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 280(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
