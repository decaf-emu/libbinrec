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
    0xFC,0x22,0x20,0xFA,  // fmadd f1,f2,f3,f4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: FMADD      r6, r3, r4, r5\n"
    "    6: LOAD_IMM   r7, 0.0\n"
    "    7: FCMP       r8, r3, r7, NUN\n"
    "    8: FCMP       r9, r4, r7, UN\n"
    "    9: FCMP       r10, r5, r7, UN\n"
    "   10: AND        r11, r8, r9\n"
    "   11: AND        r12, r11, r10\n"
    "   12: BITCAST    r13, r5\n"
    "   13: LOAD_IMM   r14, 0x8000000000000\n"
    "   14: OR         r15, r13, r14\n"
    "   15: BITCAST    r16, r15\n"
    "   16: SELECT     r17, r16, r6, r12\n"
    "   17: LOAD_IMM   r18, 0x20000000000000\n"
    "   18: BITCAST    r19, r17\n"
    "   19: SLLI       r20, r19, 1\n"
    "   20: SEQ        r21, r20, r18\n"
    "   21: GOTO_IF_Z  r21, L1\n"
    "   22: FGETSTATE  r22\n"
    "   23: FSETROUND  r23, r22, TRUNC\n"
    "   24: FSETSTATE  r23\n"
    "   25: FMADD      r24, r3, r4, r5\n"
    "   26: FGETSTATE  r25\n"
    "   27: FCOPYROUND r26, r25, r22\n"
    "   28: FSETSTATE  r26\n"
    "   29: LABEL      L1\n"
    "   30: GET_ALIAS  r27, a6\n"
    "   31: FGETSTATE  r28\n"
    "   32: FCLEAREXC  r29, r28\n"
    "   33: FSETSTATE  r29\n"
    "   34: FTESTEXC   r30, r28, INVALID\n"
    "   35: GOTO_IF_Z  r30, L3\n"
    "   36: BITCAST    r31, r3\n"
    "   37: SLLI       r32, r31, 13\n"
    "   38: BFEXT      r33, r31, 51, 12\n"
    "   39: SEQI       r34, r33, 4094\n"
    "   40: GOTO_IF_Z  r34, L5\n"
    "   41: GOTO_IF_NZ r32, L4\n"
    "   42: LABEL      L5\n"
    "   43: BITCAST    r35, r4\n"
    "   44: SLLI       r36, r35, 13\n"
    "   45: BFEXT      r37, r35, 51, 12\n"
    "   46: SEQI       r38, r37, 4094\n"
    "   47: GOTO_IF_Z  r38, L6\n"
    "   48: GOTO_IF_NZ r36, L4\n"
    "   49: LABEL      L6\n"
    "   50: BITCAST    r39, r5\n"
    "   51: SLLI       r40, r39, 13\n"
    "   52: BFEXT      r41, r39, 51, 12\n"
    "   53: SEQI       r42, r41, 4094\n"
    "   54: GOTO_IF_Z  r42, L8\n"
    "   55: GOTO_IF_NZ r40, L7\n"
    "   56: LABEL      L8\n"
    "   57: BITCAST    r43, r3\n"
    "   58: BITCAST    r44, r4\n"
    "   59: SLLI       r45, r43, 1\n"
    "   60: SLLI       r46, r44, 1\n"
    "   61: LOAD_IMM   r47, 0xFFE0000000000000\n"
    "   62: SEQ        r48, r45, r47\n"
    "   63: GOTO_IF_Z  r48, L9\n"
    "   64: GOTO_IF_Z  r46, L10\n"
    "   65: GOTO       L11\n"
    "   66: LABEL      L9\n"
    "   67: GOTO_IF_NZ r45, L11\n"
    "   68: LABEL      L10\n"
    "   69: NOT        r49, r27\n"
    "   70: ORI        r50, r27, 1048576\n"
    "   71: ANDI       r51, r49, 1048576\n"
    "   72: SET_ALIAS  a6, r50\n"
    "   73: GOTO_IF_Z  r51, L12\n"
    "   74: ORI        r52, r50, -2147483648\n"
    "   75: SET_ALIAS  a6, r52\n"
    "   76: LABEL      L12\n"
    "   77: GOTO       L13\n"
    "   78: LABEL      L11\n"
    "   79: NOT        r53, r27\n"
    "   80: ORI        r54, r27, 8388608\n"
    "   81: ANDI       r55, r53, 8388608\n"
    "   82: SET_ALIAS  a6, r54\n"
    "   83: GOTO_IF_Z  r55, L14\n"
    "   84: ORI        r56, r54, -2147483648\n"
    "   85: SET_ALIAS  a6, r56\n"
    "   86: LABEL      L14\n"
    "   87: LABEL      L13\n"
    "   88: ANDI       r57, r27, 128\n"
    "   89: GOTO_IF_Z  r57, L15\n"
    "   90: GET_ALIAS  r58, a6\n"
    "   91: BFEXT      r59, r58, 12, 7\n"
    "   92: ANDI       r60, r59, 31\n"
    "   93: GET_ALIAS  r61, a6\n"
    "   94: BFINS      r62, r61, r60, 12, 7\n"
    "   95: SET_ALIAS  a6, r62\n"
    "   96: GOTO       L2\n"
    "   97: LABEL      L15\n"
    "   98: LOAD_IMM   r63, nan(0x8000000000000)\n"
    "   99: SET_ALIAS  a2, r63\n"
    "  100: LOAD_IMM   r64, 17\n"
    "  101: GET_ALIAS  r65, a6\n"
    "  102: BFINS      r66, r65, r64, 12, 7\n"
    "  103: SET_ALIAS  a6, r66\n"
    "  104: GOTO       L2\n"
    "  105: LABEL      L7\n"
    "  106: FMUL       r67, r3, r4\n"
    "  107: FGETSTATE  r68\n"
    "  108: FSETSTATE  r29\n"
    "  109: FTESTEXC   r69, r68, INVALID\n"
    "  110: GOTO_IF_Z  r69, L4\n"
    "  111: NOT        r70, r27\n"
    "  112: ORI        r71, r27, 17825792\n"
    "  113: ANDI       r72, r70, 17825792\n"
    "  114: SET_ALIAS  a6, r71\n"
    "  115: GOTO_IF_Z  r72, L16\n"
    "  116: ORI        r73, r71, -2147483648\n"
    "  117: SET_ALIAS  a6, r73\n"
    "  118: LABEL      L16\n"
    "  119: GOTO       L17\n"
    "  120: LABEL      L4\n"
    "  121: NOT        r74, r27\n"
    "  122: ORI        r75, r27, 16777216\n"
    "  123: ANDI       r76, r74, 16777216\n"
    "  124: SET_ALIAS  a6, r75\n"
    "  125: GOTO_IF_Z  r76, L18\n"
    "  126: ORI        r77, r75, -2147483648\n"
    "  127: SET_ALIAS  a6, r77\n"
    "  128: LABEL      L18\n"
    "  129: LABEL      L17\n"
    "  130: ANDI       r78, r27, 128\n"
    "  131: GOTO_IF_Z  r78, L3\n"
    "  132: GET_ALIAS  r79, a6\n"
    "  133: BFEXT      r80, r79, 12, 7\n"
    "  134: ANDI       r81, r80, 31\n"
    "  135: GET_ALIAS  r82, a6\n"
    "  136: BFINS      r83, r82, r81, 12, 7\n"
    "  137: SET_ALIAS  a6, r83\n"
    "  138: GOTO       L2\n"
    "  139: LABEL      L3\n"
    "  140: SET_ALIAS  a2, r17\n"
    "  141: BITCAST    r84, r17\n"
    "  142: SGTUI      r85, r84, 0\n"
    "  143: SLTSI      r86, r84, 0\n"
    "  144: BFEXT      r90, r84, 52, 11\n"
    "  145: SEQI       r87, r90, 0\n"
    "  146: SEQI       r88, r90, 2047\n"
    "  147: SLLI       r91, r84, 12\n"
    "  148: SEQI       r89, r91, 0\n"
    "  149: AND        r92, r87, r89\n"
    "  150: XORI       r93, r89, 1\n"
    "  151: AND        r94, r88, r93\n"
    "  152: AND        r95, r87, r85\n"
    "  153: OR         r96, r95, r94\n"
    "  154: OR         r97, r92, r94\n"
    "  155: XORI       r98, r97, 1\n"
    "  156: XORI       r99, r86, 1\n"
    "  157: AND        r100, r86, r98\n"
    "  158: AND        r101, r99, r98\n"
    "  159: SLLI       r102, r96, 4\n"
    "  160: SLLI       r103, r100, 3\n"
    "  161: SLLI       r104, r101, 2\n"
    "  162: SLLI       r105, r92, 1\n"
    "  163: OR         r106, r102, r103\n"
    "  164: OR         r107, r104, r105\n"
    "  165: OR         r108, r106, r88\n"
    "  166: OR         r109, r108, r107\n"
    "  167: FTESTEXC   r110, r28, INEXACT\n"
    "  168: SLLI       r111, r110, 5\n"
    "  169: OR         r112, r109, r111\n"
    "  170: GET_ALIAS  r113, a6\n"
    "  171: BFINS      r114, r113, r112, 12, 7\n"
    "  172: SET_ALIAS  a6, r114\n"
    "  173: GOTO_IF_Z  r110, L19\n"
    "  174: GET_ALIAS  r115, a6\n"
    "  175: NOT        r116, r115\n"
    "  176: ORI        r117, r115, 33554432\n"
    "  177: ANDI       r118, r116, 33554432\n"
    "  178: SET_ALIAS  a6, r117\n"
    "  179: GOTO_IF_Z  r118, L20\n"
    "  180: ORI        r119, r117, -2147483648\n"
    "  181: SET_ALIAS  a6, r119\n"
    "  182: LABEL      L20\n"
    "  183: LABEL      L19\n"
    "  184: FTESTEXC   r120, r28, OVERFLOW\n"
    "  185: GOTO_IF_Z  r120, L21\n"
    "  186: GET_ALIAS  r121, a6\n"
    "  187: NOT        r122, r121\n"
    "  188: ORI        r123, r121, 268435456\n"
    "  189: ANDI       r124, r122, 268435456\n"
    "  190: SET_ALIAS  a6, r123\n"
    "  191: GOTO_IF_Z  r124, L22\n"
    "  192: ORI        r125, r123, -2147483648\n"
    "  193: SET_ALIAS  a6, r125\n"
    "  194: LABEL      L22\n"
    "  195: LABEL      L21\n"
    "  196: FTESTEXC   r126, r28, UNDERFLOW\n"
    "  197: GOTO_IF_Z  r126, L2\n"
    "  198: GET_ALIAS  r127, a6\n"
    "  199: NOT        r128, r127\n"
    "  200: ORI        r129, r127, 134217728\n"
    "  201: ANDI       r130, r128, 134217728\n"
    "  202: SET_ALIAS  a6, r129\n"
    "  203: GOTO_IF_Z  r130, L23\n"
    "  204: ORI        r131, r129, -2147483648\n"
    "  205: SET_ALIAS  a6, r131\n"
    "  206: LABEL      L23\n"
    "  207: LABEL      L2\n"
    "  208: LOAD_IMM   r132, 4\n"
    "  209: SET_ALIAS  a1, r132\n"
    "  210: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> 1,2\n"
    "Block 1: 0 --> [22,28] --> 2\n"
    "Block 2: 1,0 --> [29,35] --> 3,31\n"
    "Block 3: 2 --> [36,40] --> 4,5\n"
    "Block 4: 3 --> [41,41] --> 5,26\n"
    "Block 5: 4,3 --> [42,47] --> 6,7\n"
    "Block 6: 5 --> [48,48] --> 7,26\n"
    "Block 7: 6,5 --> [49,54] --> 8,9\n"
    "Block 8: 7 --> [55,55] --> 9,22\n"
    "Block 9: 8,7 --> [56,63] --> 10,12\n"
    "Block 10: 9 --> [64,64] --> 11,13\n"
    "Block 11: 10 --> [65,65] --> 16\n"
    "Block 12: 9 --> [66,67] --> 13,16\n"
    "Block 13: 12,10 --> [68,73] --> 14,15\n"
    "Block 14: 13 --> [74,75] --> 15\n"
    "Block 15: 14,13 --> [76,77] --> 19\n"
    "Block 16: 11,12 --> [78,83] --> 17,18\n"
    "Block 17: 16 --> [84,85] --> 18\n"
    "Block 18: 17,16 --> [86,86] --> 19\n"
    "Block 19: 18,15 --> [87,89] --> 20,21\n"
    "Block 20: 19 --> [90,96] --> 43\n"
    "Block 21: 19 --> [97,104] --> 43\n"
    "Block 22: 8 --> [105,110] --> 23,26\n"
    "Block 23: 22 --> [111,115] --> 24,25\n"
    "Block 24: 23 --> [116,117] --> 25\n"
    "Block 25: 24,23 --> [118,119] --> 29\n"
    "Block 26: 4,6,22 --> [120,125] --> 27,28\n"
    "Block 27: 26 --> [126,127] --> 28\n"
    "Block 28: 27,26 --> [128,128] --> 29\n"
    "Block 29: 28,25 --> [129,131] --> 30,31\n"
    "Block 30: 29 --> [132,138] --> 43\n"
    "Block 31: 2,29 --> [139,173] --> 32,35\n"
    "Block 32: 31 --> [174,179] --> 33,34\n"
    "Block 33: 32 --> [180,181] --> 34\n"
    "Block 34: 33,32 --> [182,182] --> 35\n"
    "Block 35: 34,31 --> [183,185] --> 36,39\n"
    "Block 36: 35 --> [186,191] --> 37,38\n"
    "Block 37: 36 --> [192,193] --> 38\n"
    "Block 38: 37,36 --> [194,194] --> 39\n"
    "Block 39: 38,35 --> [195,197] --> 40,43\n"
    "Block 40: 39 --> [198,203] --> 41,42\n"
    "Block 41: 40 --> [204,205] --> 42\n"
    "Block 42: 41,40 --> [206,206] --> 43\n"
    "Block 43: 42,20,21,30,39 --> [207,210] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
