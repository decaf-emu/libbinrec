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
    0x30,0x60,0x12,0x34,  // addic r3,r0,4660
    0x7C,0x81,0x02,0xA6,  // mfxer r4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ADDI       r4, r3, 4660\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a5\n"
    "    6: SLTUI      r6, r4, 4660\n"
    "    7: BFINS      r7, r5, r6, 29, 1\n"
    "    8: SET_ALIAS  a5, r7\n"
    "    9: SET_ALIAS  a4, r7\n"
    "   10: LOAD_IMM   r8, 8\n"
    "   11: SET_ALIAS  a1, r8\n"
    "   12: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32 @ 272(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
