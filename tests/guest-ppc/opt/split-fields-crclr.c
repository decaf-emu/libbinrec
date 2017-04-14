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
    0x4F,0xC0,0x01,0x82,  // crclr 30 (crxor 30,0,0)
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: LOAD_IMM   r5, 0\n"
    "    6: SET_ALIAS  a3, r5\n"
    "    7: LOAD_IMM   r6, 4\n"
    "    8: SET_ALIAS  a1, r6\n"
    "    9: GET_ALIAS  r7, a2\n"
    "   10: ANDI       r8, r7, -3\n"
    "   11: GET_ALIAS  r9, a3\n"
    "   12: SLLI       r10, r9, 1\n"
    "   13: OR         r11, r8, r10\n"
    "   14: SET_ALIAS  a2, r11\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
