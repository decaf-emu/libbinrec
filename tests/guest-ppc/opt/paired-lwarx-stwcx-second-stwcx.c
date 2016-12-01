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
    0x7C,0xA0,0x21,0x2D,  // stwcx. r5,0,r4
    0x7C,0xA0,0x21,0x2D,  // stwcx. r5,0,r4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_PAIRED_LWARX_STWCX;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, 0(r5)\n"
    "    6: BSWAP      r7, r6\n"
    "    7: SET_ALIAS  a2, r7\n"
    "    8: LOAD_IMM   r8, 0\n"
    "    9: GET_ALIAS  r9, a6\n"
    "   10: BFEXT      r10, r9, 31, 1\n"
    "   11: GET_ALIAS  r11, a5\n"
    "   12: BFINS      r12, r11, r10, 28, 4\n"
    "   13: SET_ALIAS  a5, r12\n"
    "   14: STORE_I8   948(r1), r8\n"
    "   15: GET_ALIAS  r13, a4\n"
    "   16: BSWAP      r14, r13\n"
    "   17: ZCAST      r15, r3\n"
    "   18: ADD        r16, r2, r15\n"
    "   19: CMPXCHG    r17, (r16), r6, r14\n"
    "   20: SEQ        r18, r17, r6\n"
    "   21: GOTO_IF_Z  r18, L1\n"
    "   22: GET_ALIAS  r19, a5\n"
    "   23: ORI        r20, r19, 536870912\n"
    "   24: SET_ALIAS  a5, r20\n"
    "   25: LABEL      L1\n"
    "   26: LOAD_U8    r21, 948(r1)\n"
    "   27: LOAD_IMM   r22, 0\n"
    "   28: BFEXT      r23, r9, 31, 1\n"
    "   29: GET_ALIAS  r24, a5\n"
    "   30: BFINS      r25, r24, r23, 28, 4\n"
    "   31: SET_ALIAS  a5, r25\n"
    "   32: GOTO_IF_Z  r21, L2\n"
    "   33: STORE_I8   948(r1), r22\n"
    "   34: LOAD_BR    r26, 952(r1)\n"
    "   35: BSWAP      r27, r13\n"
    "   36: ZCAST      r28, r3\n"
    "   37: ADD        r29, r2, r28\n"
    "   38: CMPXCHG    r30, (r29), r26, r27\n"
    "   39: SEQ        r31, r30, r26\n"
    "   40: GOTO_IF_Z  r31, L2\n"
    "   41: GET_ALIAS  r32, a5\n"
    "   42: ORI        r33, r32, 536870912\n"
    "   43: SET_ALIAS  a5, r33\n"
    "   44: LABEL      L2\n"
    "   45: LOAD_IMM   r34, 12\n"
    "   46: SET_ALIAS  a1, r34\n"
    "   47: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> 1,2\n"
    "Block 1: 0 --> [22,24] --> 2\n"
    "Block 2: 1,0 --> [25,32] --> 3,5\n"
    "Block 3: 2 --> [33,40] --> 4,5\n"
    "Block 4: 3 --> [41,43] --> 5\n"
    "Block 5: 4,2,3 --> [44,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
