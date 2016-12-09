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
    0x10,0x22,0x19,0x16,  // ps_sum1 f1,f2,f4,f3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VEXTRACT   r4, r3, 0\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: VEXTRACT   r6, r5, 1\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: FGETSTATE  r8\n"
    "    8: VEXTRACT   r9, r7, 0\n"
    "    9: FCAST      r10, r9\n"
    "   10: FSETSTATE  r8\n"
    "   11: FADD       r11, r4, r6\n"
    "   12: FCVT       r12, r11\n"
    "   13: LOAD_IMM   r13, 0x1000000\n"
    "   14: BITCAST    r14, r12\n"
    "   15: SLLI       r15, r14, 1\n"
    "   16: SEQ        r16, r15, r13\n"
    "   17: GOTO_IF_Z  r16, L1\n"
    "   18: FGETSTATE  r17\n"
    "   19: FSETROUND  r18, r17, TRUNC\n"
    "   20: FSETSTATE  r18\n"
    "   21: FADD       r19, r4, r6\n"
    "   22: FCVT       r20, r19\n"
    "   23: FGETSTATE  r21\n"
    "   24: FCOPYROUND r22, r21, r17\n"
    "   25: FSETSTATE  r22\n"
    "   26: LABEL      L1\n"
    "   27: FGETSTATE  r23\n"
    "   28: FTESTEXC   r24, r23, INVALID\n"
    "   29: GOTO_IF_Z  r24, L2\n"
    "   30: GET_ALIAS  r25, a6\n"
    "   31: FGETSTATE  r26\n"
    "   32: FCLEAREXC  r27, r26\n"
    "   33: FSETSTATE  r27\n"
    "   34: FTESTEXC   r28, r26, INVALID\n"
    "   35: GOTO_IF_Z  r28, L4\n"
    "   36: BITCAST    r29, r4\n"
    "   37: SLLI       r30, r29, 13\n"
    "   38: BFEXT      r31, r29, 51, 12\n"
    "   39: SEQI       r32, r31, 4094\n"
    "   40: GOTO_IF_Z  r32, L6\n"
    "   41: GOTO_IF_NZ r30, L5\n"
    "   42: LABEL      L6\n"
    "   43: BITCAST    r33, r6\n"
    "   44: SLLI       r34, r33, 13\n"
    "   45: BFEXT      r35, r33, 51, 12\n"
    "   46: SEQI       r36, r35, 4094\n"
    "   47: GOTO_IF_Z  r36, L7\n"
    "   48: GOTO_IF_NZ r34, L5\n"
    "   49: LABEL      L7\n"
    "   50: NOT        r37, r25\n"
    "   51: ORI        r38, r25, 8388608\n"
    "   52: ANDI       r39, r37, 8388608\n"
    "   53: SET_ALIAS  a6, r38\n"
    "   54: GOTO_IF_Z  r39, L8\n"
    "   55: ORI        r40, r38, -2147483648\n"
    "   56: SET_ALIAS  a6, r40\n"
    "   57: LABEL      L8\n"
    "   58: ANDI       r41, r25, 128\n"
    "   59: GOTO_IF_Z  r41, L9\n"
    "   60: GET_ALIAS  r42, a6\n"
    "   61: BFEXT      r43, r42, 12, 7\n"
    "   62: ANDI       r44, r43, 31\n"
    "   63: GET_ALIAS  r45, a6\n"
    "   64: BFINS      r46, r45, r44, 12, 7\n"
    "   65: SET_ALIAS  a6, r46\n"
    "   66: GOTO       L3\n"
    "   67: LABEL      L9\n"
    "   68: LOAD_IMM   r47, nan(0x400000)\n"
    "   69: FCVT       r48, r47\n"
    "   70: VBROADCAST r49, r48\n"
    "   71: SET_ALIAS  a2, r49\n"
    "   72: LOAD_IMM   r50, 17\n"
    "   73: GET_ALIAS  r51, a6\n"
    "   74: BFINS      r52, r51, r50, 12, 7\n"
    "   75: SET_ALIAS  a6, r52\n"
    "   76: GOTO       L3\n"
    "   77: LABEL      L5\n"
    "   78: NOT        r53, r25\n"
    "   79: ORI        r54, r25, 16777216\n"
    "   80: ANDI       r55, r53, 16777216\n"
    "   81: SET_ALIAS  a6, r54\n"
    "   82: GOTO_IF_Z  r55, L10\n"
    "   83: ORI        r56, r54, -2147483648\n"
    "   84: SET_ALIAS  a6, r56\n"
    "   85: LABEL      L10\n"
    "   86: ANDI       r57, r25, 128\n"
    "   87: GOTO_IF_Z  r57, L4\n"
    "   88: GET_ALIAS  r58, a6\n"
    "   89: BFEXT      r59, r58, 12, 7\n"
    "   90: ANDI       r60, r59, 31\n"
    "   91: GET_ALIAS  r61, a6\n"
    "   92: BFINS      r62, r61, r60, 12, 7\n"
    "   93: SET_ALIAS  a6, r62\n"
    "   94: GOTO       L3\n"
    "   95: LABEL      L4\n"
    "   96: FCVT       r63, r12\n"
    "   97: VBROADCAST r64, r63\n"
    "   98: SET_ALIAS  a2, r64\n"
    "   99: BITCAST    r65, r12\n"
    "  100: SGTUI      r66, r65, 0\n"
    "  101: SRLI       r67, r65, 31\n"
    "  102: BFEXT      r71, r65, 23, 8\n"
    "  103: SEQI       r68, r71, 0\n"
    "  104: SEQI       r69, r71, 255\n"
    "  105: SLLI       r72, r65, 9\n"
    "  106: SEQI       r70, r72, 0\n"
    "  107: AND        r73, r68, r70\n"
    "  108: XORI       r74, r70, 1\n"
    "  109: AND        r75, r69, r74\n"
    "  110: AND        r76, r68, r66\n"
    "  111: OR         r77, r76, r75\n"
    "  112: OR         r78, r73, r75\n"
    "  113: XORI       r79, r78, 1\n"
    "  114: XORI       r80, r67, 1\n"
    "  115: AND        r81, r67, r79\n"
    "  116: AND        r82, r80, r79\n"
    "  117: SLLI       r83, r77, 4\n"
    "  118: SLLI       r84, r81, 3\n"
    "  119: SLLI       r85, r82, 2\n"
    "  120: SLLI       r86, r73, 1\n"
    "  121: OR         r87, r83, r84\n"
    "  122: OR         r88, r85, r86\n"
    "  123: OR         r89, r87, r69\n"
    "  124: OR         r90, r89, r88\n"
    "  125: FTESTEXC   r91, r26, INEXACT\n"
    "  126: SLLI       r92, r91, 5\n"
    "  127: OR         r93, r90, r92\n"
    "  128: GET_ALIAS  r94, a6\n"
    "  129: BFINS      r95, r94, r93, 12, 7\n"
    "  130: SET_ALIAS  a6, r95\n"
    "  131: GOTO_IF_Z  r91, L11\n"
    "  132: GET_ALIAS  r96, a6\n"
    "  133: NOT        r97, r96\n"
    "  134: ORI        r98, r96, 33554432\n"
    "  135: ANDI       r99, r97, 33554432\n"
    "  136: SET_ALIAS  a6, r98\n"
    "  137: GOTO_IF_Z  r99, L12\n"
    "  138: ORI        r100, r98, -2147483648\n"
    "  139: SET_ALIAS  a6, r100\n"
    "  140: LABEL      L12\n"
    "  141: LABEL      L11\n"
    "  142: FTESTEXC   r101, r26, OVERFLOW\n"
    "  143: GOTO_IF_Z  r101, L13\n"
    "  144: GET_ALIAS  r102, a6\n"
    "  145: NOT        r103, r102\n"
    "  146: ORI        r104, r102, 268435456\n"
    "  147: ANDI       r105, r103, 268435456\n"
    "  148: SET_ALIAS  a6, r104\n"
    "  149: GOTO_IF_Z  r105, L14\n"
    "  150: ORI        r106, r104, -2147483648\n"
    "  151: SET_ALIAS  a6, r106\n"
    "  152: LABEL      L14\n"
    "  153: LABEL      L13\n"
    "  154: FTESTEXC   r107, r26, UNDERFLOW\n"
    "  155: GOTO_IF_Z  r107, L3\n"
    "  156: GET_ALIAS  r108, a6\n"
    "  157: NOT        r109, r108\n"
    "  158: ORI        r110, r108, 134217728\n"
    "  159: ANDI       r111, r109, 134217728\n"
    "  160: SET_ALIAS  a6, r110\n"
    "  161: GOTO_IF_Z  r111, L15\n"
    "  162: ORI        r112, r110, -2147483648\n"
    "  163: SET_ALIAS  a6, r112\n"
    "  164: LABEL      L15\n"
    "  165: LABEL      L3\n"
    "  166: GET_ALIAS  r113, a6\n"
    "  167: ANDI       r114, r113, 128\n"
    "  168: GOTO_IF_NZ r114, L16\n"
    "  169: GET_ALIAS  r115, a2\n"
    "  170: VEXTRACT   r116, r115, 0\n"
    "  171: FCAST      r117, r10\n"
    "  172: VBUILD2    r118, r117, r116\n"
    "  173: SET_ALIAS  a2, r118\n"
    "  174: GOTO       L16\n"
    "  175: LABEL      L2\n"
    "  176: VBUILD2    r119, r10, r12\n"
    "  177: GET_ALIAS  r120, a6\n"
    "  178: FGETSTATE  r121\n"
    "  179: FCLEAREXC  r122, r121\n"
    "  180: FSETSTATE  r122\n"
    "  181: VFCAST     r123, r119\n"
    "  182: SET_ALIAS  a2, r123\n"
    "  183: VEXTRACT   r124, r119, 1\n"
    "  184: BITCAST    r125, r124\n"
    "  185: SGTUI      r126, r125, 0\n"
    "  186: SRLI       r127, r125, 31\n"
    "  187: BFEXT      r131, r125, 23, 8\n"
    "  188: SEQI       r128, r131, 0\n"
    "  189: SEQI       r129, r131, 255\n"
    "  190: SLLI       r132, r125, 9\n"
    "  191: SEQI       r130, r132, 0\n"
    "  192: AND        r133, r128, r130\n"
    "  193: XORI       r134, r130, 1\n"
    "  194: AND        r135, r129, r134\n"
    "  195: AND        r136, r128, r126\n"
    "  196: OR         r137, r136, r135\n"
    "  197: OR         r138, r133, r135\n"
    "  198: XORI       r139, r138, 1\n"
    "  199: XORI       r140, r127, 1\n"
    "  200: AND        r141, r127, r139\n"
    "  201: AND        r142, r140, r139\n"
    "  202: SLLI       r143, r137, 4\n"
    "  203: SLLI       r144, r141, 3\n"
    "  204: SLLI       r145, r142, 2\n"
    "  205: SLLI       r146, r133, 1\n"
    "  206: OR         r147, r143, r144\n"
    "  207: OR         r148, r145, r146\n"
    "  208: OR         r149, r147, r129\n"
    "  209: OR         r150, r149, r148\n"
    "  210: FTESTEXC   r151, r121, INEXACT\n"
    "  211: SLLI       r152, r151, 5\n"
    "  212: OR         r153, r150, r152\n"
    "  213: GET_ALIAS  r154, a6\n"
    "  214: BFINS      r155, r154, r153, 12, 7\n"
    "  215: SET_ALIAS  a6, r155\n"
    "  216: GOTO_IF_Z  r151, L18\n"
    "  217: GET_ALIAS  r156, a6\n"
    "  218: NOT        r157, r156\n"
    "  219: ORI        r158, r156, 33554432\n"
    "  220: ANDI       r159, r157, 33554432\n"
    "  221: SET_ALIAS  a6, r158\n"
    "  222: GOTO_IF_Z  r159, L19\n"
    "  223: ORI        r160, r158, -2147483648\n"
    "  224: SET_ALIAS  a6, r160\n"
    "  225: LABEL      L19\n"
    "  226: LABEL      L18\n"
    "  227: FTESTEXC   r161, r121, OVERFLOW\n"
    "  228: GOTO_IF_Z  r161, L20\n"
    "  229: GET_ALIAS  r162, a6\n"
    "  230: NOT        r163, r162\n"
    "  231: ORI        r164, r162, 268435456\n"
    "  232: ANDI       r165, r163, 268435456\n"
    "  233: SET_ALIAS  a6, r164\n"
    "  234: GOTO_IF_Z  r165, L21\n"
    "  235: ORI        r166, r164, -2147483648\n"
    "  236: SET_ALIAS  a6, r166\n"
    "  237: LABEL      L21\n"
    "  238: LABEL      L20\n"
    "  239: FTESTEXC   r167, r121, UNDERFLOW\n"
    "  240: GOTO_IF_Z  r167, L17\n"
    "  241: GET_ALIAS  r168, a6\n"
    "  242: NOT        r169, r168\n"
    "  243: ORI        r170, r168, 134217728\n"
    "  244: ANDI       r171, r169, 134217728\n"
    "  245: SET_ALIAS  a6, r170\n"
    "  246: GOTO_IF_Z  r171, L22\n"
    "  247: ORI        r172, r170, -2147483648\n"
    "  248: SET_ALIAS  a6, r172\n"
    "  249: LABEL      L22\n"
    "  250: LABEL      L17\n"
    "  251: LABEL      L16\n"
    "  252: LOAD_IMM   r173, 4\n"
    "  253: SET_ALIAS  a1, r173\n"
    "  254: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> 1,2\n"
    "Block 1: 0 --> [18,25] --> 2\n"
    "Block 2: 1,0 --> [26,29] --> 3,31\n"
    "Block 3: 2 --> [30,35] --> 4,17\n"
    "Block 4: 3 --> [36,40] --> 5,6\n"
    "Block 5: 4 --> [41,41] --> 6,13\n"
    "Block 6: 5,4 --> [42,47] --> 7,8\n"
    "Block 7: 6 --> [48,48] --> 8,13\n"
    "Block 8: 7,6 --> [49,54] --> 9,10\n"
    "Block 9: 8 --> [55,56] --> 10\n"
    "Block 10: 9,8 --> [57,59] --> 11,12\n"
    "Block 11: 10 --> [60,66] --> 29\n"
    "Block 12: 10 --> [67,76] --> 29\n"
    "Block 13: 5,7 --> [77,82] --> 14,15\n"
    "Block 14: 13 --> [83,84] --> 15\n"
    "Block 15: 14,13 --> [85,87] --> 16,17\n"
    "Block 16: 15 --> [88,94] --> 29\n"
    "Block 17: 3,15 --> [95,131] --> 18,21\n"
    "Block 18: 17 --> [132,137] --> 19,20\n"
    "Block 19: 18 --> [138,139] --> 20\n"
    "Block 20: 19,18 --> [140,140] --> 21\n"
    "Block 21: 20,17 --> [141,143] --> 22,25\n"
    "Block 22: 21 --> [144,149] --> 23,24\n"
    "Block 23: 22 --> [150,151] --> 24\n"
    "Block 24: 23,22 --> [152,152] --> 25\n"
    "Block 25: 24,21 --> [153,155] --> 26,29\n"
    "Block 26: 25 --> [156,161] --> 27,28\n"
    "Block 27: 26 --> [162,163] --> 28\n"
    "Block 28: 27,26 --> [164,164] --> 29\n"
    "Block 29: 28,11,12,16,25 --> [165,168] --> 30,44\n"
    "Block 30: 29 --> [169,174] --> 44\n"
    "Block 31: 2 --> [175,216] --> 32,35\n"
    "Block 32: 31 --> [217,222] --> 33,34\n"
    "Block 33: 32 --> [223,224] --> 34\n"
    "Block 34: 33,32 --> [225,225] --> 35\n"
    "Block 35: 34,31 --> [226,228] --> 36,39\n"
    "Block 36: 35 --> [229,234] --> 37,38\n"
    "Block 37: 36 --> [235,236] --> 38\n"
    "Block 38: 37,36 --> [237,237] --> 39\n"
    "Block 39: 38,35 --> [238,240] --> 40,43\n"
    "Block 40: 39 --> [241,246] --> 41,42\n"
    "Block 41: 40 --> [247,248] --> 42\n"
    "Block 42: 41,40 --> [249,249] --> 43\n"
    "Block 43: 42,39 --> [250,250] --> 44\n"
    "Block 44: 43,29,30 --> [251,254] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
