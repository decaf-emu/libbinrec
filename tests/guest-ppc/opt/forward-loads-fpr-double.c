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
    0xC8,0x23,0xFF,0xF0,  // lfd f1,-16(r3)
    0xD8,0x23,0xFF,0xF8,  // stfd f1,-8(r3)
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_FORWARD_LOADS
                                    | BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, -16(r5)\n"
    "    6: BSWAP      r7, r6\n"
    "    7: BITCAST    r8, r7\n"
    "    8: ZCAST      r9, r3\n"
    "    9: ADD        r10, r2, r9\n"
    "   10: STORE      -8(r10), r6\n"
    "   11: SET_ALIAS  a3, r8\n"
    "   12: LOAD_IMM   r11, 8\n"
    "   13: SET_ALIAS  a1, r11\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64 @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
