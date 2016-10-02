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
    /* Normally, CR7 would not be loaded because it's not referenced except
     * by the mfcr instruction, which does not allocate aliases on its own.
     * However, since the mfcr precedes the first store to CR7, its value
     * should be loaded here so it's available to the mfcr code.  CR6 is
     * written before the mfcr, so it doesn't need to be loaded separately.
     * The second mfcr should not affect this logic. */
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a6, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: SLTSI      r6, r5, 4\n"
    "    8: SGTSI      r7, r5, 4\n"
    "    9: SEQI       r8, r5, 4\n"
    "   10: GET_ALIAS  r10, a7\n"
    "   11: BFEXT      r9, r10, 31, 1\n"
    "   12: SLLI       r11, r8, 1\n"
    "   13: OR         r12, r9, r11\n"
    "   14: SLLI       r13, r7, 2\n"
    "   15: OR         r14, r12, r13\n"
    "   16: SLLI       r15, r6, 3\n"
    "   17: OR         r16, r14, r15\n"
    "   18: LOAD       r17, 928(r1)\n"
    "   19: ANDI       r18, r17, -256\n"
    "   20: SLLI       r19, r16, 4\n"
    "   21: OR         r20, r18, r19\n"
    "   22: GET_ALIAS  r21, a6\n"
    "   23: OR         r22, r20, r21\n"
    "   24: GET_ALIAS  r23, a4\n"
    "   25: SLTSI      r24, r23, 5\n"
    "   26: SGTSI      r25, r23, 5\n"
    "   27: SEQI       r26, r23, 5\n"
    "   28: BFEXT      r27, r10, 31, 1\n"
    "   29: SLLI       r28, r26, 1\n"
    "   30: OR         r29, r27, r28\n"
    "   31: SLLI       r30, r25, 2\n"
    "   32: OR         r31, r29, r30\n"
    "   33: SLLI       r32, r24, 3\n"
    "   34: OR         r33, r31, r32\n"
    "   35: LOAD       r34, 928(r1)\n"
    "   36: ANDI       r35, r34, -256\n"
    "   37: SLLI       r36, r16, 4\n"
    "   38: OR         r37, r35, r36\n"
    "   39: OR         r38, r37, r33\n"
    "   40: SET_ALIAS  a2, r22\n"
    "   41: SET_ALIAS  a3, r38\n"
    "   42: SET_ALIAS  a5, r16\n"
    "   43: SET_ALIAS  a6, r33\n"
    "   44: LOAD_IMM   r39, 16\n"
    "   45: SET_ALIAS  a1, r39\n"
    "   46: LABEL      L2\n"
    "   47: LOAD       r40, 928(r1)\n"
    "   48: ANDI       r41, r40, -256\n"
    "   49: GET_ALIAS  r42, a5\n"
    "   50: SLLI       r43, r42, 4\n"
    "   51: OR         r44, r41, r43\n"
    "   52: GET_ALIAS  r45, a6\n"
    "   53: OR         r46, r44, r45\n"
    "   54: STORE      928(r1), r46\n"
    "   55: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,45] --> 2\n"
    "Block 2: 1 --> [46,55] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
