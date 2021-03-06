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
    0x10,0x22,0x20,0xFA,  // ps_madd f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_FAST_FMADDS;
static const unsigned int common_opt = BINREC_OPT_NATIVE_IEEE_NAN;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: VEXTRACT   r6, r3, 0\n"
    "    6: VEXTRACT   r7, r3, 1\n"
    "    7: VEXTRACT   r8, r4, 0\n"
    "    8: VEXTRACT   r9, r4, 1\n"
    "    9: VEXTRACT   r10, r5, 0\n"
    "   10: VEXTRACT   r11, r5, 1\n"
    "   11: SET_ALIAS  a7, r6\n"
    "   12: SET_ALIAS  a8, r8\n"
    "   13: BITCAST    r12, r8\n"
    "   14: BITCAST    r13, r6\n"
    "   15: BFEXT      r14, r12, 0, 52\n"
    "   16: GOTO_IF_Z  r14, L1\n"
    "   17: BFEXT      r15, r13, 52, 11\n"
    "   18: BFEXT      r16, r12, 52, 11\n"
    "   19: SEQI       r17, r15, 2047\n"
    "   20: GOTO_IF_NZ r17, L1\n"
    "   21: SEQI       r18, r16, 2047\n"
    "   22: GOTO_IF_NZ r18, L1\n"
    "   23: GOTO_IF_NZ r16, L2\n"
    "   24: CLZ        r19, r14\n"
    "   25: ADDI       r20, r19, -11\n"
    "   26: SUB        r21, r15, r20\n"
    "   27: SGTSI      r22, r21, 0\n"
    "   28: GOTO_IF_Z  r22, L1\n"
    "   29: LOAD_IMM   r23, 0x8000000000000000\n"
    "   30: AND        r24, r12, r23\n"
    "   31: SLL        r25, r14, r20\n"
    "   32: BFINS      r26, r13, r21, 52, 11\n"
    "   33: OR         r27, r24, r25\n"
    "   34: BITCAST    r28, r26\n"
    "   35: BITCAST    r29, r27\n"
    "   36: SET_ALIAS  a7, r28\n"
    "   37: SET_ALIAS  a8, r29\n"
    "   38: LABEL      L2\n"
    "   39: GET_ALIAS  r30, a8\n"
    "   40: BITCAST    r31, r30\n"
    "   41: ANDI       r32, r31, -134217728\n"
    "   42: ANDI       r33, r31, 134217728\n"
    "   43: ADD        r34, r32, r33\n"
    "   44: BITCAST    r35, r34\n"
    "   45: SET_ALIAS  a8, r35\n"
    "   46: BFEXT      r36, r34, 52, 11\n"
    "   47: SEQI       r37, r36, 2047\n"
    "   48: GOTO_IF_Z  r37, L1\n"
    "   49: LOAD_IMM   r38, 0x10000000000000\n"
    "   50: SUB        r39, r34, r38\n"
    "   51: BITCAST    r40, r39\n"
    "   52: SET_ALIAS  a8, r40\n"
    "   53: BFEXT      r41, r13, 52, 11\n"
    "   54: GOTO_IF_Z  r41, L3\n"
    "   55: SGTUI      r42, r41, 2045\n"
    "   56: GOTO_IF_NZ r42, L1\n"
    "   57: ADD        r43, r13, r38\n"
    "   58: BITCAST    r44, r43\n"
    "   59: SET_ALIAS  a7, r44\n"
    "   60: GOTO       L1\n"
    "   61: LABEL      L3\n"
    "   62: SLLI       r45, r13, 1\n"
    "   63: BFINS      r46, r13, r45, 0, 63\n"
    "   64: BITCAST    r47, r46\n"
    "   65: SET_ALIAS  a7, r47\n"
    "   66: LABEL      L1\n"
    "   67: GET_ALIAS  r48, a7\n"
    "   68: GET_ALIAS  r49, a8\n"
    "   69: GET_ALIAS  r50, a2\n"
    "   70: FMADD      r51, r48, r49, r10\n"
    "   71: FGETSTATE  r52\n"
    "   72: FTESTEXC   r53, r52, INVALID\n"
    "   73: FCVT       r54, r51\n"
    "   74: LOAD_IMM   r55, 0x1000000\n"
    "   75: BITCAST    r56, r54\n"
    "   76: SLLI       r57, r56, 1\n"
    "   77: SEQ        r58, r57, r55\n"
    "   78: GOTO_IF_Z  r58, L4\n"
    "   79: FGETSTATE  r59\n"
    "   80: FSETROUND  r60, r59, TRUNC\n"
    "   81: FSETSTATE  r60\n"
    "   82: FMADD      r61, r48, r49, r10\n"
    "   83: FCVT       r62, r61\n"
    "   84: FGETSTATE  r63\n"
    "   85: FCOPYROUND r64, r63, r59\n"
    "   86: FSETSTATE  r64\n"
    "   87: LABEL      L4\n"
    "   88: GET_ALIAS  r65, a6\n"
    "   89: FGETSTATE  r66\n"
    "   90: FCLEAREXC  r67, r66\n"
    "   91: FSETSTATE  r67\n"
    "   92: FTESTEXC   r68, r66, INVALID\n"
    "   93: GOTO_IF_Z  r68, L6\n"
    "   94: BITCAST    r69, r48\n"
    "   95: SLLI       r70, r69, 13\n"
    "   96: BFEXT      r71, r69, 51, 12\n"
    "   97: SEQI       r72, r71, 4094\n"
    "   98: GOTO_IF_Z  r72, L8\n"
    "   99: GOTO_IF_NZ r70, L7\n"
    "  100: LABEL      L8\n"
    "  101: BITCAST    r73, r49\n"
    "  102: SLLI       r74, r73, 13\n"
    "  103: BFEXT      r75, r73, 51, 12\n"
    "  104: SEQI       r76, r75, 4094\n"
    "  105: GOTO_IF_Z  r76, L9\n"
    "  106: GOTO_IF_NZ r74, L7\n"
    "  107: LABEL      L9\n"
    "  108: BITCAST    r77, r10\n"
    "  109: SLLI       r78, r77, 13\n"
    "  110: BFEXT      r79, r77, 51, 12\n"
    "  111: SEQI       r80, r79, 4094\n"
    "  112: GOTO_IF_Z  r80, L11\n"
    "  113: GOTO_IF_NZ r78, L10\n"
    "  114: LABEL      L11\n"
    "  115: BITCAST    r81, r48\n"
    "  116: BITCAST    r82, r49\n"
    "  117: SLLI       r83, r81, 1\n"
    "  118: SLLI       r84, r82, 1\n"
    "  119: LOAD_IMM   r85, 0xFFE0000000000000\n"
    "  120: SEQ        r86, r83, r85\n"
    "  121: GOTO_IF_Z  r86, L12\n"
    "  122: GOTO_IF_Z  r84, L13\n"
    "  123: GOTO       L14\n"
    "  124: LABEL      L12\n"
    "  125: GOTO_IF_NZ r83, L14\n"
    "  126: LABEL      L13\n"
    "  127: NOT        r87, r65\n"
    "  128: ORI        r88, r65, 1048576\n"
    "  129: ANDI       r89, r87, 1048576\n"
    "  130: SET_ALIAS  a6, r88\n"
    "  131: GOTO_IF_Z  r89, L15\n"
    "  132: ORI        r90, r88, -2147483648\n"
    "  133: SET_ALIAS  a6, r90\n"
    "  134: LABEL      L15\n"
    "  135: GOTO       L16\n"
    "  136: LABEL      L14\n"
    "  137: NOT        r91, r65\n"
    "  138: ORI        r92, r65, 8388608\n"
    "  139: ANDI       r93, r91, 8388608\n"
    "  140: SET_ALIAS  a6, r92\n"
    "  141: GOTO_IF_Z  r93, L17\n"
    "  142: ORI        r94, r92, -2147483648\n"
    "  143: SET_ALIAS  a6, r94\n"
    "  144: LABEL      L17\n"
    "  145: GOTO       L16\n"
    "  146: LABEL      L10\n"
    "  147: FMUL       r95, r48, r49\n"
    "  148: FGETSTATE  r96\n"
    "  149: FSETSTATE  r67\n"
    "  150: FTESTEXC   r97, r96, INVALID\n"
    "  151: GOTO_IF_Z  r97, L7\n"
    "  152: NOT        r98, r65\n"
    "  153: ORI        r99, r65, 17825792\n"
    "  154: ANDI       r100, r98, 17825792\n"
    "  155: SET_ALIAS  a6, r99\n"
    "  156: GOTO_IF_Z  r100, L18\n"
    "  157: ORI        r101, r99, -2147483648\n"
    "  158: SET_ALIAS  a6, r101\n"
    "  159: LABEL      L18\n"
    "  160: GOTO       L16\n"
    "  161: LABEL      L7\n"
    "  162: NOT        r102, r65\n"
    "  163: ORI        r103, r65, 16777216\n"
    "  164: ANDI       r104, r102, 16777216\n"
    "  165: SET_ALIAS  a6, r103\n"
    "  166: GOTO_IF_Z  r104, L19\n"
    "  167: ORI        r105, r103, -2147483648\n"
    "  168: SET_ALIAS  a6, r105\n"
    "  169: LABEL      L19\n"
    "  170: LABEL      L16\n"
    "  171: ANDI       r106, r65, 128\n"
    "  172: GOTO_IF_Z  r106, L6\n"
    "  173: GET_ALIAS  r107, a6\n"
    "  174: BFEXT      r108, r107, 12, 7\n"
    "  175: ANDI       r109, r108, 31\n"
    "  176: GET_ALIAS  r110, a6\n"
    "  177: BFINS      r111, r110, r109, 12, 7\n"
    "  178: SET_ALIAS  a6, r111\n"
    "  179: GOTO       L5\n"
    "  180: LABEL      L6\n"
    "  181: FCVT       r112, r54\n"
    "  182: VBROADCAST r113, r112\n"
    "  183: SET_ALIAS  a2, r113\n"
    "  184: BITCAST    r114, r54\n"
    "  185: SGTUI      r115, r114, 0\n"
    "  186: SRLI       r116, r114, 31\n"
    "  187: BFEXT      r120, r114, 23, 8\n"
    "  188: SEQI       r117, r120, 0\n"
    "  189: SEQI       r118, r120, 255\n"
    "  190: SLLI       r121, r114, 9\n"
    "  191: SEQI       r119, r121, 0\n"
    "  192: AND        r122, r117, r119\n"
    "  193: XORI       r123, r119, 1\n"
    "  194: AND        r124, r118, r123\n"
    "  195: AND        r125, r117, r115\n"
    "  196: OR         r126, r125, r124\n"
    "  197: OR         r127, r122, r124\n"
    "  198: XORI       r128, r127, 1\n"
    "  199: XORI       r129, r116, 1\n"
    "  200: AND        r130, r116, r128\n"
    "  201: AND        r131, r129, r128\n"
    "  202: SLLI       r132, r126, 4\n"
    "  203: SLLI       r133, r130, 3\n"
    "  204: SLLI       r134, r131, 2\n"
    "  205: SLLI       r135, r122, 1\n"
    "  206: OR         r136, r132, r133\n"
    "  207: OR         r137, r134, r135\n"
    "  208: OR         r138, r136, r118\n"
    "  209: OR         r139, r138, r137\n"
    "  210: FTESTEXC   r140, r66, INEXACT\n"
    "  211: SLLI       r141, r140, 5\n"
    "  212: OR         r142, r139, r141\n"
    "  213: GET_ALIAS  r143, a6\n"
    "  214: BFINS      r144, r143, r142, 12, 7\n"
    "  215: SET_ALIAS  a6, r144\n"
    "  216: GOTO_IF_Z  r140, L20\n"
    "  217: GET_ALIAS  r145, a6\n"
    "  218: NOT        r146, r145\n"
    "  219: ORI        r147, r145, 33554432\n"
    "  220: ANDI       r148, r146, 33554432\n"
    "  221: SET_ALIAS  a6, r147\n"
    "  222: GOTO_IF_Z  r148, L21\n"
    "  223: ORI        r149, r147, -2147483648\n"
    "  224: SET_ALIAS  a6, r149\n"
    "  225: LABEL      L21\n"
    "  226: LABEL      L20\n"
    "  227: FTESTEXC   r150, r66, OVERFLOW\n"
    "  228: GOTO_IF_Z  r150, L22\n"
    "  229: GET_ALIAS  r151, a6\n"
    "  230: NOT        r152, r151\n"
    "  231: ORI        r153, r151, 268435456\n"
    "  232: ANDI       r154, r152, 268435456\n"
    "  233: SET_ALIAS  a6, r153\n"
    "  234: GOTO_IF_Z  r154, L23\n"
    "  235: ORI        r155, r153, -2147483648\n"
    "  236: SET_ALIAS  a6, r155\n"
    "  237: LABEL      L23\n"
    "  238: LABEL      L22\n"
    "  239: FTESTEXC   r156, r66, UNDERFLOW\n"
    "  240: GOTO_IF_Z  r156, L5\n"
    "  241: GET_ALIAS  r157, a6\n"
    "  242: NOT        r158, r157\n"
    "  243: ORI        r159, r157, 134217728\n"
    "  244: ANDI       r160, r158, 134217728\n"
    "  245: SET_ALIAS  a6, r159\n"
    "  246: GOTO_IF_Z  r160, L24\n"
    "  247: ORI        r161, r159, -2147483648\n"
    "  248: SET_ALIAS  a6, r161\n"
    "  249: LABEL      L24\n"
    "  250: LABEL      L5\n"
    "  251: GET_ALIAS  r162, a2\n"
    "  252: VEXTRACT   r163, r162, 0\n"
    "  253: GET_ALIAS  r164, a6\n"
    "  254: BFEXT      r165, r164, 12, 7\n"
    "  255: FMADD      r166, r7, r9, r11\n"
    "  256: FGETSTATE  r167\n"
    "  257: FTESTEXC   r168, r167, INVALID\n"
    "  258: FCVT       r169, r166\n"
    "  259: LOAD_IMM   r170, 0x1000000\n"
    "  260: BITCAST    r171, r169\n"
    "  261: SLLI       r172, r171, 1\n"
    "  262: SEQ        r173, r172, r170\n"
    "  263: GOTO_IF_Z  r173, L25\n"
    "  264: FGETSTATE  r174\n"
    "  265: FSETROUND  r175, r174, TRUNC\n"
    "  266: FSETSTATE  r175\n"
    "  267: FMADD      r176, r7, r9, r11\n"
    "  268: FCVT       r177, r176\n"
    "  269: FGETSTATE  r178\n"
    "  270: FCOPYROUND r179, r178, r174\n"
    "  271: FSETSTATE  r179\n"
    "  272: LABEL      L25\n"
    "  273: GET_ALIAS  r180, a6\n"
    "  274: FGETSTATE  r181\n"
    "  275: FCLEAREXC  r182, r181\n"
    "  276: FSETSTATE  r182\n"
    "  277: FTESTEXC   r183, r181, INVALID\n"
    "  278: GOTO_IF_Z  r183, L27\n"
    "  279: BITCAST    r184, r7\n"
    "  280: SLLI       r185, r184, 13\n"
    "  281: BFEXT      r186, r184, 51, 12\n"
    "  282: SEQI       r187, r186, 4094\n"
    "  283: GOTO_IF_Z  r187, L29\n"
    "  284: GOTO_IF_NZ r185, L28\n"
    "  285: LABEL      L29\n"
    "  286: BITCAST    r188, r9\n"
    "  287: SLLI       r189, r188, 13\n"
    "  288: BFEXT      r190, r188, 51, 12\n"
    "  289: SEQI       r191, r190, 4094\n"
    "  290: GOTO_IF_Z  r191, L30\n"
    "  291: GOTO_IF_NZ r189, L28\n"
    "  292: LABEL      L30\n"
    "  293: BITCAST    r192, r11\n"
    "  294: SLLI       r193, r192, 13\n"
    "  295: BFEXT      r194, r192, 51, 12\n"
    "  296: SEQI       r195, r194, 4094\n"
    "  297: GOTO_IF_Z  r195, L32\n"
    "  298: GOTO_IF_NZ r193, L31\n"
    "  299: LABEL      L32\n"
    "  300: BITCAST    r196, r7\n"
    "  301: BITCAST    r197, r9\n"
    "  302: SLLI       r198, r196, 1\n"
    "  303: SLLI       r199, r197, 1\n"
    "  304: LOAD_IMM   r200, 0xFFE0000000000000\n"
    "  305: SEQ        r201, r198, r200\n"
    "  306: GOTO_IF_Z  r201, L33\n"
    "  307: GOTO_IF_Z  r199, L34\n"
    "  308: GOTO       L35\n"
    "  309: LABEL      L33\n"
    "  310: GOTO_IF_NZ r198, L35\n"
    "  311: LABEL      L34\n"
    "  312: NOT        r202, r180\n"
    "  313: ORI        r203, r180, 1048576\n"
    "  314: ANDI       r204, r202, 1048576\n"
    "  315: SET_ALIAS  a6, r203\n"
    "  316: GOTO_IF_Z  r204, L36\n"
    "  317: ORI        r205, r203, -2147483648\n"
    "  318: SET_ALIAS  a6, r205\n"
    "  319: LABEL      L36\n"
    "  320: GOTO       L37\n"
    "  321: LABEL      L35\n"
    "  322: NOT        r206, r180\n"
    "  323: ORI        r207, r180, 8388608\n"
    "  324: ANDI       r208, r206, 8388608\n"
    "  325: SET_ALIAS  a6, r207\n"
    "  326: GOTO_IF_Z  r208, L38\n"
    "  327: ORI        r209, r207, -2147483648\n"
    "  328: SET_ALIAS  a6, r209\n"
    "  329: LABEL      L38\n"
    "  330: GOTO       L37\n"
    "  331: LABEL      L31\n"
    "  332: FMUL       r210, r7, r9\n"
    "  333: FGETSTATE  r211\n"
    "  334: FSETSTATE  r182\n"
    "  335: FTESTEXC   r212, r211, INVALID\n"
    "  336: GOTO_IF_Z  r212, L28\n"
    "  337: NOT        r213, r180\n"
    "  338: ORI        r214, r180, 17825792\n"
    "  339: ANDI       r215, r213, 17825792\n"
    "  340: SET_ALIAS  a6, r214\n"
    "  341: GOTO_IF_Z  r215, L39\n"
    "  342: ORI        r216, r214, -2147483648\n"
    "  343: SET_ALIAS  a6, r216\n"
    "  344: LABEL      L39\n"
    "  345: GOTO       L37\n"
    "  346: LABEL      L28\n"
    "  347: NOT        r217, r180\n"
    "  348: ORI        r218, r180, 16777216\n"
    "  349: ANDI       r219, r217, 16777216\n"
    "  350: SET_ALIAS  a6, r218\n"
    "  351: GOTO_IF_Z  r219, L40\n"
    "  352: ORI        r220, r218, -2147483648\n"
    "  353: SET_ALIAS  a6, r220\n"
    "  354: LABEL      L40\n"
    "  355: LABEL      L37\n"
    "  356: ANDI       r221, r180, 128\n"
    "  357: GOTO_IF_Z  r221, L27\n"
    "  358: GET_ALIAS  r222, a6\n"
    "  359: BFEXT      r223, r222, 12, 7\n"
    "  360: ANDI       r224, r223, 31\n"
    "  361: GET_ALIAS  r225, a6\n"
    "  362: BFINS      r226, r225, r224, 12, 7\n"
    "  363: SET_ALIAS  a6, r226\n"
    "  364: GOTO       L26\n"
    "  365: LABEL      L27\n"
    "  366: FCVT       r227, r169\n"
    "  367: VBROADCAST r228, r227\n"
    "  368: SET_ALIAS  a2, r228\n"
    "  369: BITCAST    r229, r169\n"
    "  370: SGTUI      r230, r229, 0\n"
    "  371: SRLI       r231, r229, 31\n"
    "  372: BFEXT      r235, r229, 23, 8\n"
    "  373: SEQI       r232, r235, 0\n"
    "  374: SEQI       r233, r235, 255\n"
    "  375: SLLI       r236, r229, 9\n"
    "  376: SEQI       r234, r236, 0\n"
    "  377: AND        r237, r232, r234\n"
    "  378: XORI       r238, r234, 1\n"
    "  379: AND        r239, r233, r238\n"
    "  380: AND        r240, r232, r230\n"
    "  381: OR         r241, r240, r239\n"
    "  382: OR         r242, r237, r239\n"
    "  383: XORI       r243, r242, 1\n"
    "  384: XORI       r244, r231, 1\n"
    "  385: AND        r245, r231, r243\n"
    "  386: AND        r246, r244, r243\n"
    "  387: SLLI       r247, r241, 4\n"
    "  388: SLLI       r248, r245, 3\n"
    "  389: SLLI       r249, r246, 2\n"
    "  390: SLLI       r250, r237, 1\n"
    "  391: OR         r251, r247, r248\n"
    "  392: OR         r252, r249, r250\n"
    "  393: OR         r253, r251, r233\n"
    "  394: OR         r254, r253, r252\n"
    "  395: FTESTEXC   r255, r181, INEXACT\n"
    "  396: SLLI       r256, r255, 5\n"
    "  397: OR         r257, r254, r256\n"
    "  398: GET_ALIAS  r258, a6\n"
    "  399: BFINS      r259, r258, r257, 12, 7\n"
    "  400: SET_ALIAS  a6, r259\n"
    "  401: GOTO_IF_Z  r255, L41\n"
    "  402: GET_ALIAS  r260, a6\n"
    "  403: NOT        r261, r260\n"
    "  404: ORI        r262, r260, 33554432\n"
    "  405: ANDI       r263, r261, 33554432\n"
    "  406: SET_ALIAS  a6, r262\n"
    "  407: GOTO_IF_Z  r263, L42\n"
    "  408: ORI        r264, r262, -2147483648\n"
    "  409: SET_ALIAS  a6, r264\n"
    "  410: LABEL      L42\n"
    "  411: LABEL      L41\n"
    "  412: FTESTEXC   r265, r181, OVERFLOW\n"
    "  413: GOTO_IF_Z  r265, L43\n"
    "  414: GET_ALIAS  r266, a6\n"
    "  415: NOT        r267, r266\n"
    "  416: ORI        r268, r266, 268435456\n"
    "  417: ANDI       r269, r267, 268435456\n"
    "  418: SET_ALIAS  a6, r268\n"
    "  419: GOTO_IF_Z  r269, L44\n"
    "  420: ORI        r270, r268, -2147483648\n"
    "  421: SET_ALIAS  a6, r270\n"
    "  422: LABEL      L44\n"
    "  423: LABEL      L43\n"
    "  424: FTESTEXC   r271, r181, UNDERFLOW\n"
    "  425: GOTO_IF_Z  r271, L26\n"
    "  426: GET_ALIAS  r272, a6\n"
    "  427: NOT        r273, r272\n"
    "  428: ORI        r274, r272, 134217728\n"
    "  429: ANDI       r275, r273, 134217728\n"
    "  430: SET_ALIAS  a6, r274\n"
    "  431: GOTO_IF_Z  r275, L45\n"
    "  432: ORI        r276, r274, -2147483648\n"
    "  433: SET_ALIAS  a6, r276\n"
    "  434: LABEL      L45\n"
    "  435: LABEL      L26\n"
    "  436: GET_ALIAS  r277, a2\n"
    "  437: VEXTRACT   r278, r277, 0\n"
    "  438: GET_ALIAS  r279, a6\n"
    "  439: BFEXT      r280, r279, 12, 7\n"
    "  440: ANDI       r281, r280, 32\n"
    "  441: OR         r282, r165, r281\n"
    "  442: GET_ALIAS  r283, a6\n"
    "  443: BFINS      r284, r283, r282, 12, 7\n"
    "  444: SET_ALIAS  a6, r284\n"
    "  445: OR         r285, r53, r168\n"
    "  446: GOTO_IF_Z  r285, L46\n"
    "  447: GET_ALIAS  r286, a6\n"
    "  448: ANDI       r287, r286, 128\n"
    "  449: GOTO_IF_Z  r287, L46\n"
    "  450: SET_ALIAS  a2, r50\n"
    "  451: GOTO       L47\n"
    "  452: LABEL      L46\n"
    "  453: VBUILD2    r288, r163, r278\n"
    "  454: SET_ALIAS  a2, r288\n"
    "  455: LABEL      L47\n"
    "  456: LOAD_IMM   r289, 4\n"
    "  457: SET_ALIAS  a1, r289\n"
    "  458: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "Alias 7: float64, no bound storage\n"
    "Alias 8: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,16] --> 1,11\n"
    "Block 1: 0 --> [17,20] --> 2,11\n"
    "Block 2: 1 --> [21,22] --> 3,11\n"
    "Block 3: 2 --> [23,23] --> 4,6\n"
    "Block 4: 3 --> [24,28] --> 5,11\n"
    "Block 5: 4 --> [29,37] --> 6\n"
    "Block 6: 5,3 --> [38,48] --> 7,11\n"
    "Block 7: 6 --> [49,54] --> 8,10\n"
    "Block 8: 7 --> [55,56] --> 9,11\n"
    "Block 9: 8 --> [57,60] --> 11\n"
    "Block 10: 7 --> [61,65] --> 11\n"
    "Block 11: 10,9,0,1,2,4,6,8 --> [66,78] --> 12,13\n"
    "Block 12: 11 --> [79,86] --> 13\n"
    "Block 13: 12,11 --> [87,93] --> 14,39\n"
    "Block 14: 13 --> [94,98] --> 15,16\n"
    "Block 15: 14 --> [99,99] --> 16,34\n"
    "Block 16: 15,14 --> [100,105] --> 17,18\n"
    "Block 17: 16 --> [106,106] --> 18,34\n"
    "Block 18: 17,16 --> [107,112] --> 19,20\n"
    "Block 19: 18 --> [113,113] --> 20,30\n"
    "Block 20: 19,18 --> [114,121] --> 21,23\n"
    "Block 21: 20 --> [122,122] --> 22,24\n"
    "Block 22: 21 --> [123,123] --> 27\n"
    "Block 23: 20 --> [124,125] --> 24,27\n"
    "Block 24: 23,21 --> [126,131] --> 25,26\n"
    "Block 25: 24 --> [132,133] --> 26\n"
    "Block 26: 25,24 --> [134,135] --> 37\n"
    "Block 27: 22,23 --> [136,141] --> 28,29\n"
    "Block 28: 27 --> [142,143] --> 29\n"
    "Block 29: 28,27 --> [144,145] --> 37\n"
    "Block 30: 19 --> [146,151] --> 31,34\n"
    "Block 31: 30 --> [152,156] --> 32,33\n"
    "Block 32: 31 --> [157,158] --> 33\n"
    "Block 33: 32,31 --> [159,160] --> 37\n"
    "Block 34: 15,17,30 --> [161,166] --> 35,36\n"
    "Block 35: 34 --> [167,168] --> 36\n"
    "Block 36: 35,34 --> [169,169] --> 37\n"
    "Block 37: 36,26,29,33 --> [170,172] --> 38,39\n"
    "Block 38: 37 --> [173,179] --> 51\n"
    "Block 39: 13,37 --> [180,216] --> 40,43\n"
    "Block 40: 39 --> [217,222] --> 41,42\n"
    "Block 41: 40 --> [223,224] --> 42\n"
    "Block 42: 41,40 --> [225,225] --> 43\n"
    "Block 43: 42,39 --> [226,228] --> 44,47\n"
    "Block 44: 43 --> [229,234] --> 45,46\n"
    "Block 45: 44 --> [235,236] --> 46\n"
    "Block 46: 45,44 --> [237,237] --> 47\n"
    "Block 47: 46,43 --> [238,240] --> 48,51\n"
    "Block 48: 47 --> [241,246] --> 49,50\n"
    "Block 49: 48 --> [247,248] --> 50\n"
    "Block 50: 49,48 --> [249,249] --> 51\n"
    "Block 51: 50,38,47 --> [250,263] --> 52,53\n"
    "Block 52: 51 --> [264,271] --> 53\n"
    "Block 53: 52,51 --> [272,278] --> 54,79\n"
    "Block 54: 53 --> [279,283] --> 55,56\n"
    "Block 55: 54 --> [284,284] --> 56,74\n"
    "Block 56: 55,54 --> [285,290] --> 57,58\n"
    "Block 57: 56 --> [291,291] --> 58,74\n"
    "Block 58: 57,56 --> [292,297] --> 59,60\n"
    "Block 59: 58 --> [298,298] --> 60,70\n"
    "Block 60: 59,58 --> [299,306] --> 61,63\n"
    "Block 61: 60 --> [307,307] --> 62,64\n"
    "Block 62: 61 --> [308,308] --> 67\n"
    "Block 63: 60 --> [309,310] --> 64,67\n"
    "Block 64: 63,61 --> [311,316] --> 65,66\n"
    "Block 65: 64 --> [317,318] --> 66\n"
    "Block 66: 65,64 --> [319,320] --> 77\n"
    "Block 67: 62,63 --> [321,326] --> 68,69\n"
    "Block 68: 67 --> [327,328] --> 69\n"
    "Block 69: 68,67 --> [329,330] --> 77\n"
    "Block 70: 59 --> [331,336] --> 71,74\n"
    "Block 71: 70 --> [337,341] --> 72,73\n"
    "Block 72: 71 --> [342,343] --> 73\n"
    "Block 73: 72,71 --> [344,345] --> 77\n"
    "Block 74: 55,57,70 --> [346,351] --> 75,76\n"
    "Block 75: 74 --> [352,353] --> 76\n"
    "Block 76: 75,74 --> [354,354] --> 77\n"
    "Block 77: 76,66,69,73 --> [355,357] --> 78,79\n"
    "Block 78: 77 --> [358,364] --> 91\n"
    "Block 79: 53,77 --> [365,401] --> 80,83\n"
    "Block 80: 79 --> [402,407] --> 81,82\n"
    "Block 81: 80 --> [408,409] --> 82\n"
    "Block 82: 81,80 --> [410,410] --> 83\n"
    "Block 83: 82,79 --> [411,413] --> 84,87\n"
    "Block 84: 83 --> [414,419] --> 85,86\n"
    "Block 85: 84 --> [420,421] --> 86\n"
    "Block 86: 85,84 --> [422,422] --> 87\n"
    "Block 87: 86,83 --> [423,425] --> 88,91\n"
    "Block 88: 87 --> [426,431] --> 89,90\n"
    "Block 89: 88 --> [432,433] --> 90\n"
    "Block 90: 89,88 --> [434,434] --> 91\n"
    "Block 91: 90,78,87 --> [435,446] --> 92,94\n"
    "Block 92: 91 --> [447,449] --> 93,94\n"
    "Block 93: 92 --> [450,451] --> 95\n"
    "Block 94: 91,92 --> [452,454] --> 95\n"
    "Block 95: 94,93 --> [455,458] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
