/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define PRE_INSN_CALLBACK  ((void (*)(void *, uint32_t))1)

static const uint8_t input[] = {
    0x7C,0x64,0x00,0x34,  // cntlzw r4,r3
    0x54,0x83,0xD9,0x7E,  // rlwinm r3,r4,27,5,31 (srwi r3,r4,5)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 0x1\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: CALL_TRANSPARENT @r3, r1, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: CLZ        r6, r5\n"
    "    7: SET_ALIAS  a3, r6\n"
    "    8: LOAD_IMM   r7, 0x1\n"
    "    9: LOAD_IMM   r8, 4\n"
    "   10: CALL_TRANSPARENT @r7, r1, r8\n"
    "   11: SRLI       r9, r6, 5\n"
    "   12: SET_ALIAS  a2, r9\n"
    "   13: LOAD_IMM   r10, 8\n"
    "   14: SET_ALIAS  a1, r10\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "\n"
    "Block 0: <none> --> [0,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
