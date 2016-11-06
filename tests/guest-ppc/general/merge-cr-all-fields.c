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
    0x7C,0x8F,0x01,0x20,  // mtcrf 240,r4
    0x7C,0xA0,0xF1,0x20,  // mtcrf 15,r5
    0x7C,0x60,0x00,0x26,  // mfcr r3
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: BFEXT      r5, r3, 30, 1\n"
    "    5: BFEXT      r6, r3, 29, 1\n"
    "    6: BFEXT      r7, r3, 28, 1\n"
    "    7: SET_ALIAS  a5, r4\n"
    "    8: SET_ALIAS  a6, r5\n"
    "    9: SET_ALIAS  a7, r6\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: BFEXT      r8, r3, 27, 1\n"
    "   12: BFEXT      r9, r3, 26, 1\n"
    "   13: BFEXT      r10, r3, 25, 1\n"
    "   14: BFEXT      r11, r3, 24, 1\n"
    "   15: SET_ALIAS  a9, r8\n"
    "   16: SET_ALIAS  a10, r9\n"
    "   17: SET_ALIAS  a11, r10\n"
    "   18: SET_ALIAS  a12, r11\n"
    "   19: BFEXT      r12, r3, 23, 1\n"
    "   20: BFEXT      r13, r3, 22, 1\n"
    "   21: BFEXT      r14, r3, 21, 1\n"
    "   22: BFEXT      r15, r3, 20, 1\n"
    "   23: SET_ALIAS  a13, r12\n"
    "   24: SET_ALIAS  a14, r13\n"
    "   25: SET_ALIAS  a15, r14\n"
    "   26: SET_ALIAS  a16, r15\n"
    "   27: BFEXT      r16, r3, 19, 1\n"
    "   28: BFEXT      r17, r3, 18, 1\n"
    "   29: BFEXT      r18, r3, 17, 1\n"
    "   30: BFEXT      r19, r3, 16, 1\n"
    "   31: SET_ALIAS  a17, r16\n"
    "   32: SET_ALIAS  a18, r17\n"
    "   33: SET_ALIAS  a19, r18\n"
    "   34: SET_ALIAS  a20, r19\n"
    "   35: GET_ALIAS  r20, a4\n"
    "   36: BFEXT      r21, r20, 15, 1\n"
    "   37: BFEXT      r22, r20, 14, 1\n"
    "   38: BFEXT      r23, r20, 13, 1\n"
    "   39: BFEXT      r24, r20, 12, 1\n"
    "   40: SET_ALIAS  a21, r21\n"
    "   41: SET_ALIAS  a22, r22\n"
    "   42: SET_ALIAS  a23, r23\n"
    "   43: SET_ALIAS  a24, r24\n"
    "   44: BFEXT      r25, r20, 11, 1\n"
    "   45: BFEXT      r26, r20, 10, 1\n"
    "   46: BFEXT      r27, r20, 9, 1\n"
    "   47: BFEXT      r28, r20, 8, 1\n"
    "   48: SET_ALIAS  a25, r25\n"
    "   49: SET_ALIAS  a26, r26\n"
    "   50: SET_ALIAS  a27, r27\n"
    "   51: SET_ALIAS  a28, r28\n"
    "   52: BFEXT      r29, r20, 7, 1\n"
    "   53: BFEXT      r30, r20, 6, 1\n"
    "   54: BFEXT      r31, r20, 5, 1\n"
    "   55: BFEXT      r32, r20, 4, 1\n"
    "   56: SET_ALIAS  a29, r29\n"
    "   57: SET_ALIAS  a30, r30\n"
    "   58: SET_ALIAS  a31, r31\n"
    "   59: SET_ALIAS  a32, r32\n"
    "   60: BFEXT      r33, r20, 3, 1\n"
    "   61: BFEXT      r34, r20, 2, 1\n"
    "   62: BFEXT      r35, r20, 1, 1\n"
    "   63: BFEXT      r36, r20, 0, 1\n"
    "   64: SET_ALIAS  a33, r33\n"
    "   65: SET_ALIAS  a34, r34\n"
    "   66: SET_ALIAS  a35, r35\n"
    "   67: SET_ALIAS  a36, r36\n"
    "   68: SLLI       r37, r4, 31\n"
    "   69: SLLI       r38, r5, 30\n"
    "   70: OR         r39, r37, r38\n"
    "   71: SLLI       r40, r6, 29\n"
    "   72: OR         r41, r39, r40\n"
    "   73: SLLI       r42, r7, 28\n"
    "   74: OR         r43, r41, r42\n"
    "   75: SLLI       r44, r8, 27\n"
    "   76: OR         r45, r43, r44\n"
    "   77: SLLI       r46, r9, 26\n"
    "   78: OR         r47, r45, r46\n"
    "   79: SLLI       r48, r10, 25\n"
    "   80: OR         r49, r47, r48\n"
    "   81: SLLI       r50, r11, 24\n"
    "   82: OR         r51, r49, r50\n"
    "   83: SLLI       r52, r12, 23\n"
    "   84: OR         r53, r51, r52\n"
    "   85: SLLI       r54, r13, 22\n"
    "   86: OR         r55, r53, r54\n"
    "   87: SLLI       r56, r14, 21\n"
    "   88: OR         r57, r55, r56\n"
    "   89: SLLI       r58, r15, 20\n"
    "   90: OR         r59, r57, r58\n"
    "   91: SLLI       r60, r16, 19\n"
    "   92: OR         r61, r59, r60\n"
    "   93: SLLI       r62, r17, 18\n"
    "   94: OR         r63, r61, r62\n"
    "   95: SLLI       r64, r18, 17\n"
    "   96: OR         r65, r63, r64\n"
    "   97: SLLI       r66, r19, 16\n"
    "   98: OR         r67, r65, r66\n"
    "   99: SLLI       r68, r21, 15\n"
    "  100: OR         r69, r67, r68\n"
    "  101: SLLI       r70, r22, 14\n"
    "  102: OR         r71, r69, r70\n"
    "  103: SLLI       r72, r23, 13\n"
    "  104: OR         r73, r71, r72\n"
    "  105: SLLI       r74, r24, 12\n"
    "  106: OR         r75, r73, r74\n"
    "  107: SLLI       r76, r25, 11\n"
    "  108: OR         r77, r75, r76\n"
    "  109: SLLI       r78, r26, 10\n"
    "  110: OR         r79, r77, r78\n"
    "  111: SLLI       r80, r27, 9\n"
    "  112: OR         r81, r79, r80\n"
    "  113: SLLI       r82, r28, 8\n"
    "  114: OR         r83, r81, r82\n"
    "  115: SLLI       r84, r29, 7\n"
    "  116: OR         r85, r83, r84\n"
    "  117: SLLI       r86, r30, 6\n"
    "  118: OR         r87, r85, r86\n"
    "  119: SLLI       r88, r31, 5\n"
    "  120: OR         r89, r87, r88\n"
    "  121: SLLI       r90, r32, 4\n"
    "  122: OR         r91, r89, r90\n"
    "  123: SLLI       r92, r33, 3\n"
    "  124: OR         r93, r91, r92\n"
    "  125: SLLI       r94, r34, 2\n"
    "  126: OR         r95, r93, r94\n"
    "  127: SLLI       r96, r35, 1\n"
    "  128: OR         r97, r95, r96\n"
    "  129: OR         r98, r97, r36\n"
    "  130: SET_ALIAS  a37, r98\n"
    "  131: SET_ALIAS  a2, r98\n"
    "  132: LOAD_IMM   r99, 12\n"
    "  133: SET_ALIAS  a1, r99\n"
    "  134: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32, no bound storage\n"
    "Alias 12: int32, no bound storage\n"
    "Alias 13: int32, no bound storage\n"
    "Alias 14: int32, no bound storage\n"
    "Alias 15: int32, no bound storage\n"
    "Alias 16: int32, no bound storage\n"
    "Alias 17: int32, no bound storage\n"
    "Alias 18: int32, no bound storage\n"
    "Alias 19: int32, no bound storage\n"
    "Alias 20: int32, no bound storage\n"
    "Alias 21: int32, no bound storage\n"
    "Alias 22: int32, no bound storage\n"
    "Alias 23: int32, no bound storage\n"
    "Alias 24: int32, no bound storage\n"
    "Alias 25: int32, no bound storage\n"
    "Alias 26: int32, no bound storage\n"
    "Alias 27: int32, no bound storage\n"
    "Alias 28: int32, no bound storage\n"
    "Alias 29: int32, no bound storage\n"
    "Alias 30: int32, no bound storage\n"
    "Alias 31: int32, no bound storage\n"
    "Alias 32: int32, no bound storage\n"
    "Alias 33: int32, no bound storage\n"
    "Alias 34: int32, no bound storage\n"
    "Alias 35: int32, no bound storage\n"
    "Alias 36: int32, no bound storage\n"
    "Alias 37: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,134] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
