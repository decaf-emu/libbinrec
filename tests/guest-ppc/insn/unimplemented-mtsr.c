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
    0x7C,0x61,0x01,0xA4,  // mtsr r3,1
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Unsupported instruction 7C6101A4 at address 0x0, treating as invalid\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: ILLEGAL\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
