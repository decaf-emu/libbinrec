/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

/* Keep the blr outside the range of code to translate; the translator
 * should not optimize this case. */
static const struct {
    uint8_t input[4];
    uint8_t extra[4];
} input_struct = {
    {0x44,0x00,0x00,0x02},  // sc
    {0x4E,0x80,0x00,0x20},  // blr
};
#define input input_struct.input

static const unsigned int guest_opt = BINREC_OPT_G_PPC_SC_BLR;
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
