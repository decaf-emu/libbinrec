/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define POST_INSN_CALLBACK  ((void (*)(void *, uint32_t))2)

static const uint8_t input[] = {
    0x44,0x00,0x00,0x02,  // sc
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
    "    5: LOAD_IMM   r5, 0x44000002\n"
    "    6: CALL       r6, @r4, r1, r5\n"
    "    7: LOAD_IMM   r7, 0x2\n"
    "    8: LOAD_IMM   r8, 0\n"
    "    9: CALL_TRANSPARENT @r7, r6, r8\n"
    "   10: RETURN     r6\n"
    "   11: LOAD_IMM   r9, 4\n"
    "   12: SET_ALIAS  a1, r9\n"
    "   13: LOAD_IMM   r10, 0x2\n"
    "   14: LOAD_IMM   r11, 0\n"
    "   15: CALL_TRANSPARENT @r10, r1, r11\n"
    "   16: LOAD_IMM   r12, 4\n"
    "   17: SET_ALIAS  a1, r12\n"
    "   18: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> <none>\n"
    "Block 1: <none> --> [11,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
