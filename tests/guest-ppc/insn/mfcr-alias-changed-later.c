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
    0x2F,0x04,0x00,0x04,  // cmpi cr6,r4,4
    0x7C,0x60,0x00,0x26,  // mfcr r3
    0x2F,0x85,0x00,0x05,  // cmpi cr7,r5,5
    0x7C,0x80,0x00,0x26,  // mfcr r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    /* mfcr should cause the aliases to be preloaded even though the only
     * accesses to them as individual fields are cmpi stores. */
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 4, 4\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 0, 4\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: LABEL      L1\n"
    "    8: GET_ALIAS  r6, a3\n"
    "    9: SLTSI      r7, r6, 4\n"
    "   10: SGTSI      r8, r6, 4\n"
    "   11: SEQI       r9, r6, 4\n"
    "   12: GET_ALIAS  r11, a7\n"
    "   13: BFEXT      r10, r11, 31, 1\n"
    "   14: SLLI       r12, r9, 1\n"
    "   15: OR         r13, r10, r12\n"
    "   16: SLLI       r14, r8, 2\n"
    "   17: OR         r15, r13, r14\n"
    "   18: SLLI       r16, r7, 3\n"
    "   19: OR         r17, r15, r16\n"
    "   20: LOAD       r18, 928(r1)\n"
    "   21: ANDI       r19, r18, -256\n"
    "   22: SLLI       r20, r17, 4\n"
    "   23: OR         r21, r19, r20\n"
    "   24: GET_ALIAS  r22, a6\n"
    "   25: OR         r23, r21, r22\n"
    "   26: GET_ALIAS  r24, a4\n"
    "   27: SLTSI      r25, r24, 5\n"
    "   28: SGTSI      r26, r24, 5\n"
    "   29: SEQI       r27, r24, 5\n"
    "   30: BFEXT      r28, r11, 31, 1\n"
    "   31: SLLI       r29, r27, 1\n"
    "   32: OR         r30, r28, r29\n"
    "   33: SLLI       r31, r26, 2\n"
    "   34: OR         r32, r30, r31\n"
    "   35: SLLI       r33, r25, 3\n"
    "   36: OR         r34, r32, r33\n"
    "   37: LOAD       r35, 928(r1)\n"
    "   38: ANDI       r36, r35, -256\n"
    "   39: SLLI       r37, r17, 4\n"
    "   40: OR         r38, r36, r37\n"
    "   41: OR         r39, r38, r34\n"
    "   42: SET_ALIAS  a2, r23\n"
    "   43: SET_ALIAS  a3, r39\n"
    "   44: SET_ALIAS  a5, r17\n"
    "   45: SET_ALIAS  a6, r34\n"
    "   46: LOAD_IMM   r40, 16\n"
    "   47: SET_ALIAS  a1, r40\n"
    "   48: LABEL      L2\n"
    "   49: LOAD       r41, 928(r1)\n"
    "   50: ANDI       r42, r41, -256\n"
    "   51: GET_ALIAS  r43, a5\n"
    "   52: SLLI       r44, r43, 4\n"
    "   53: OR         r45, r42, r44\n"
    "   54: GET_ALIAS  r46, a6\n"
    "   55: OR         r47, r45, r46\n"
    "   56: STORE      928(r1), r47\n"
    "   57: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1\n"
    "Block 1: 0 --> [7,47] --> 2\n"
    "Block 2: 1 --> [48,57] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
