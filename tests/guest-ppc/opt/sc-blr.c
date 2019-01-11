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
    0x44,0x00,0x00,0x02,  // sc
    0x4E,0x80,0x00,0x20,  // blr
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_SC_BLR;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, -4\n"
    "    4: SET_ALIAS  a1, r4\n"
    "    5: LOAD       r5, 984(r1)\n"
    "    6: LOAD_IMM   r6, 0x44000002\n"
    "    7: CALL       r7, @r5, r1, r6\n"
    "    8: RETURN     r7\n"
    "    9: LOAD_IMM   r8, 8\n"
    "   10: SET_ALIAS  a1, r8\n"
    "   11: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> <none>\n"
    "Block 1: <none> --> [9,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
