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
    0x7C,0x64,0x29,0x2D,  // stwcx. r3,r4,r5
    /* stwcx. sets CR0[eq] conditionally and flushes it, which should not
     * confuse the optimizer. */
    0x48,0x00,0x00,0x04,  // b 0x8
    0x4C,0x42,0x11,0x82,  // crclr 2
    0x4C,0x63,0x19,0x82,  // crclr 3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 9\n"
        "[info] r6 no longer used, setting death = birth\n"
        "[info] Killing instruction 5\n"
        "[info] r5 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_U8    r3, 948(r1)\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: GET_ALIAS  r5, a10\n"
    "    5: NOP\n"
    "    6: SET_ALIAS  a6, r4\n"
    "    7: SET_ALIAS  a7, r4\n"
    "    8: SET_ALIAS  a8, r4\n"
    "    9: NOP\n"
    "   10: GOTO_IF_Z  r3, L2\n"
    "   11: STORE_I8   948(r1), r4\n"
    "   12: LOAD       r7, 952(r1)\n"
    "   13: BSWAP      r8, r7\n"
    "   14: GET_ALIAS  r9, a2\n"
    "   15: BSWAP      r10, r9\n"
    "   16: GET_ALIAS  r11, a3\n"
    "   17: GET_ALIAS  r12, a4\n"
    "   18: ADD        r13, r11, r12\n"
    "   19: ZCAST      r14, r13\n"
    "   20: ADD        r15, r2, r14\n"
    "   21: CMPXCHG    r16, (r15), r8, r10\n"
    "   22: SEQ        r17, r16, r8\n"
    "   23: GOTO_IF_Z  r17, L2\n"
    "   24: LOAD_IMM   r18, 1\n"
    "   25: SET_ALIAS  a8, r18\n"
    "   26: LABEL      L2\n"
    "   27: GOTO       L1\n"
    "   28: LOAD_IMM   r19, 8\n"
    "   29: SET_ALIAS  a1, r19\n"
    "   30: LABEL      L1\n"
    "   31: LOAD_IMM   r20, 0\n"
    "   32: SET_ALIAS  a8, r20\n"
    "   33: LOAD_IMM   r21, 0\n"
    "   34: SET_ALIAS  a9, r21\n"
    "   35: LOAD_IMM   r22, 16\n"
    "   36: SET_ALIAS  a1, r22\n"
    "   37: GET_ALIAS  r23, a5\n"
    "   38: ANDI       r24, r23, 268435455\n"
    "   39: GET_ALIAS  r25, a6\n"
    "   40: SLLI       r26, r25, 31\n"
    "   41: OR         r27, r24, r26\n"
    "   42: GET_ALIAS  r28, a7\n"
    "   43: SLLI       r29, r28, 30\n"
    "   44: OR         r30, r27, r29\n"
    "   45: GET_ALIAS  r31, a8\n"
    "   46: SLLI       r32, r31, 29\n"
    "   47: OR         r33, r30, r32\n"
    "   48: GET_ALIAS  r34, a9\n"
    "   49: SLLI       r35, r34, 28\n"
    "   50: OR         r36, r33, r35\n"
    "   51: SET_ALIAS  a5, r36\n"
    "   52: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,3\n"
    "Block 1: 0 --> [11,23] --> 2,3\n"
    "Block 2: 1 --> [24,25] --> 3\n"
    "Block 3: 2,0,1 --> [26,27] --> 5\n"
    "Block 4: <none> --> [28,29] --> 5\n"
    "Block 5: 4,3 --> [30,52] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
