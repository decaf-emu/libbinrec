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
    /* Note that we currently ignore the address on icbi and return
     * unconditionally, so we don't need to initialize these registers. */
    0x7C,0x03,0x27,0xAC,  // icbi r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: GOTO       L2\n"
    "    6: LOAD_IMM   r4, 4\n"
    "    7: SET_ALIAS  a1, r4\n"
    "    8: LABEL      L2\n"
    "    9: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,5] --> 3\n"
    "Block 2: <none> --> [6,7] --> 3\n"
    "Block 3: 2,1 --> [8,9] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
