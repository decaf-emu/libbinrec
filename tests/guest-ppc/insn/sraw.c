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
    0x7C,0x83,0x2E,0x30,  // sraw r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: ANDI       r5, r4, 63\n"
    "    5: SCAST      r6, r3\n"
    "    6: SRA        r7, r6, r5\n"
    "    7: ZCAST      r8, r7\n"
    "    8: SET_ALIAS  a2, r8\n"
    "    9: LOAD_IMM   r9, 1\n"
    "   10: SLL        r10, r9, r5\n"
    "   11: ADDI       r11, r10, -1\n"
    "   12: AND        r12, r6, r11\n"
    "   13: SGTUI      r13, r12, 0\n"
    "   14: SLTSI      r14, r6, 0\n"
    "   15: AND        r15, r13, r14\n"
    "   16: GET_ALIAS  r16, a5\n"
    "   17: BFINS      r17, r16, r15, 29, 1\n"
    "   18: SET_ALIAS  a5, r17\n"
    "   19: LOAD_IMM   r18, 4\n"
    "   20: SET_ALIAS  a1, r18\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
