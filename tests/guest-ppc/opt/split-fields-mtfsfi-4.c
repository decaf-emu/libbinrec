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
    0xFE,0x00,0xF1,0x0C,  // mtfsfi 4,15
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a3\n"
    "    6: ANDI       r6, r5, 112\n"
    "    7: ORI        r7, r6, 15\n"
    "    8: SET_ALIAS  a3, r7\n"
    "    9: LOAD_IMM   r8, 4\n"
    "   10: SET_ALIAS  a1, r8\n"
    "   11: GET_ALIAS  r9, a2\n"
    "   12: GET_ALIAS  r10, a3\n"
    "   13: ANDI       r11, r9, -1611134977\n"
    "   14: SLLI       r12, r10, 12\n"
    "   15: OR         r13, r11, r12\n"
    "   16: SET_ALIAS  a2, r13\n"
    "   17: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
