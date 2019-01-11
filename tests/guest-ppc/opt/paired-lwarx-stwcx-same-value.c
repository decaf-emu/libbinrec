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
    0x7C,0x60,0x20,0x28,  // lwarx r3,0,r4
    0x7C,0x60,0x21,0x2D,  // stwcx. r3,0,r4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_PAIRED_LWARX_STWCX;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, 0(r5)\n"
    "    6: BSWAP      r7, r6\n"
    "    7: SET_ALIAS  a2, r7\n"
    "    8: LOAD_IMM   r8, 0\n"
    "    9: GET_ALIAS  r9, a5\n"
    "   10: BFEXT      r10, r9, 31, 1\n"
    "   11: STORE_I8   956(r1), r8\n"
    "   12: GET_ALIAS  r11, a4\n"
    "   13: ORI        r12, r10, 2\n"
    "   14: BFINS      r13, r11, r12, 28, 4\n"
    "   15: SET_ALIAS  a4, r13\n"
    "   16: LOAD_IMM   r14, 8\n"
    "   17: SET_ALIAS  a1, r14\n"
    "   18: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
