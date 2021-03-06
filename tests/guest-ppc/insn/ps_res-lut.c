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
    0x10,0x20,0x10,0x30,  // ps_res f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VFCVT      r4, r3\n"
    "    4: VEXTRACT   r5, r4, 0\n"
    "    5: BITCAST    r6, r5\n"
    "    6: ANDI       r7, r6, -2147483648\n"
    "    7: BFEXT      r8, r6, 0, 23\n"
    "    8: SET_ALIAS  a6, r8\n"
    "    9: BFEXT      r9, r6, 23, 8\n"
    "   10: SET_ALIAS  a5, r9\n"
    "   11: GOTO_IF_Z  r9, L2\n"
    "   12: SEQI       r10, r9, 255\n"
    "   13: GOTO_IF_NZ r10, L3\n"
    "   14: GOTO       L4\n"
    "   15: LABEL      L3\n"
    "   16: GOTO_IF_NZ r8, L5\n"
    "   17: BITCAST    r11, r7\n"
    "   18: SET_ALIAS  a4, r11\n"
    "   19: GOTO       L1\n"
    "   20: LABEL      L5\n"
    "   21: ORI        r12, r6, 4194304\n"
    "   22: BITCAST    r13, r12\n"
    "   23: SET_ALIAS  a4, r13\n"
    "   24: GOTO       L1\n"
    "   25: LABEL      L2\n"
    "   26: GOTO_IF_NZ r8, L6\n"
    "   27: ORI        r14, r7, 2139095040\n"
    "   28: BITCAST    r15, r14\n"
    "   29: SET_ALIAS  a4, r15\n"
    "   30: GOTO       L1\n"
    "   31: LABEL      L6\n"
    "   32: SLTUI      r16, r8, 2097152\n"
    "   33: GOTO_IF_Z  r16, L7\n"
    "   34: ORI        r17, r7, 2139095039\n"
    "   35: BITCAST    r18, r17\n"
    "   36: SET_ALIAS  a4, r18\n"
    "   37: GOTO       L1\n"
    "   38: LABEL      L7\n"
    "   39: SLLI       r19, r8, 1\n"
    "   40: SET_ALIAS  a6, r19\n"
    "   41: ANDI       r20, r19, 8388608\n"
    "   42: GOTO_IF_NZ r20, L8\n"
    "   43: LOAD_IMM   r21, -1\n"
    "   44: SET_ALIAS  a5, r21\n"
    "   45: SLLI       r22, r19, 1\n"
    "   46: SET_ALIAS  a6, r22\n"
    "   47: LABEL      L8\n"
    "   48: GET_ALIAS  r23, a6\n"
    "   49: ANDI       r24, r23, 8388607\n"
    "   50: SET_ALIAS  a6, r24\n"
    "   51: LABEL      L4\n"
    "   52: LOAD       r25, 1016(r1)\n"
    "   53: GET_ALIAS  r26, a6\n"
    "   54: SRLI       r27, r26, 18\n"
    "   55: SLLI       r28, r27, 2\n"
    "   56: LOAD_IMM   r29, 253\n"
    "   57: GET_ALIAS  r30, a5\n"
    "   58: SUB        r31, r29, r30\n"
    "   59: SET_ALIAS  a5, r31\n"
    "   60: ZCAST      r32, r28\n"
    "   61: ADD        r33, r25, r32\n"
    "   62: LOAD_U16   r34, 2(r33)\n"
    "   63: LOAD_U16   r35, 0(r33)\n"
    "   64: BFEXT      r36, r26, 8, 10\n"
    "   65: MUL        r37, r36, r34\n"
    "   66: SLLI       r38, r35, 10\n"
    "   67: SUB        r39, r38, r37\n"
    "   68: SRLI       r40, r39, 1\n"
    "   69: SET_ALIAS  a6, r40\n"
    "   70: GOTO_IF_Z  r31, L9\n"
    "   71: SLTSI      r41, r31, 0\n"
    "   72: GOTO_IF_NZ r41, L10\n"
    "   73: LABEL      L11\n"
    "   74: GET_ALIAS  r42, a6\n"
    "   75: OR         r43, r42, r7\n"
    "   76: GET_ALIAS  r44, a5\n"
    "   77: SLLI       r45, r44, 23\n"
    "   78: OR         r46, r43, r45\n"
    "   79: BITCAST    r47, r46\n"
    "   80: SET_ALIAS  a4, r47\n"
    "   81: GOTO       L1\n"
    "   82: LABEL      L10\n"
    "   83: LOAD_IMM   r48, 0\n"
    "   84: SET_ALIAS  a5, r48\n"
    "   85: GET_ALIAS  r49, a6\n"
    "   86: ORI        r50, r49, 8388608\n"
    "   87: SRLI       r51, r50, 2\n"
    "   88: SET_ALIAS  a6, r51\n"
    "   89: GOTO       L11\n"
    "   90: LABEL      L9\n"
    "   91: GET_ALIAS  r52, a6\n"
    "   92: ORI        r53, r52, 8388608\n"
    "   93: SRLI       r54, r53, 1\n"
    "   94: SET_ALIAS  a6, r54\n"
    "   95: GOTO       L11\n"
    "   96: LABEL      L1\n"
    "   97: GET_ALIAS  r55, a4\n"
    "   98: VEXTRACT   r56, r4, 1\n"
    "   99: BITCAST    r57, r56\n"
    "  100: ANDI       r58, r57, -2147483648\n"
    "  101: BFEXT      r59, r57, 0, 23\n"
    "  102: SET_ALIAS  a9, r59\n"
    "  103: BFEXT      r60, r57, 23, 8\n"
    "  104: SET_ALIAS  a8, r60\n"
    "  105: GOTO_IF_Z  r60, L13\n"
    "  106: SEQI       r61, r60, 255\n"
    "  107: GOTO_IF_NZ r61, L14\n"
    "  108: GOTO       L15\n"
    "  109: LABEL      L14\n"
    "  110: GOTO_IF_NZ r59, L16\n"
    "  111: BITCAST    r62, r58\n"
    "  112: SET_ALIAS  a7, r62\n"
    "  113: GOTO       L12\n"
    "  114: LABEL      L16\n"
    "  115: ORI        r63, r57, 4194304\n"
    "  116: BITCAST    r64, r63\n"
    "  117: SET_ALIAS  a7, r64\n"
    "  118: GOTO       L12\n"
    "  119: LABEL      L13\n"
    "  120: GOTO_IF_NZ r59, L17\n"
    "  121: ORI        r65, r58, 2139095040\n"
    "  122: BITCAST    r66, r65\n"
    "  123: SET_ALIAS  a7, r66\n"
    "  124: GOTO       L12\n"
    "  125: LABEL      L17\n"
    "  126: SLTUI      r67, r59, 2097152\n"
    "  127: GOTO_IF_Z  r67, L18\n"
    "  128: ORI        r68, r58, 2139095039\n"
    "  129: BITCAST    r69, r68\n"
    "  130: SET_ALIAS  a7, r69\n"
    "  131: GOTO       L12\n"
    "  132: LABEL      L18\n"
    "  133: SLLI       r70, r59, 1\n"
    "  134: SET_ALIAS  a9, r70\n"
    "  135: ANDI       r71, r70, 8388608\n"
    "  136: GOTO_IF_NZ r71, L19\n"
    "  137: LOAD_IMM   r72, -1\n"
    "  138: SET_ALIAS  a8, r72\n"
    "  139: SLLI       r73, r70, 1\n"
    "  140: SET_ALIAS  a9, r73\n"
    "  141: LABEL      L19\n"
    "  142: GET_ALIAS  r74, a9\n"
    "  143: ANDI       r75, r74, 8388607\n"
    "  144: SET_ALIAS  a9, r75\n"
    "  145: LABEL      L15\n"
    "  146: LOAD       r76, 1016(r1)\n"
    "  147: GET_ALIAS  r77, a9\n"
    "  148: SRLI       r78, r77, 18\n"
    "  149: SLLI       r79, r78, 2\n"
    "  150: LOAD_IMM   r80, 253\n"
    "  151: GET_ALIAS  r81, a8\n"
    "  152: SUB        r82, r80, r81\n"
    "  153: SET_ALIAS  a8, r82\n"
    "  154: ZCAST      r83, r79\n"
    "  155: ADD        r84, r76, r83\n"
    "  156: LOAD_U16   r85, 2(r84)\n"
    "  157: LOAD_U16   r86, 0(r84)\n"
    "  158: BFEXT      r87, r77, 8, 10\n"
    "  159: MUL        r88, r87, r85\n"
    "  160: SLLI       r89, r86, 10\n"
    "  161: SUB        r90, r89, r88\n"
    "  162: SRLI       r91, r90, 1\n"
    "  163: SET_ALIAS  a9, r91\n"
    "  164: GOTO_IF_Z  r82, L20\n"
    "  165: SLTSI      r92, r82, 0\n"
    "  166: GOTO_IF_NZ r92, L21\n"
    "  167: LABEL      L22\n"
    "  168: GET_ALIAS  r93, a9\n"
    "  169: OR         r94, r93, r58\n"
    "  170: GET_ALIAS  r95, a8\n"
    "  171: SLLI       r96, r95, 23\n"
    "  172: OR         r97, r94, r96\n"
    "  173: BITCAST    r98, r97\n"
    "  174: SET_ALIAS  a7, r98\n"
    "  175: GOTO       L12\n"
    "  176: LABEL      L21\n"
    "  177: LOAD_IMM   r99, 0\n"
    "  178: SET_ALIAS  a8, r99\n"
    "  179: GET_ALIAS  r100, a9\n"
    "  180: ORI        r101, r100, 8388608\n"
    "  181: SRLI       r102, r101, 2\n"
    "  182: SET_ALIAS  a9, r102\n"
    "  183: GOTO       L22\n"
    "  184: LABEL      L20\n"
    "  185: GET_ALIAS  r103, a9\n"
    "  186: ORI        r104, r103, 8388608\n"
    "  187: SRLI       r105, r104, 1\n"
    "  188: SET_ALIAS  a9, r105\n"
    "  189: GOTO       L22\n"
    "  190: LABEL      L12\n"
    "  191: GET_ALIAS  r106, a7\n"
    "  192: VBUILD2    r107, r55, r106\n"
    "  193: VFCVT      r108, r107\n"
    "  194: SET_ALIAS  a2, r108\n"
    "  195: LOAD_IMM   r109, 4\n"
    "  196: SET_ALIAS  a1, r109\n"
    "  197: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: float32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,6\n"
    "Block 1: 0 --> [12,13] --> 2,3\n"
    "Block 2: 1 --> [14,14] --> 13\n"
    "Block 3: 1 --> [15,16] --> 4,5\n"
    "Block 4: 3 --> [17,19] --> 18\n"
    "Block 5: 3 --> [20,24] --> 18\n"
    "Block 6: 0 --> [25,26] --> 7,8\n"
    "Block 7: 6 --> [27,30] --> 18\n"
    "Block 8: 6 --> [31,33] --> 9,10\n"
    "Block 9: 8 --> [34,37] --> 18\n"
    "Block 10: 8 --> [38,42] --> 11,12\n"
    "Block 11: 10 --> [43,46] --> 12\n"
    "Block 12: 11,10 --> [47,50] --> 13\n"
    "Block 13: 12,2 --> [51,70] --> 14,17\n"
    "Block 14: 13 --> [71,72] --> 15,16\n"
    "Block 15: 14,16,17 --> [73,81] --> 18\n"
    "Block 16: 14 --> [82,89] --> 15\n"
    "Block 17: 13 --> [90,95] --> 15\n"
    "Block 18: 4,5,7,9,15 --> [96,105] --> 19,24\n"
    "Block 19: 18 --> [106,107] --> 20,21\n"
    "Block 20: 19 --> [108,108] --> 31\n"
    "Block 21: 19 --> [109,110] --> 22,23\n"
    "Block 22: 21 --> [111,113] --> 36\n"
    "Block 23: 21 --> [114,118] --> 36\n"
    "Block 24: 18 --> [119,120] --> 25,26\n"
    "Block 25: 24 --> [121,124] --> 36\n"
    "Block 26: 24 --> [125,127] --> 27,28\n"
    "Block 27: 26 --> [128,131] --> 36\n"
    "Block 28: 26 --> [132,136] --> 29,30\n"
    "Block 29: 28 --> [137,140] --> 30\n"
    "Block 30: 29,28 --> [141,144] --> 31\n"
    "Block 31: 30,20 --> [145,164] --> 32,35\n"
    "Block 32: 31 --> [165,166] --> 33,34\n"
    "Block 33: 32,34,35 --> [167,175] --> 36\n"
    "Block 34: 32 --> [176,183] --> 33\n"
    "Block 35: 31 --> [184,189] --> 33\n"
    "Block 36: 22,23,25,27,33 --> [190,197] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
