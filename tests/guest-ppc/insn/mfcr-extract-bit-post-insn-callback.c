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
    0x7C,0x80,0x00,0x26,  // mfcr r4
    0x54,0x83,0x17,0xFE,  // rlwinm r3,r4,2,31,31
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: SET_ALIAS  a3, r3\n"
    "    4: LOAD_IMM   r4, 4\n"
    "    5: SET_ALIAS  a1, r4\n"
    "    6: LOAD_IMM   r5, 0x2\n"
    "    7: LOAD_IMM   r6, 0\n"
    "    8: CALL_TRANSPARENT @r5, r1, r6\n"
    "    9: RORI       r7, r3, 30\n"
    "   10: ANDI       r8, r7, 1\n"
    "   11: SET_ALIAS  a2, r8\n"
    "   12: LOAD_IMM   r9, 8\n"
    "   13: SET_ALIAS  a1, r9\n"
    "   14: LOAD_IMM   r10, 0x2\n"
    "   15: LOAD_IMM   r11, 4\n"
    "   16: CALL_TRANSPARENT @r10, r1, r11\n"
    "   17: LOAD_IMM   r12, 8\n"
    "   18: SET_ALIAS  a1, r12\n"
    "   19: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
