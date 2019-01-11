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
    0x47,0xFF,0xFF,0xFF,  // (nonstandard sc encoding)
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 4\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: LOAD       r4, 984(r1)\n"
    "    5: LOAD_IMM   r5, 0x47FFFFFF\n"
    "    6: CALL       r6, @r4, r1, r5\n"
    "    7: RETURN     r6\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> <none>\n"
    "Block 1: <none> --> [8,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
