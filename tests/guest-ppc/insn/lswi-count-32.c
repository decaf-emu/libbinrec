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
    0x7C,0x62,0x04,0xAA,  // lswi r3,r2,32
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
    "    5: LOAD_U8    r6, 0(r5)\n"
    "    6: LOAD_U8    r7, 1(r5)\n"
    "    7: LOAD_U8    r8, 2(r5)\n"
    "    8: LOAD_U8    r9, 3(r5)\n"
    "    9: STORE_I8   271(r1), r6\n"
    "   10: STORE_I8   270(r1), r7\n"
    "   11: STORE_I8   269(r1), r8\n"
    "   12: STORE_I8   268(r1), r9\n"
    "   13: LOAD_U8    r10, 4(r5)\n"
    "   14: LOAD_U8    r11, 5(r5)\n"
    "   15: LOAD_U8    r12, 6(r5)\n"
    "   16: LOAD_U8    r13, 7(r5)\n"
    "   17: STORE_I8   275(r1), r10\n"
    "   18: STORE_I8   274(r1), r11\n"
    "   19: STORE_I8   273(r1), r12\n"
    "   20: STORE_I8   272(r1), r13\n"
    "   21: LOAD_U8    r14, 8(r5)\n"
    "   22: LOAD_U8    r15, 9(r5)\n"
    "   23: LOAD_U8    r16, 10(r5)\n"
    "   24: LOAD_U8    r17, 11(r5)\n"
    "   25: STORE_I8   279(r1), r14\n"
    "   26: STORE_I8   278(r1), r15\n"
    "   27: STORE_I8   277(r1), r16\n"
    "   28: STORE_I8   276(r1), r17\n"
    "   29: LOAD_U8    r18, 12(r5)\n"
    "   30: LOAD_U8    r19, 13(r5)\n"
    "   31: LOAD_U8    r20, 14(r5)\n"
    "   32: LOAD_U8    r21, 15(r5)\n"
    "   33: STORE_I8   283(r1), r18\n"
    "   34: STORE_I8   282(r1), r19\n"
    "   35: STORE_I8   281(r1), r20\n"
    "   36: STORE_I8   280(r1), r21\n"
    "   37: LOAD_U8    r22, 16(r5)\n"
    "   38: LOAD_U8    r23, 17(r5)\n"
    "   39: LOAD_U8    r24, 18(r5)\n"
    "   40: LOAD_U8    r25, 19(r5)\n"
    "   41: STORE_I8   287(r1), r22\n"
    "   42: STORE_I8   286(r1), r23\n"
    "   43: STORE_I8   285(r1), r24\n"
    "   44: STORE_I8   284(r1), r25\n"
    "   45: LOAD_U8    r26, 20(r5)\n"
    "   46: LOAD_U8    r27, 21(r5)\n"
    "   47: LOAD_U8    r28, 22(r5)\n"
    "   48: LOAD_U8    r29, 23(r5)\n"
    "   49: STORE_I8   291(r1), r26\n"
    "   50: STORE_I8   290(r1), r27\n"
    "   51: STORE_I8   289(r1), r28\n"
    "   52: STORE_I8   288(r1), r29\n"
    "   53: LOAD_U8    r30, 24(r5)\n"
    "   54: LOAD_U8    r31, 25(r5)\n"
    "   55: LOAD_U8    r32, 26(r5)\n"
    "   56: LOAD_U8    r33, 27(r5)\n"
    "   57: STORE_I8   295(r1), r30\n"
    "   58: STORE_I8   294(r1), r31\n"
    "   59: STORE_I8   293(r1), r32\n"
    "   60: STORE_I8   292(r1), r33\n"
    "   61: LOAD_U8    r34, 28(r5)\n"
    "   62: LOAD_U8    r35, 29(r5)\n"
    "   63: LOAD_U8    r36, 30(r5)\n"
    "   64: LOAD_U8    r37, 31(r5)\n"
    "   65: STORE_I8   299(r1), r34\n"
    "   66: STORE_I8   298(r1), r35\n"
    "   67: STORE_I8   297(r1), r36\n"
    "   68: STORE_I8   296(r1), r37\n"
    "   69: LOAD_IMM   r38, 4\n"
    "   70: SET_ALIAS  a1, r38\n"
    "   71: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 264(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32 @ 272(r1)\n"
    "Alias 5: int32 @ 276(r1)\n"
    "Alias 6: int32 @ 280(r1)\n"
    "Alias 7: int32 @ 284(r1)\n"
    "Alias 8: int32 @ 288(r1)\n"
    "Alias 9: int32 @ 292(r1)\n"
    "Alias 10: int32 @ 296(r1)\n"
    "\n"
    "Block 0: <none> --> [0,71] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
