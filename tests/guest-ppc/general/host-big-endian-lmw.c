/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#define TEST_PPC_HOST_BIG_ENDIAN
#include "tests/guest-ppc/insn/common.h"

static const uint8_t input[] = {
    0xBB,0x64,0xFF,0xF8,  // lmw r27,-8(r4)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, -8(r5)\n"
    "    6: SET_ALIAS  a3, r6\n"
    "    7: LOAD       r7, -4(r5)\n"
    "    8: LOAD       r8, 0(r5)\n"
    "    9: LOAD       r9, 4(r5)\n"
    "   10: LOAD       r10, 8(r5)\n"
    "   11: SET_ALIAS  a4, r7\n"
    "   12: SET_ALIAS  a5, r8\n"
    "   13: SET_ALIAS  a6, r9\n"
    "   14: SET_ALIAS  a7, r10\n"
    "   15: LOAD_IMM   r11, 4\n"
    "   16: SET_ALIAS  a1, r11\n"
    "   17: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 272(r1)\n"
    "Alias 3: int32 @ 364(r1)\n"
    "Alias 4: int32 @ 368(r1)\n"
    "Alias 5: int32 @ 372(r1)\n"
    "Alias 6: int32 @ 376(r1)\n"
    "Alias 7: int32 @ 380(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
