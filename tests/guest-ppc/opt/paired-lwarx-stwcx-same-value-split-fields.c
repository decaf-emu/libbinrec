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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_PAIRED_LWARX_STWCX
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 30, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 29, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 28, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a9\n"
    "   12: BFEXT      r9, r8, 31, 1\n"
    "   13: SET_ALIAS  a10, r9\n"
    "   14: GET_ALIAS  r10, a3\n"
    "   15: ZCAST      r11, r10\n"
    "   16: ADD        r12, r2, r11\n"
    "   17: LOAD       r13, 0(r12)\n"
    "   18: BSWAP      r14, r13\n"
    "   19: SET_ALIAS  a2, r14\n"
    "   20: LOAD_IMM   r15, 0\n"
    "   21: GET_ALIAS  r16, a10\n"
    "   22: STORE_I8   956(r1), r15\n"
    "   23: LOAD_IMM   r17, 1\n"
    "   24: SET_ALIAS  a5, r15\n"
    "   25: SET_ALIAS  a6, r15\n"
    "   26: SET_ALIAS  a7, r17\n"
    "   27: SET_ALIAS  a8, r16\n"
    "   28: LOAD_IMM   r18, 8\n"
    "   29: SET_ALIAS  a1, r18\n"
    "   30: GET_ALIAS  r19, a4\n"
    "   31: ANDI       r20, r19, 268435455\n"
    "   32: GET_ALIAS  r21, a5\n"
    "   33: SLLI       r22, r21, 31\n"
    "   34: OR         r23, r20, r22\n"
    "   35: GET_ALIAS  r24, a6\n"
    "   36: SLLI       r25, r24, 30\n"
    "   37: OR         r26, r23, r25\n"
    "   38: GET_ALIAS  r27, a7\n"
    "   39: SLLI       r28, r27, 29\n"
    "   40: OR         r29, r26, r28\n"
    "   41: GET_ALIAS  r30, a8\n"
    "   42: SLLI       r31, r30, 28\n"
    "   43: OR         r32, r29, r31\n"
    "   44: SET_ALIAS  a4, r32\n"
    "   45: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 940(r1)\n"
    "Alias 10: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,45] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
