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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NATIVE_RECIPROCAL;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FGETSTATE  r4\n"
    "    4: VFCAST     r5, r3\n"
    "    5: FSETSTATE  r4\n"
    "    6: LOAD_IMM   r6, 1.0f\n"
    "    7: VBROADCAST r7, r6\n"
    "    8: FDIV       r8, r7, r5\n"
    "    9: FGETSTATE  r9\n"
    "   10: FTESTEXC   r10, r9, INVALID\n"
    "   11: GOTO_IF_Z  r10, L1\n"
    "   12: GET_ALIAS  r11, a2\n"
    "   13: FCLEAREXC  r12, r9\n"
    "   14: FSETSTATE  r12\n"
    "   15: VEXTRACT   r13, r5, 0\n"
    "   16: FDIV       r14, r6, r13\n"
    "   17: GET_ALIAS  r15, a4\n"
    "   18: FGETSTATE  r16\n"
    "   19: FCLEAREXC  r17, r16\n"
    "   20: FSETSTATE  r17\n"
    "   21: FTESTEXC   r18, r16, INVALID\n"
    "   22: GOTO_IF_Z  r18, L3\n"
    "   23: NOT        r19, r15\n"
    "   24: ORI        r20, r15, 16777216\n"
    "   25: ANDI       r21, r19, 16777216\n"
    "   26: SET_ALIAS  a4, r20\n"
    "   27: GOTO_IF_Z  r21, L4\n"
    "   28: ORI        r22, r20, -2147483648\n"
    "   29: SET_ALIAS  a4, r22\n"
    "   30: LABEL      L4\n"
    "   31: ANDI       r23, r15, 128\n"
    "   32: GOTO_IF_Z  r23, L3\n"
    "   33: LABEL      L5\n"
    "   34: GET_ALIAS  r24, a4\n"
    "   35: BFEXT      r25, r24, 12, 7\n"
    "   36: ANDI       r26, r25, 31\n"
    "   37: GET_ALIAS  r27, a4\n"
    "   38: BFINS      r28, r27, r26, 12, 7\n"
    "   39: SET_ALIAS  a4, r28\n"
    "   40: GOTO       L2\n"
    "   41: LABEL      L3\n"
    "   42: FTESTEXC   r29, r16, ZERO_DIVIDE\n"
    "   43: GOTO_IF_Z  r29, L6\n"
    "   44: NOT        r30, r15\n"
    "   45: ORI        r31, r15, 67108864\n"
    "   46: ANDI       r32, r30, 67108864\n"
    "   47: SET_ALIAS  a4, r31\n"
    "   48: GOTO_IF_Z  r32, L7\n"
    "   49: ORI        r33, r31, -2147483648\n"
    "   50: SET_ALIAS  a4, r33\n"
    "   51: LABEL      L7\n"
    "   52: ANDI       r34, r15, 16\n"
    "   53: GOTO_IF_NZ r34, L5\n"
    "   54: LABEL      L6\n"
    "   55: FCVT       r35, r14\n"
    "   56: VBROADCAST r36, r35\n"
    "   57: SET_ALIAS  a2, r36\n"
    "   58: BITCAST    r37, r14\n"
    "   59: SGTUI      r38, r37, 0\n"
    "   60: SRLI       r39, r37, 31\n"
    "   61: BFEXT      r43, r37, 23, 8\n"
    "   62: SEQI       r40, r43, 0\n"
    "   63: SEQI       r41, r43, 255\n"
    "   64: SLLI       r44, r37, 9\n"
    "   65: SEQI       r42, r44, 0\n"
    "   66: AND        r45, r40, r42\n"
    "   67: XORI       r46, r42, 1\n"
    "   68: AND        r47, r41, r46\n"
    "   69: AND        r48, r40, r38\n"
    "   70: OR         r49, r48, r47\n"
    "   71: OR         r50, r45, r47\n"
    "   72: XORI       r51, r50, 1\n"
    "   73: XORI       r52, r39, 1\n"
    "   74: AND        r53, r39, r51\n"
    "   75: AND        r54, r52, r51\n"
    "   76: SLLI       r55, r49, 4\n"
    "   77: SLLI       r56, r53, 3\n"
    "   78: SLLI       r57, r54, 2\n"
    "   79: SLLI       r58, r45, 1\n"
    "   80: OR         r59, r55, r56\n"
    "   81: OR         r60, r57, r58\n"
    "   82: OR         r61, r59, r41\n"
    "   83: OR         r62, r61, r60\n"
    "   84: FTESTEXC   r63, r16, INEXACT\n"
    "   85: SLLI       r64, r63, 5\n"
    "   86: OR         r65, r62, r64\n"
    "   87: GET_ALIAS  r66, a4\n"
    "   88: BFINS      r67, r66, r65, 12, 7\n"
    "   89: SET_ALIAS  a4, r67\n"
    "   90: FTESTEXC   r68, r16, OVERFLOW\n"
    "   91: GOTO_IF_Z  r68, L8\n"
    "   92: GET_ALIAS  r69, a4\n"
    "   93: NOT        r70, r69\n"
    "   94: ORI        r71, r69, 268435456\n"
    "   95: ANDI       r72, r70, 268435456\n"
    "   96: SET_ALIAS  a4, r71\n"
    "   97: GOTO_IF_Z  r72, L9\n"
    "   98: ORI        r73, r71, -2147483648\n"
    "   99: SET_ALIAS  a4, r73\n"
    "  100: LABEL      L9\n"
    "  101: LABEL      L8\n"
    "  102: FTESTEXC   r74, r16, UNDERFLOW\n"
    "  103: GOTO_IF_Z  r74, L2\n"
    "  104: GET_ALIAS  r75, a4\n"
    "  105: NOT        r76, r75\n"
    "  106: ORI        r77, r75, 134217728\n"
    "  107: ANDI       r78, r76, 134217728\n"
    "  108: SET_ALIAS  a4, r77\n"
    "  109: GOTO_IF_Z  r78, L10\n"
    "  110: ORI        r79, r77, -2147483648\n"
    "  111: SET_ALIAS  a4, r79\n"
    "  112: LABEL      L10\n"
    "  113: LABEL      L2\n"
    "  114: GET_ALIAS  r80, a2\n"
    "  115: VEXTRACT   r81, r80, 0\n"
    "  116: GET_ALIAS  r82, a4\n"
    "  117: BFEXT      r83, r82, 12, 7\n"
    "  118: VEXTRACT   r84, r5, 1\n"
    "  119: FDIV       r85, r6, r84\n"
    "  120: GET_ALIAS  r86, a4\n"
    "  121: FGETSTATE  r87\n"
    "  122: FCLEAREXC  r88, r87\n"
    "  123: FSETSTATE  r88\n"
    "  124: FTESTEXC   r89, r87, INVALID\n"
    "  125: GOTO_IF_Z  r89, L12\n"
    "  126: NOT        r90, r86\n"
    "  127: ORI        r91, r86, 16777216\n"
    "  128: ANDI       r92, r90, 16777216\n"
    "  129: SET_ALIAS  a4, r91\n"
    "  130: GOTO_IF_Z  r92, L13\n"
    "  131: ORI        r93, r91, -2147483648\n"
    "  132: SET_ALIAS  a4, r93\n"
    "  133: LABEL      L13\n"
    "  134: ANDI       r94, r86, 128\n"
    "  135: GOTO_IF_Z  r94, L12\n"
    "  136: LABEL      L14\n"
    "  137: GET_ALIAS  r95, a4\n"
    "  138: BFEXT      r96, r95, 12, 7\n"
    "  139: ANDI       r97, r96, 31\n"
    "  140: GET_ALIAS  r98, a4\n"
    "  141: BFINS      r99, r98, r97, 12, 7\n"
    "  142: SET_ALIAS  a4, r99\n"
    "  143: GOTO       L11\n"
    "  144: LABEL      L12\n"
    "  145: FTESTEXC   r100, r87, ZERO_DIVIDE\n"
    "  146: GOTO_IF_Z  r100, L15\n"
    "  147: NOT        r101, r86\n"
    "  148: ORI        r102, r86, 67108864\n"
    "  149: ANDI       r103, r101, 67108864\n"
    "  150: SET_ALIAS  a4, r102\n"
    "  151: GOTO_IF_Z  r103, L16\n"
    "  152: ORI        r104, r102, -2147483648\n"
    "  153: SET_ALIAS  a4, r104\n"
    "  154: LABEL      L16\n"
    "  155: ANDI       r105, r86, 16\n"
    "  156: GOTO_IF_NZ r105, L14\n"
    "  157: LABEL      L15\n"
    "  158: FCVT       r106, r85\n"
    "  159: VBROADCAST r107, r106\n"
    "  160: SET_ALIAS  a2, r107\n"
    "  161: BITCAST    r108, r85\n"
    "  162: SGTUI      r109, r108, 0\n"
    "  163: SRLI       r110, r108, 31\n"
    "  164: BFEXT      r114, r108, 23, 8\n"
    "  165: SEQI       r111, r114, 0\n"
    "  166: SEQI       r112, r114, 255\n"
    "  167: SLLI       r115, r108, 9\n"
    "  168: SEQI       r113, r115, 0\n"
    "  169: AND        r116, r111, r113\n"
    "  170: XORI       r117, r113, 1\n"
    "  171: AND        r118, r112, r117\n"
    "  172: AND        r119, r111, r109\n"
    "  173: OR         r120, r119, r118\n"
    "  174: OR         r121, r116, r118\n"
    "  175: XORI       r122, r121, 1\n"
    "  176: XORI       r123, r110, 1\n"
    "  177: AND        r124, r110, r122\n"
    "  178: AND        r125, r123, r122\n"
    "  179: SLLI       r126, r120, 4\n"
    "  180: SLLI       r127, r124, 3\n"
    "  181: SLLI       r128, r125, 2\n"
    "  182: SLLI       r129, r116, 1\n"
    "  183: OR         r130, r126, r127\n"
    "  184: OR         r131, r128, r129\n"
    "  185: OR         r132, r130, r112\n"
    "  186: OR         r133, r132, r131\n"
    "  187: FTESTEXC   r134, r87, INEXACT\n"
    "  188: SLLI       r135, r134, 5\n"
    "  189: OR         r136, r133, r135\n"
    "  190: GET_ALIAS  r137, a4\n"
    "  191: BFINS      r138, r137, r136, 12, 7\n"
    "  192: SET_ALIAS  a4, r138\n"
    "  193: FTESTEXC   r139, r87, OVERFLOW\n"
    "  194: GOTO_IF_Z  r139, L17\n"
    "  195: GET_ALIAS  r140, a4\n"
    "  196: NOT        r141, r140\n"
    "  197: ORI        r142, r140, 268435456\n"
    "  198: ANDI       r143, r141, 268435456\n"
    "  199: SET_ALIAS  a4, r142\n"
    "  200: GOTO_IF_Z  r143, L18\n"
    "  201: ORI        r144, r142, -2147483648\n"
    "  202: SET_ALIAS  a4, r144\n"
    "  203: LABEL      L18\n"
    "  204: LABEL      L17\n"
    "  205: FTESTEXC   r145, r87, UNDERFLOW\n"
    "  206: GOTO_IF_Z  r145, L11\n"
    "  207: GET_ALIAS  r146, a4\n"
    "  208: NOT        r147, r146\n"
    "  209: ORI        r148, r146, 134217728\n"
    "  210: ANDI       r149, r147, 134217728\n"
    "  211: SET_ALIAS  a4, r148\n"
    "  212: GOTO_IF_Z  r149, L19\n"
    "  213: ORI        r150, r148, -2147483648\n"
    "  214: SET_ALIAS  a4, r150\n"
    "  215: LABEL      L19\n"
    "  216: LABEL      L11\n"
    "  217: GET_ALIAS  r151, a2\n"
    "  218: VEXTRACT   r152, r151, 0\n"
    "  219: GET_ALIAS  r153, a4\n"
    "  220: BFEXT      r154, r153, 12, 7\n"
    "  221: ANDI       r155, r154, 32\n"
    "  222: OR         r156, r83, r155\n"
    "  223: GET_ALIAS  r157, a4\n"
    "  224: BFINS      r158, r157, r156, 12, 7\n"
    "  225: SET_ALIAS  a4, r158\n"
    "  226: GET_ALIAS  r159, a4\n"
    "  227: ANDI       r160, r159, 128\n"
    "  228: GOTO_IF_Z  r160, L20\n"
    "  229: SET_ALIAS  a2, r11\n"
    "  230: GOTO       L21\n"
    "  231: LABEL      L20\n"
    "  232: VBUILD2    r161, r81, r152\n"
    "  233: SET_ALIAS  a2, r161\n"
    "  234: GOTO       L21\n"
    "  235: LABEL      L1\n"
    "  236: GET_ALIAS  r162, a4\n"
    "  237: FGETSTATE  r163\n"
    "  238: FCLEAREXC  r164, r163\n"
    "  239: FSETSTATE  r164\n"
    "  240: FTESTEXC   r165, r163, ZERO_DIVIDE\n"
    "  241: GOTO_IF_Z  r165, L23\n"
    "  242: NOT        r166, r162\n"
    "  243: ORI        r167, r162, 67108864\n"
    "  244: ANDI       r168, r166, 67108864\n"
    "  245: SET_ALIAS  a4, r167\n"
    "  246: GOTO_IF_Z  r168, L24\n"
    "  247: ORI        r169, r167, -2147483648\n"
    "  248: SET_ALIAS  a4, r169\n"
    "  249: LABEL      L24\n"
    "  250: ANDI       r170, r162, 16\n"
    "  251: GOTO_IF_Z  r170, L23\n"
    "  252: VEXTRACT   r171, r7, 0\n"
    "  253: VEXTRACT   r172, r7, 1\n"
    "  254: VEXTRACT   r173, r5, 0\n"
    "  255: VEXTRACT   r174, r5, 1\n"
    "  256: FDIV       r175, r171, r173\n"
    "  257: FGETSTATE  r176\n"
    "  258: FDIV       r177, r172, r174\n"
    "  259: FTESTEXC   r178, r176, ZERO_DIVIDE\n"
    "  260: BITCAST    r179, r175\n"
    "  261: SGTUI      r180, r179, 0\n"
    "  262: SRLI       r181, r179, 31\n"
    "  263: BFEXT      r185, r179, 23, 8\n"
    "  264: SEQI       r182, r185, 0\n"
    "  265: SEQI       r183, r185, 255\n"
    "  266: SLLI       r186, r179, 9\n"
    "  267: SEQI       r184, r186, 0\n"
    "  268: AND        r187, r182, r184\n"
    "  269: XORI       r188, r184, 1\n"
    "  270: AND        r189, r183, r188\n"
    "  271: AND        r190, r182, r180\n"
    "  272: OR         r191, r190, r189\n"
    "  273: OR         r192, r187, r189\n"
    "  274: XORI       r193, r192, 1\n"
    "  275: XORI       r194, r181, 1\n"
    "  276: AND        r195, r181, r193\n"
    "  277: AND        r196, r194, r193\n"
    "  278: SLLI       r197, r191, 4\n"
    "  279: SLLI       r198, r195, 3\n"
    "  280: SLLI       r199, r196, 2\n"
    "  281: SLLI       r200, r187, 1\n"
    "  282: OR         r201, r197, r198\n"
    "  283: OR         r202, r199, r200\n"
    "  284: OR         r203, r201, r183\n"
    "  285: OR         r204, r203, r202\n"
    "  286: LOAD_IMM   r205, 0\n"
    "  287: SELECT     r206, r205, r204, r178\n"
    "  288: FGETSTATE  r207\n"
    "  289: FSETSTATE  r164\n"
    "  290: FTESTEXC   r208, r207, INEXACT\n"
    "  291: GOTO_IF_Z  r208, L25\n"
    "  292: GET_ALIAS  r209, a4\n"
    "  293: NOT        r210, r209\n"
    "  294: ORI        r211, r209, 33554432\n"
    "  295: ANDI       r212, r210, 33554432\n"
    "  296: SET_ALIAS  a4, r211\n"
    "  297: GOTO_IF_Z  r212, L26\n"
    "  298: ORI        r213, r211, -2147483648\n"
    "  299: SET_ALIAS  a4, r213\n"
    "  300: LABEL      L26\n"
    "  301: LABEL      L25\n"
    "  302: FTESTEXC   r214, r207, OVERFLOW\n"
    "  303: GOTO_IF_Z  r214, L27\n"
    "  304: GET_ALIAS  r215, a4\n"
    "  305: NOT        r216, r215\n"
    "  306: ORI        r217, r215, 268435456\n"
    "  307: ANDI       r218, r216, 268435456\n"
    "  308: SET_ALIAS  a4, r217\n"
    "  309: GOTO_IF_Z  r218, L28\n"
    "  310: ORI        r219, r217, -2147483648\n"
    "  311: SET_ALIAS  a4, r219\n"
    "  312: LABEL      L28\n"
    "  313: LABEL      L27\n"
    "  314: FTESTEXC   r220, r207, UNDERFLOW\n"
    "  315: GOTO_IF_Z  r220, L29\n"
    "  316: GET_ALIAS  r221, a4\n"
    "  317: NOT        r222, r221\n"
    "  318: ORI        r223, r221, 134217728\n"
    "  319: ANDI       r224, r222, 134217728\n"
    "  320: SET_ALIAS  a4, r223\n"
    "  321: GOTO_IF_Z  r224, L30\n"
    "  322: ORI        r225, r223, -2147483648\n"
    "  323: SET_ALIAS  a4, r225\n"
    "  324: LABEL      L30\n"
    "  325: LABEL      L29\n"
    "  326: SLLI       r226, r208, 5\n"
    "  327: OR         r227, r226, r206\n"
    "  328: GET_ALIAS  r228, a4\n"
    "  329: BFINS      r229, r228, r227, 12, 7\n"
    "  330: SET_ALIAS  a4, r229\n"
    "  331: GOTO       L22\n"
    "  332: LABEL      L23\n"
    "  333: VFCVT      r230, r8\n"
    "  334: SET_ALIAS  a2, r230\n"
    "  335: VEXTRACT   r231, r8, 0\n"
    "  336: BITCAST    r232, r231\n"
    "  337: SGTUI      r233, r232, 0\n"
    "  338: SRLI       r234, r232, 31\n"
    "  339: BFEXT      r238, r232, 23, 8\n"
    "  340: SEQI       r235, r238, 0\n"
    "  341: SEQI       r236, r238, 255\n"
    "  342: SLLI       r239, r232, 9\n"
    "  343: SEQI       r237, r239, 0\n"
    "  344: AND        r240, r235, r237\n"
    "  345: XORI       r241, r237, 1\n"
    "  346: AND        r242, r236, r241\n"
    "  347: AND        r243, r235, r233\n"
    "  348: OR         r244, r243, r242\n"
    "  349: OR         r245, r240, r242\n"
    "  350: XORI       r246, r245, 1\n"
    "  351: XORI       r247, r234, 1\n"
    "  352: AND        r248, r234, r246\n"
    "  353: AND        r249, r247, r246\n"
    "  354: SLLI       r250, r244, 4\n"
    "  355: SLLI       r251, r248, 3\n"
    "  356: SLLI       r252, r249, 2\n"
    "  357: SLLI       r253, r240, 1\n"
    "  358: OR         r254, r250, r251\n"
    "  359: OR         r255, r252, r253\n"
    "  360: OR         r256, r254, r236\n"
    "  361: OR         r257, r256, r255\n"
    "  362: FTESTEXC   r258, r163, INEXACT\n"
    "  363: SLLI       r259, r258, 5\n"
    "  364: OR         r260, r257, r259\n"
    "  365: GET_ALIAS  r261, a4\n"
    "  366: BFINS      r262, r261, r260, 12, 7\n"
    "  367: SET_ALIAS  a4, r262\n"
    "  368: GOTO_IF_Z  r258, L31\n"
    "  369: GET_ALIAS  r263, a4\n"
    "  370: NOT        r264, r263\n"
    "  371: ORI        r265, r263, 33554432\n"
    "  372: ANDI       r266, r264, 33554432\n"
    "  373: SET_ALIAS  a4, r265\n"
    "  374: GOTO_IF_Z  r266, L32\n"
    "  375: ORI        r267, r265, -2147483648\n"
    "  376: SET_ALIAS  a4, r267\n"
    "  377: LABEL      L32\n"
    "  378: LABEL      L31\n"
    "  379: FTESTEXC   r268, r163, OVERFLOW\n"
    "  380: GOTO_IF_Z  r268, L33\n"
    "  381: GET_ALIAS  r269, a4\n"
    "  382: NOT        r270, r269\n"
    "  383: ORI        r271, r269, 268435456\n"
    "  384: ANDI       r272, r270, 268435456\n"
    "  385: SET_ALIAS  a4, r271\n"
    "  386: GOTO_IF_Z  r272, L34\n"
    "  387: ORI        r273, r271, -2147483648\n"
    "  388: SET_ALIAS  a4, r273\n"
    "  389: LABEL      L34\n"
    "  390: LABEL      L33\n"
    "  391: FTESTEXC   r274, r163, UNDERFLOW\n"
    "  392: GOTO_IF_Z  r274, L22\n"
    "  393: GET_ALIAS  r275, a4\n"
    "  394: NOT        r276, r275\n"
    "  395: ORI        r277, r275, 134217728\n"
    "  396: ANDI       r278, r276, 134217728\n"
    "  397: SET_ALIAS  a4, r277\n"
    "  398: GOTO_IF_Z  r278, L35\n"
    "  399: ORI        r279, r277, -2147483648\n"
    "  400: SET_ALIAS  a4, r279\n"
    "  401: LABEL      L35\n"
    "  402: LABEL      L22\n"
    "  403: LABEL      L21\n"
    "  404: LOAD_IMM   r280, 4\n"
    "  405: SET_ALIAS  a1, r280\n"
    "  406: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,38\n"
    "Block 1: 0 --> [12,22] --> 2,6\n"
    "Block 2: 1 --> [23,27] --> 3,4\n"
    "Block 3: 2 --> [28,29] --> 4\n"
    "Block 4: 3,2 --> [30,32] --> 5,6\n"
    "Block 5: 4,9 --> [33,40] --> 18\n"
    "Block 6: 1,4 --> [41,43] --> 7,10\n"
    "Block 7: 6 --> [44,48] --> 8,9\n"
    "Block 8: 7 --> [49,50] --> 9\n"
    "Block 9: 8,7 --> [51,53] --> 10,5\n"
    "Block 10: 9,6 --> [54,91] --> 11,14\n"
    "Block 11: 10 --> [92,97] --> 12,13\n"
    "Block 12: 11 --> [98,99] --> 13\n"
    "Block 13: 12,11 --> [100,100] --> 14\n"
    "Block 14: 13,10 --> [101,103] --> 15,18\n"
    "Block 15: 14 --> [104,109] --> 16,17\n"
    "Block 16: 15 --> [110,111] --> 17\n"
    "Block 17: 16,15 --> [112,112] --> 18\n"
    "Block 18: 17,5,14 --> [113,125] --> 19,23\n"
    "Block 19: 18 --> [126,130] --> 20,21\n"
    "Block 20: 19 --> [131,132] --> 21\n"
    "Block 21: 20,19 --> [133,135] --> 22,23\n"
    "Block 22: 21,26 --> [136,143] --> 35\n"
    "Block 23: 18,21 --> [144,146] --> 24,27\n"
    "Block 24: 23 --> [147,151] --> 25,26\n"
    "Block 25: 24 --> [152,153] --> 26\n"
    "Block 26: 25,24 --> [154,156] --> 27,22\n"
    "Block 27: 26,23 --> [157,194] --> 28,31\n"
    "Block 28: 27 --> [195,200] --> 29,30\n"
    "Block 29: 28 --> [201,202] --> 30\n"
    "Block 30: 29,28 --> [203,203] --> 31\n"
    "Block 31: 30,27 --> [204,206] --> 32,35\n"
    "Block 32: 31 --> [207,212] --> 33,34\n"
    "Block 33: 32 --> [213,214] --> 34\n"
    "Block 34: 33,32 --> [215,215] --> 35\n"
    "Block 35: 34,22,31 --> [216,228] --> 36,37\n"
    "Block 36: 35 --> [229,230] --> 68\n"
    "Block 37: 35 --> [231,234] --> 68\n"
    "Block 38: 0 --> [235,241] --> 39,55\n"
    "Block 39: 38 --> [242,246] --> 40,41\n"
    "Block 40: 39 --> [247,248] --> 41\n"
    "Block 41: 40,39 --> [249,251] --> 42,55\n"
    "Block 42: 41 --> [252,291] --> 43,46\n"
    "Block 43: 42 --> [292,297] --> 44,45\n"
    "Block 44: 43 --> [298,299] --> 45\n"
    "Block 45: 44,43 --> [300,300] --> 46\n"
    "Block 46: 45,42 --> [301,303] --> 47,50\n"
    "Block 47: 46 --> [304,309] --> 48,49\n"
    "Block 48: 47 --> [310,311] --> 49\n"
    "Block 49: 48,47 --> [312,312] --> 50\n"
    "Block 50: 49,46 --> [313,315] --> 51,54\n"
    "Block 51: 50 --> [316,321] --> 52,53\n"
    "Block 52: 51 --> [322,323] --> 53\n"
    "Block 53: 52,51 --> [324,324] --> 54\n"
    "Block 54: 53,50 --> [325,331] --> 67\n"
    "Block 55: 38,41 --> [332,368] --> 56,59\n"
    "Block 56: 55 --> [369,374] --> 57,58\n"
    "Block 57: 56 --> [375,376] --> 58\n"
    "Block 58: 57,56 --> [377,377] --> 59\n"
    "Block 59: 58,55 --> [378,380] --> 60,63\n"
    "Block 60: 59 --> [381,386] --> 61,62\n"
    "Block 61: 60 --> [387,388] --> 62\n"
    "Block 62: 61,60 --> [389,389] --> 63\n"
    "Block 63: 62,59 --> [390,392] --> 64,67\n"
    "Block 64: 63 --> [393,398] --> 65,66\n"
    "Block 65: 64 --> [399,400] --> 66\n"
    "Block 66: 65,64 --> [401,401] --> 67\n"
    "Block 67: 66,54,63 --> [402,402] --> 68\n"
    "Block 68: 67,36,37 --> [403,406] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
