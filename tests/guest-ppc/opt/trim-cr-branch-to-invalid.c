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
    0x48,0x00,0x00,0x04,  // b 0x4
    0x00,0x00,0x00,0x00,  // (invalid)
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 4\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: GOTO       L2\n"
    "    5: LOAD_IMM   r4, 4\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L1\n"
    "    8: LOAD_IMM   r5, 4\n"
    "    9: SET_ALIAS  a1, r5\n"
    "   10: GOTO       L2\n"
    "   11: LABEL      L2\n"
    "   12: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 3\n"
    "Block 1: <none> --> [5,6] --> 2\n"
    "Block 2: 1 --> [7,10] --> 3\n"
    "Block 3: 0,2 --> [11,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
