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
    0xC0,0x23,0x00,0x00,  // lfs f3,0(r3)
    0x10,0x22,0x18,0x2A,  // ps_add f1,f2,f3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, 0(r5)\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: GET_ALIAS  r8, a5\n"
    "    8: FADD       r9, r7, r8\n"
    "    9: VFCVT      r10, r9\n"
    "   10: LOAD_IMM   r11, 0x1000000\n"
    "   11: VEXTRACT   r12, r10, 1\n"
    "   12: BITCAST    r13, r12\n"
    "   13: SLLI       r14, r13, 1\n"
    "   14: SEQ        r15, r14, r11\n"
    "   15: GOTO_IF_NZ r15, L1\n"
    "   16: VEXTRACT   r16, r10, 0\n"
    "   17: BITCAST    r17, r16\n"
    "   18: SLLI       r18, r17, 1\n"
    "   19: SEQ        r19, r18, r11\n"
    "   20: GOTO_IF_Z  r19, L2\n"
    "   21: LABEL      L1\n"
    "   22: FGETSTATE  r20\n"
    "   23: FSETROUND  r21, r20, TRUNC\n"
    "   24: FSETSTATE  r21\n"
    "   25: FADD       r22, r7, r8\n"
    "   26: VFCVT      r23, r22\n"
    "   27: FGETSTATE  r24\n"
    "   28: FCOPYROUND r25, r24, r20\n"
    "   29: FSETSTATE  r25\n"
    "   30: LABEL      L2\n"
    "   31: FGETSTATE  r26\n"
    "   32: FTESTEXC   r27, r26, INVALID\n"
    "   33: GOTO_IF_Z  r27, L3\n"
    "   34: FCLEAREXC  r28, r26\n"
    "   35: FSETSTATE  r28\n"
    "   36: VEXTRACT   r29, r7, 0\n"
    "   37: VEXTRACT   r30, r8, 0\n"
    "   38: FADD       r31, r29, r30\n"
    "   39: FCVT       r32, r31\n"
    "   40: GET_ALIAS  r33, a6\n"
    "   41: FGETSTATE  r34\n"
    "   42: FCLEAREXC  r35, r34\n"
    "   43: FSETSTATE  r35\n"
    "   44: FTESTEXC   r36, r34, INVALID\n"
    "   45: GOTO_IF_Z  r36, L5\n"
    "   46: BITCAST    r37, r29\n"
    "   47: SLLI       r38, r37, 13\n"
    "   48: BFEXT      r39, r37, 51, 12\n"
    "   49: SEQI       r40, r39, 4094\n"
    "   50: GOTO_IF_Z  r40, L7\n"
    "   51: GOTO_IF_NZ r38, L6\n"
    "   52: LABEL      L7\n"
    "   53: BITCAST    r41, r30\n"
    "   54: SLLI       r42, r41, 13\n"
    "   55: BFEXT      r43, r41, 51, 12\n"
    "   56: SEQI       r44, r43, 4094\n"
    "   57: GOTO_IF_Z  r44, L8\n"
    "   58: GOTO_IF_NZ r42, L6\n"
    "   59: LABEL      L8\n"
    "   60: NOT        r45, r33\n"
    "   61: ORI        r46, r33, 8388608\n"
    "   62: ANDI       r47, r45, 8388608\n"
    "   63: SET_ALIAS  a6, r46\n"
    "   64: GOTO_IF_Z  r47, L9\n"
    "   65: ORI        r48, r46, -2147483648\n"
    "   66: SET_ALIAS  a6, r48\n"
    "   67: LABEL      L9\n"
    "   68: ANDI       r49, r33, 128\n"
    "   69: GOTO_IF_Z  r49, L10\n"
    "   70: GET_ALIAS  r50, a6\n"
    "   71: BFEXT      r51, r50, 12, 7\n"
    "   72: ANDI       r52, r51, 31\n"
    "   73: GET_ALIAS  r53, a6\n"
    "   74: BFINS      r54, r53, r52, 12, 7\n"
    "   75: SET_ALIAS  a6, r54\n"
    "   76: GOTO       L4\n"
    "   77: LABEL      L10\n"
    "   78: LOAD_IMM   r55, nan(0x400000)\n"
    "   79: FCVT       r56, r55\n"
    "   80: VBROADCAST r57, r56\n"
    "   81: SET_ALIAS  a3, r57\n"
    "   82: LOAD_IMM   r58, 17\n"
    "   83: GET_ALIAS  r59, a6\n"
    "   84: BFINS      r60, r59, r58, 12, 7\n"
    "   85: SET_ALIAS  a6, r60\n"
    "   86: GOTO       L4\n"
    "   87: LABEL      L6\n"
    "   88: NOT        r61, r33\n"
    "   89: ORI        r62, r33, 16777216\n"
    "   90: ANDI       r63, r61, 16777216\n"
    "   91: SET_ALIAS  a6, r62\n"
    "   92: GOTO_IF_Z  r63, L11\n"
    "   93: ORI        r64, r62, -2147483648\n"
    "   94: SET_ALIAS  a6, r64\n"
    "   95: LABEL      L11\n"
    "   96: ANDI       r65, r33, 128\n"
    "   97: GOTO_IF_Z  r65, L5\n"
    "   98: GET_ALIAS  r66, a6\n"
    "   99: BFEXT      r67, r66, 12, 7\n"
    "  100: ANDI       r68, r67, 31\n"
    "  101: GET_ALIAS  r69, a6\n"
    "  102: BFINS      r70, r69, r68, 12, 7\n"
    "  103: SET_ALIAS  a6, r70\n"
    "  104: GOTO       L4\n"
    "  105: LABEL      L5\n"
    "  106: FCVT       r71, r32\n"
    "  107: VBROADCAST r72, r71\n"
    "  108: SET_ALIAS  a3, r72\n"
    "  109: BITCAST    r73, r32\n"
    "  110: SGTUI      r74, r73, 0\n"
    "  111: SRLI       r75, r73, 31\n"
    "  112: BFEXT      r79, r73, 23, 8\n"
    "  113: SEQI       r76, r79, 0\n"
    "  114: SEQI       r77, r79, 255\n"
    "  115: SLLI       r80, r73, 9\n"
    "  116: SEQI       r78, r80, 0\n"
    "  117: AND        r81, r76, r78\n"
    "  118: XORI       r82, r78, 1\n"
    "  119: AND        r83, r77, r82\n"
    "  120: AND        r84, r76, r74\n"
    "  121: OR         r85, r84, r83\n"
    "  122: OR         r86, r81, r83\n"
    "  123: XORI       r87, r86, 1\n"
    "  124: XORI       r88, r75, 1\n"
    "  125: AND        r89, r75, r87\n"
    "  126: AND        r90, r88, r87\n"
    "  127: SLLI       r91, r85, 4\n"
    "  128: SLLI       r92, r89, 3\n"
    "  129: SLLI       r93, r90, 2\n"
    "  130: SLLI       r94, r81, 1\n"
    "  131: OR         r95, r91, r92\n"
    "  132: OR         r96, r93, r94\n"
    "  133: OR         r97, r95, r77\n"
    "  134: OR         r98, r97, r96\n"
    "  135: FTESTEXC   r99, r34, INEXACT\n"
    "  136: SLLI       r100, r99, 5\n"
    "  137: OR         r101, r98, r100\n"
    "  138: GET_ALIAS  r102, a6\n"
    "  139: BFINS      r103, r102, r101, 12, 7\n"
    "  140: SET_ALIAS  a6, r103\n"
    "  141: GOTO_IF_Z  r99, L12\n"
    "  142: GET_ALIAS  r104, a6\n"
    "  143: NOT        r105, r104\n"
    "  144: ORI        r106, r104, 33554432\n"
    "  145: ANDI       r107, r105, 33554432\n"
    "  146: SET_ALIAS  a6, r106\n"
    "  147: GOTO_IF_Z  r107, L13\n"
    "  148: ORI        r108, r106, -2147483648\n"
    "  149: SET_ALIAS  a6, r108\n"
    "  150: LABEL      L13\n"
    "  151: LABEL      L12\n"
    "  152: FTESTEXC   r109, r34, OVERFLOW\n"
    "  153: GOTO_IF_Z  r109, L14\n"
    "  154: GET_ALIAS  r110, a6\n"
    "  155: NOT        r111, r110\n"
    "  156: ORI        r112, r110, 268435456\n"
    "  157: ANDI       r113, r111, 268435456\n"
    "  158: SET_ALIAS  a6, r112\n"
    "  159: GOTO_IF_Z  r113, L15\n"
    "  160: ORI        r114, r112, -2147483648\n"
    "  161: SET_ALIAS  a6, r114\n"
    "  162: LABEL      L15\n"
    "  163: LABEL      L14\n"
    "  164: FTESTEXC   r115, r34, UNDERFLOW\n"
    "  165: GOTO_IF_Z  r115, L4\n"
    "  166: GET_ALIAS  r116, a6\n"
    "  167: NOT        r117, r116\n"
    "  168: ORI        r118, r116, 134217728\n"
    "  169: ANDI       r119, r117, 134217728\n"
    "  170: SET_ALIAS  a6, r118\n"
    "  171: GOTO_IF_Z  r119, L16\n"
    "  172: ORI        r120, r118, -2147483648\n"
    "  173: SET_ALIAS  a6, r120\n"
    "  174: LABEL      L16\n"
    "  175: LABEL      L4\n"
    "  176: GET_ALIAS  r121, a3\n"
    "  177: VEXTRACT   r122, r121, 0\n"
    "  178: GET_ALIAS  r123, a6\n"
    "  179: BFEXT      r124, r123, 12, 7\n"
    "  180: VEXTRACT   r125, r7, 1\n"
    "  181: VEXTRACT   r126, r8, 1\n"
    "  182: FADD       r127, r125, r126\n"
    "  183: FCVT       r128, r127\n"
    "  184: GET_ALIAS  r129, a6\n"
    "  185: FGETSTATE  r130\n"
    "  186: FCLEAREXC  r131, r130\n"
    "  187: FSETSTATE  r131\n"
    "  188: FTESTEXC   r132, r130, INVALID\n"
    "  189: GOTO_IF_Z  r132, L18\n"
    "  190: BITCAST    r133, r125\n"
    "  191: SLLI       r134, r133, 13\n"
    "  192: BFEXT      r135, r133, 51, 12\n"
    "  193: SEQI       r136, r135, 4094\n"
    "  194: GOTO_IF_Z  r136, L20\n"
    "  195: GOTO_IF_NZ r134, L19\n"
    "  196: LABEL      L20\n"
    "  197: BITCAST    r137, r126\n"
    "  198: SLLI       r138, r137, 13\n"
    "  199: BFEXT      r139, r137, 51, 12\n"
    "  200: SEQI       r140, r139, 4094\n"
    "  201: GOTO_IF_Z  r140, L21\n"
    "  202: GOTO_IF_NZ r138, L19\n"
    "  203: LABEL      L21\n"
    "  204: NOT        r141, r129\n"
    "  205: ORI        r142, r129, 8388608\n"
    "  206: ANDI       r143, r141, 8388608\n"
    "  207: SET_ALIAS  a6, r142\n"
    "  208: GOTO_IF_Z  r143, L22\n"
    "  209: ORI        r144, r142, -2147483648\n"
    "  210: SET_ALIAS  a6, r144\n"
    "  211: LABEL      L22\n"
    "  212: ANDI       r145, r129, 128\n"
    "  213: GOTO_IF_Z  r145, L23\n"
    "  214: GET_ALIAS  r146, a6\n"
    "  215: BFEXT      r147, r146, 12, 7\n"
    "  216: ANDI       r148, r147, 31\n"
    "  217: GET_ALIAS  r149, a6\n"
    "  218: BFINS      r150, r149, r148, 12, 7\n"
    "  219: SET_ALIAS  a6, r150\n"
    "  220: GOTO       L17\n"
    "  221: LABEL      L23\n"
    "  222: LOAD_IMM   r151, nan(0x400000)\n"
    "  223: FCVT       r152, r151\n"
    "  224: VBROADCAST r153, r152\n"
    "  225: SET_ALIAS  a3, r153\n"
    "  226: LOAD_IMM   r154, 17\n"
    "  227: GET_ALIAS  r155, a6\n"
    "  228: BFINS      r156, r155, r154, 12, 7\n"
    "  229: SET_ALIAS  a6, r156\n"
    "  230: GOTO       L17\n"
    "  231: LABEL      L19\n"
    "  232: NOT        r157, r129\n"
    "  233: ORI        r158, r129, 16777216\n"
    "  234: ANDI       r159, r157, 16777216\n"
    "  235: SET_ALIAS  a6, r158\n"
    "  236: GOTO_IF_Z  r159, L24\n"
    "  237: ORI        r160, r158, -2147483648\n"
    "  238: SET_ALIAS  a6, r160\n"
    "  239: LABEL      L24\n"
    "  240: ANDI       r161, r129, 128\n"
    "  241: GOTO_IF_Z  r161, L18\n"
    "  242: GET_ALIAS  r162, a6\n"
    "  243: BFEXT      r163, r162, 12, 7\n"
    "  244: ANDI       r164, r163, 31\n"
    "  245: GET_ALIAS  r165, a6\n"
    "  246: BFINS      r166, r165, r164, 12, 7\n"
    "  247: SET_ALIAS  a6, r166\n"
    "  248: GOTO       L17\n"
    "  249: LABEL      L18\n"
    "  250: FCVT       r167, r128\n"
    "  251: VBROADCAST r168, r167\n"
    "  252: SET_ALIAS  a3, r168\n"
    "  253: BITCAST    r169, r128\n"
    "  254: SGTUI      r170, r169, 0\n"
    "  255: SRLI       r171, r169, 31\n"
    "  256: BFEXT      r175, r169, 23, 8\n"
    "  257: SEQI       r172, r175, 0\n"
    "  258: SEQI       r173, r175, 255\n"
    "  259: SLLI       r176, r169, 9\n"
    "  260: SEQI       r174, r176, 0\n"
    "  261: AND        r177, r172, r174\n"
    "  262: XORI       r178, r174, 1\n"
    "  263: AND        r179, r173, r178\n"
    "  264: AND        r180, r172, r170\n"
    "  265: OR         r181, r180, r179\n"
    "  266: OR         r182, r177, r179\n"
    "  267: XORI       r183, r182, 1\n"
    "  268: XORI       r184, r171, 1\n"
    "  269: AND        r185, r171, r183\n"
    "  270: AND        r186, r184, r183\n"
    "  271: SLLI       r187, r181, 4\n"
    "  272: SLLI       r188, r185, 3\n"
    "  273: SLLI       r189, r186, 2\n"
    "  274: SLLI       r190, r177, 1\n"
    "  275: OR         r191, r187, r188\n"
    "  276: OR         r192, r189, r190\n"
    "  277: OR         r193, r191, r173\n"
    "  278: OR         r194, r193, r192\n"
    "  279: FTESTEXC   r195, r130, INEXACT\n"
    "  280: SLLI       r196, r195, 5\n"
    "  281: OR         r197, r194, r196\n"
    "  282: GET_ALIAS  r198, a6\n"
    "  283: BFINS      r199, r198, r197, 12, 7\n"
    "  284: SET_ALIAS  a6, r199\n"
    "  285: GOTO_IF_Z  r195, L25\n"
    "  286: GET_ALIAS  r200, a6\n"
    "  287: NOT        r201, r200\n"
    "  288: ORI        r202, r200, 33554432\n"
    "  289: ANDI       r203, r201, 33554432\n"
    "  290: SET_ALIAS  a6, r202\n"
    "  291: GOTO_IF_Z  r203, L26\n"
    "  292: ORI        r204, r202, -2147483648\n"
    "  293: SET_ALIAS  a6, r204\n"
    "  294: LABEL      L26\n"
    "  295: LABEL      L25\n"
    "  296: FTESTEXC   r205, r130, OVERFLOW\n"
    "  297: GOTO_IF_Z  r205, L27\n"
    "  298: GET_ALIAS  r206, a6\n"
    "  299: NOT        r207, r206\n"
    "  300: ORI        r208, r206, 268435456\n"
    "  301: ANDI       r209, r207, 268435456\n"
    "  302: SET_ALIAS  a6, r208\n"
    "  303: GOTO_IF_Z  r209, L28\n"
    "  304: ORI        r210, r208, -2147483648\n"
    "  305: SET_ALIAS  a6, r210\n"
    "  306: LABEL      L28\n"
    "  307: LABEL      L27\n"
    "  308: FTESTEXC   r211, r130, UNDERFLOW\n"
    "  309: GOTO_IF_Z  r211, L17\n"
    "  310: GET_ALIAS  r212, a6\n"
    "  311: NOT        r213, r212\n"
    "  312: ORI        r214, r212, 134217728\n"
    "  313: ANDI       r215, r213, 134217728\n"
    "  314: SET_ALIAS  a6, r214\n"
    "  315: GOTO_IF_Z  r215, L29\n"
    "  316: ORI        r216, r214, -2147483648\n"
    "  317: SET_ALIAS  a6, r216\n"
    "  318: LABEL      L29\n"
    "  319: LABEL      L17\n"
    "  320: GET_ALIAS  r217, a3\n"
    "  321: VEXTRACT   r218, r217, 0\n"
    "  322: GET_ALIAS  r219, a6\n"
    "  323: BFEXT      r220, r219, 12, 7\n"
    "  324: ANDI       r221, r220, 32\n"
    "  325: OR         r222, r124, r221\n"
    "  326: GET_ALIAS  r223, a6\n"
    "  327: BFINS      r224, r223, r222, 12, 7\n"
    "  328: SET_ALIAS  a6, r224\n"
    "  329: GET_ALIAS  r225, a6\n"
    "  330: ANDI       r226, r225, 128\n"
    "  331: GOTO_IF_Z  r226, L30\n"
    "  332: FGETSTATE  r227\n"
    "  333: FCMP       r228, r6, r6, UN\n"
    "  334: FCVT       r229, r6\n"
    "  335: SET_ALIAS  a7, r229\n"
    "  336: GOTO_IF_Z  r228, L31\n"
    "  337: BITCAST    r230, r6\n"
    "  338: ANDI       r231, r230, 4194304\n"
    "  339: GOTO_IF_NZ r231, L31\n"
    "  340: BITCAST    r232, r229\n"
    "  341: LOAD_IMM   r233, 0x8000000000000\n"
    "  342: XOR        r234, r232, r233\n"
    "  343: BITCAST    r235, r234\n"
    "  344: SET_ALIAS  a7, r235\n"
    "  345: LABEL      L31\n"
    "  346: GET_ALIAS  r236, a7\n"
    "  347: VBROADCAST r237, r236\n"
    "  348: FSETSTATE  r227\n"
    "  349: SET_ALIAS  a3, r237\n"
    "  350: GOTO       L32\n"
    "  351: LABEL      L30\n"
    "  352: VBUILD2    r238, r122, r218\n"
    "  353: SET_ALIAS  a3, r238\n"
    "  354: GOTO       L32\n"
    "  355: LABEL      L3\n"
    "  356: GET_ALIAS  r239, a6\n"
    "  357: FGETSTATE  r240\n"
    "  358: FCLEAREXC  r241, r240\n"
    "  359: FSETSTATE  r241\n"
    "  360: VFCVT      r242, r10\n"
    "  361: SET_ALIAS  a3, r242\n"
    "  362: VEXTRACT   r243, r10, 0\n"
    "  363: BITCAST    r244, r243\n"
    "  364: SGTUI      r245, r244, 0\n"
    "  365: SRLI       r246, r244, 31\n"
    "  366: BFEXT      r250, r244, 23, 8\n"
    "  367: SEQI       r247, r250, 0\n"
    "  368: SEQI       r248, r250, 255\n"
    "  369: SLLI       r251, r244, 9\n"
    "  370: SEQI       r249, r251, 0\n"
    "  371: AND        r252, r247, r249\n"
    "  372: XORI       r253, r249, 1\n"
    "  373: AND        r254, r248, r253\n"
    "  374: AND        r255, r247, r245\n"
    "  375: OR         r256, r255, r254\n"
    "  376: OR         r257, r252, r254\n"
    "  377: XORI       r258, r257, 1\n"
    "  378: XORI       r259, r246, 1\n"
    "  379: AND        r260, r246, r258\n"
    "  380: AND        r261, r259, r258\n"
    "  381: SLLI       r262, r256, 4\n"
    "  382: SLLI       r263, r260, 3\n"
    "  383: SLLI       r264, r261, 2\n"
    "  384: SLLI       r265, r252, 1\n"
    "  385: OR         r266, r262, r263\n"
    "  386: OR         r267, r264, r265\n"
    "  387: OR         r268, r266, r248\n"
    "  388: OR         r269, r268, r267\n"
    "  389: FTESTEXC   r270, r240, INEXACT\n"
    "  390: SLLI       r271, r270, 5\n"
    "  391: OR         r272, r269, r271\n"
    "  392: GET_ALIAS  r273, a6\n"
    "  393: BFINS      r274, r273, r272, 12, 7\n"
    "  394: SET_ALIAS  a6, r274\n"
    "  395: GOTO_IF_Z  r270, L34\n"
    "  396: GET_ALIAS  r275, a6\n"
    "  397: NOT        r276, r275\n"
    "  398: ORI        r277, r275, 33554432\n"
    "  399: ANDI       r278, r276, 33554432\n"
    "  400: SET_ALIAS  a6, r277\n"
    "  401: GOTO_IF_Z  r278, L35\n"
    "  402: ORI        r279, r277, -2147483648\n"
    "  403: SET_ALIAS  a6, r279\n"
    "  404: LABEL      L35\n"
    "  405: LABEL      L34\n"
    "  406: FTESTEXC   r280, r240, OVERFLOW\n"
    "  407: GOTO_IF_Z  r280, L36\n"
    "  408: GET_ALIAS  r281, a6\n"
    "  409: NOT        r282, r281\n"
    "  410: ORI        r283, r281, 268435456\n"
    "  411: ANDI       r284, r282, 268435456\n"
    "  412: SET_ALIAS  a6, r283\n"
    "  413: GOTO_IF_Z  r284, L37\n"
    "  414: ORI        r285, r283, -2147483648\n"
    "  415: SET_ALIAS  a6, r285\n"
    "  416: LABEL      L37\n"
    "  417: LABEL      L36\n"
    "  418: FTESTEXC   r286, r240, UNDERFLOW\n"
    "  419: GOTO_IF_Z  r286, L33\n"
    "  420: GET_ALIAS  r287, a6\n"
    "  421: NOT        r288, r287\n"
    "  422: ORI        r289, r287, 134217728\n"
    "  423: ANDI       r290, r288, 134217728\n"
    "  424: SET_ALIAS  a6, r289\n"
    "  425: GOTO_IF_Z  r290, L38\n"
    "  426: ORI        r291, r289, -2147483648\n"
    "  427: SET_ALIAS  a6, r291\n"
    "  428: LABEL      L38\n"
    "  429: LABEL      L33\n"
    "  430: LABEL      L32\n"
    "  431: LOAD_IMM   r292, 8\n"
    "  432: SET_ALIAS  a1, r292\n"
    "  433: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2] @ 416(r1)\n"
    "Alias 5: float64[2] @ 432(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "Alias 7: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,15] --> 1,2\n"
    "Block 1: 0 --> [16,20] --> 2,3\n"
    "Block 2: 1,0 --> [21,29] --> 3\n"
    "Block 3: 2,1 --> [30,33] --> 4,62\n"
    "Block 4: 3 --> [34,45] --> 5,18\n"
    "Block 5: 4 --> [46,50] --> 6,7\n"
    "Block 6: 5 --> [51,51] --> 7,14\n"
    "Block 7: 6,5 --> [52,57] --> 8,9\n"
    "Block 8: 7 --> [58,58] --> 9,14\n"
    "Block 9: 8,7 --> [59,64] --> 10,11\n"
    "Block 10: 9 --> [65,66] --> 11\n"
    "Block 11: 10,9 --> [67,69] --> 12,13\n"
    "Block 12: 11 --> [70,76] --> 30\n"
    "Block 13: 11 --> [77,86] --> 30\n"
    "Block 14: 6,8 --> [87,92] --> 15,16\n"
    "Block 15: 14 --> [93,94] --> 16\n"
    "Block 16: 15,14 --> [95,97] --> 17,18\n"
    "Block 17: 16 --> [98,104] --> 30\n"
    "Block 18: 4,16 --> [105,141] --> 19,22\n"
    "Block 19: 18 --> [142,147] --> 20,21\n"
    "Block 20: 19 --> [148,149] --> 21\n"
    "Block 21: 20,19 --> [150,150] --> 22\n"
    "Block 22: 21,18 --> [151,153] --> 23,26\n"
    "Block 23: 22 --> [154,159] --> 24,25\n"
    "Block 24: 23 --> [160,161] --> 25\n"
    "Block 25: 24,23 --> [162,162] --> 26\n"
    "Block 26: 25,22 --> [163,165] --> 27,30\n"
    "Block 27: 26 --> [166,171] --> 28,29\n"
    "Block 28: 27 --> [172,173] --> 29\n"
    "Block 29: 28,27 --> [174,174] --> 30\n"
    "Block 30: 29,12,13,17,26 --> [175,189] --> 31,44\n"
    "Block 31: 30 --> [190,194] --> 32,33\n"
    "Block 32: 31 --> [195,195] --> 33,40\n"
    "Block 33: 32,31 --> [196,201] --> 34,35\n"
    "Block 34: 33 --> [202,202] --> 35,40\n"
    "Block 35: 34,33 --> [203,208] --> 36,37\n"
    "Block 36: 35 --> [209,210] --> 37\n"
    "Block 37: 36,35 --> [211,213] --> 38,39\n"
    "Block 38: 37 --> [214,220] --> 56\n"
    "Block 39: 37 --> [221,230] --> 56\n"
    "Block 40: 32,34 --> [231,236] --> 41,42\n"
    "Block 41: 40 --> [237,238] --> 42\n"
    "Block 42: 41,40 --> [239,241] --> 43,44\n"
    "Block 43: 42 --> [242,248] --> 56\n"
    "Block 44: 30,42 --> [249,285] --> 45,48\n"
    "Block 45: 44 --> [286,291] --> 46,47\n"
    "Block 46: 45 --> [292,293] --> 47\n"
    "Block 47: 46,45 --> [294,294] --> 48\n"
    "Block 48: 47,44 --> [295,297] --> 49,52\n"
    "Block 49: 48 --> [298,303] --> 50,51\n"
    "Block 50: 49 --> [304,305] --> 51\n"
    "Block 51: 50,49 --> [306,306] --> 52\n"
    "Block 52: 51,48 --> [307,309] --> 53,56\n"
    "Block 53: 52 --> [310,315] --> 54,55\n"
    "Block 54: 53 --> [316,317] --> 55\n"
    "Block 55: 54,53 --> [318,318] --> 56\n"
    "Block 56: 55,38,39,43,52 --> [319,331] --> 57,61\n"
    "Block 57: 56 --> [332,336] --> 58,60\n"
    "Block 58: 57 --> [337,339] --> 59,60\n"
    "Block 59: 58 --> [340,344] --> 60\n"
    "Block 60: 59,57,58 --> [345,350] --> 75\n"
    "Block 61: 56 --> [351,354] --> 75\n"
    "Block 62: 3 --> [355,395] --> 63,66\n"
    "Block 63: 62 --> [396,401] --> 64,65\n"
    "Block 64: 63 --> [402,403] --> 65\n"
    "Block 65: 64,63 --> [404,404] --> 66\n"
    "Block 66: 65,62 --> [405,407] --> 67,70\n"
    "Block 67: 66 --> [408,413] --> 68,69\n"
    "Block 68: 67 --> [414,415] --> 69\n"
    "Block 69: 68,67 --> [416,416] --> 70\n"
    "Block 70: 69,66 --> [417,419] --> 71,74\n"
    "Block 71: 70 --> [420,425] --> 72,73\n"
    "Block 72: 71 --> [426,427] --> 73\n"
    "Block 73: 72,71 --> [428,428] --> 74\n"
    "Block 74: 73,70 --> [429,429] --> 75\n"
    "Block 75: 74,60,61 --> [430,433] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
