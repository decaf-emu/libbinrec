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
    "   13: GET_ALIAS  r8, a2\n"
    "   14: BSWAP      r9, r8\n"
    "   15: GET_ALIAS  r10, a3\n"
    "   16: GET_ALIAS  r11, a4\n"
    "   17: ADD        r12, r10, r11\n"
    "   18: ZCAST      r13, r12\n"
    "   19: ADD        r14, r2, r13\n"
    "   20: CMPXCHG    r15, (r14), r7, r9\n"
    "   21: SEQ        r16, r15, r7\n"
    "   22: GOTO_IF_Z  r16, L2\n"
    "   23: LOAD_IMM   r17, 1\n"
    "   24: SET_ALIAS  a8, r17\n"
    "   25: LABEL      L2\n"
    "   26: GOTO       L1\n"
    "   27: LOAD_IMM   r18, 8\n"
    "   28: SET_ALIAS  a1, r18\n"
    "   29: LABEL      L1\n"
    "   30: LOAD_IMM   r19, 0\n"
    "   31: SET_ALIAS  a8, r19\n"
    "   32: LOAD_IMM   r20, 0\n"
    "   33: SET_ALIAS  a9, r20\n"
    "   34: LOAD_IMM   r21, 16\n"
    "   35: SET_ALIAS  a1, r21\n"
    "   36: GET_ALIAS  r22, a5\n"
    "   37: ANDI       r23, r22, 268435455\n"
    "   38: GET_ALIAS  r24, a6\n"
    "   39: SLLI       r25, r24, 31\n"
    "   40: OR         r26, r23, r25\n"
    "   41: GET_ALIAS  r27, a7\n"
    "   42: SLLI       r28, r27, 30\n"
    "   43: OR         r29, r26, r28\n"
    "   44: GET_ALIAS  r30, a8\n"
    "   45: SLLI       r31, r30, 29\n"
    "   46: OR         r32, r29, r31\n"
    "   47: GET_ALIAS  r33, a9\n"
    "   48: SLLI       r34, r33, 28\n"
    "   49: OR         r35, r32, r34\n"
    "   50: SET_ALIAS  a5, r35\n"
    "   51: RETURN\n"
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
    "Block 1: 0 --> [11,22] --> 2,3\n"
    "Block 2: 1 --> [23,24] --> 3\n"
    "Block 3: 2,0,1 --> [25,26] --> 5\n"
    "Block 4: <none> --> [27,28] --> 5\n"
    "Block 5: 4,3 --> [29,51] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
