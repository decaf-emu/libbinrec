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
    0x10,0x20,0x10,0x34,  // ps_rsqrte f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FGETSTATE  r4\n"
    "    4: VFCMP      r5, r3, r3, UN\n"
    "    5: VFCVT      r6, r3\n"
    "    6: SET_ALIAS  a5, r6\n"
    "    7: GOTO_IF_Z  r5, L1\n"
    "    8: VEXTRACT   r7, r3, 0\n"
    "    9: VEXTRACT   r8, r3, 1\n"
    "   10: BFEXT      r9, r5, 0, 32\n"
    "   11: BFEXT      r10, r5, 32, 32\n"
    "   12: ZCAST      r11, r9\n"
    "   13: ZCAST      r12, r10\n"
    "   14: BITCAST    r13, r7\n"
    "   15: BITCAST    r14, r8\n"
    "   16: NOT        r15, r13\n"
    "   17: NOT        r16, r14\n"
    "   18: LOAD_IMM   r17, 0x8000000000000\n"
    "   19: AND        r18, r15, r17\n"
    "   20: AND        r19, r16, r17\n"
    "   21: VEXTRACT   r20, r6, 0\n"
    "   22: VEXTRACT   r21, r6, 1\n"
    "   23: SRLI       r22, r18, 29\n"
    "   24: SRLI       r23, r19, 29\n"
    "   25: ZCAST      r24, r22\n"
    "   26: ZCAST      r25, r23\n"
    "   27: BITCAST    r26, r20\n"
    "   28: BITCAST    r27, r21\n"
    "   29: AND        r28, r24, r11\n"
    "   30: AND        r29, r25, r12\n"
    "   31: XOR        r30, r26, r28\n"
    "   32: XOR        r31, r27, r29\n"
    "   33: BITCAST    r32, r30\n"
    "   34: BITCAST    r33, r31\n"
    "   35: VBUILD2    r34, r32, r33\n"
    "   36: SET_ALIAS  a5, r34\n"
    "   37: LABEL      L1\n"
    "   38: GET_ALIAS  r35, a5\n"
    "   39: FSETSTATE  r4\n"
    "   40: LOAD_IMM   r36, 0\n"
    "   41: SET_ALIAS  a6, r36\n"
    "   42: VEXTRACT   r37, r35, 0\n"
    "   43: GET_ALIAS  r38, a4\n"
    "   44: BITCAST    r39, r37\n"
    "   45: LOAD_IMM   r40, 0x80000000\n"
    "   46: AND        r41, r39, r40\n"
    "   47: BFEXT      r42, r39, 0, 23\n"
    "   48: SET_ALIAS  a9, r42\n"
    "   49: BFEXT      r43, r39, 23, 8\n"
    "   50: SET_ALIAS  a8, r43\n"
    "   51: GOTO_IF_Z  r43, L5\n"
    "   52: SEQI       r44, r43, 255\n"
    "   53: GOTO_IF_NZ r44, L6\n"
    "   54: GOTO_IF_NZ r41, L7\n"
    "   55: GOTO       L8\n"
    "   56: LABEL      L6\n"
    "   57: BFEXT      r45, r39, 0, 23\n"
    "   58: GOTO_IF_NZ r45, L9\n"
    "   59: GOTO_IF_NZ r41, L7\n"
    "   60: LOAD_IMM   r46, 0.0f\n"
    "   61: SET_ALIAS  a7, r46\n"
    "   62: LOAD_IMM   r47, 2\n"
    "   63: GET_ALIAS  r48, a4\n"
    "   64: BFINS      r49, r48, r47, 12, 7\n"
    "   65: SET_ALIAS  a4, r49\n"
    "   66: GOTO       L3\n"
    "   67: LABEL      L9\n"
    "   68: ANDI       r50, r42, 4194304\n"
    "   69: GOTO_IF_Z  r50, L10\n"
    "   70: SET_ALIAS  a7, r37\n"
    "   71: LOAD_IMM   r51, 17\n"
    "   72: GET_ALIAS  r52, a4\n"
    "   73: BFINS      r53, r52, r51, 12, 7\n"
    "   74: SET_ALIAS  a4, r53\n"
    "   75: GOTO       L3\n"
    "   76: LABEL      L10\n"
    "   77: GET_ALIAS  r54, a4\n"
    "   78: NOT        r55, r54\n"
    "   79: ORI        r56, r54, 16777216\n"
    "   80: ANDI       r57, r55, 16777216\n"
    "   81: SET_ALIAS  a4, r56\n"
    "   82: GOTO_IF_Z  r57, L11\n"
    "   83: ORI        r58, r56, -2147483648\n"
    "   84: SET_ALIAS  a4, r58\n"
    "   85: LABEL      L11\n"
    "   86: ANDI       r59, r38, 128\n"
    "   87: GOTO_IF_NZ r59, L4\n"
    "   88: LOAD_IMM   r60, 0x400000\n"
    "   89: OR         r61, r39, r60\n"
    "   90: BITCAST    r62, r61\n"
    "   91: SET_ALIAS  a7, r62\n"
    "   92: LOAD_IMM   r63, 17\n"
    "   93: GET_ALIAS  r64, a4\n"
    "   94: BFINS      r65, r64, r63, 12, 7\n"
    "   95: SET_ALIAS  a4, r65\n"
    "   96: GOTO       L3\n"
    "   97: LABEL      L5\n"
    "   98: BFEXT      r66, r39, 0, 23\n"
    "   99: GOTO_IF_NZ r66, L12\n"
    "  100: GET_ALIAS  r67, a4\n"
    "  101: NOT        r68, r67\n"
    "  102: ORI        r69, r67, 67108864\n"
    "  103: ANDI       r70, r68, 67108864\n"
    "  104: SET_ALIAS  a4, r69\n"
    "  105: GOTO_IF_Z  r70, L13\n"
    "  106: ORI        r71, r69, -2147483648\n"
    "  107: SET_ALIAS  a4, r71\n"
    "  108: LABEL      L13\n"
    "  109: ANDI       r72, r38, 16\n"
    "  110: GOTO_IF_NZ r72, L4\n"
    "  111: LOAD_IMM   r73, 0x7F800000\n"
    "  112: OR         r74, r41, r73\n"
    "  113: BITCAST    r75, r74\n"
    "  114: SET_ALIAS  a7, r75\n"
    "  115: LOAD_IMM   r76, 5\n"
    "  116: LOAD_IMM   r77, 9\n"
    "  117: SELECT     r78, r77, r76, r41\n"
    "  118: GET_ALIAS  r79, a4\n"
    "  119: BFINS      r80, r79, r78, 12, 7\n"
    "  120: SET_ALIAS  a4, r80\n"
    "  121: GOTO       L3\n"
    "  122: LABEL      L12\n"
    "  123: GOTO_IF_NZ r41, L7\n"
    "  124: CLZ        r81, r66\n"
    "  125: ADDI       r82, r81, -8\n"
    "  126: ADDI       r83, r81, -9\n"
    "  127: SLL        r84, r66, r82\n"
    "  128: NEG        r85, r83\n"
    "  129: SET_ALIAS  a8, r85\n"
    "  130: BFEXT      r86, r84, 0, 23\n"
    "  131: ZCAST      r87, r86\n"
    "  132: SET_ALIAS  a9, r87\n"
    "  133: LABEL      L8\n"
    "  134: LOAD       r88, 1016(r1)\n"
    "  135: GET_ALIAS  r89, a9\n"
    "  136: SRLI       r90, r89, 19\n"
    "  137: SLLI       r91, r90, 2\n"
    "  138: GET_ALIAS  r92, a8\n"
    "  139: ANDI       r93, r92, 1\n"
    "  140: XORI       r94, r93, 1\n"
    "  141: SLLI       r95, r94, 6\n"
    "  142: OR         r96, r91, r95\n"
    "  143: LOAD_IMM   r97, 380\n"
    "  144: SUB        r98, r97, r92\n"
    "  145: SRLI       r99, r98, 1\n"
    "  146: ZCAST      r100, r96\n"
    "  147: ADD        r101, r88, r100\n"
    "  148: LOAD_U16   r102, 2(r101)\n"
    "  149: LOAD_U16   r103, 0(r101)\n"
    "  150: BFEXT      r104, r89, 8, 11\n"
    "  151: MUL        r105, r104, r102\n"
    "  152: SLLI       r106, r103, 11\n"
    "  153: SUB        r107, r106, r105\n"
    "  154: SRLI       r108, r107, 3\n"
    "  155: SLLI       r109, r99, 23\n"
    "  156: OR         r110, r108, r41\n"
    "  157: OR         r111, r110, r109\n"
    "  158: BITCAST    r112, r111\n"
    "  159: SET_ALIAS  a7, r112\n"
    "  160: BITCAST    r113, r112\n"
    "  161: SGTUI      r114, r113, 0\n"
    "  162: SRLI       r115, r113, 31\n"
    "  163: BFEXT      r119, r113, 23, 8\n"
    "  164: SEQI       r116, r119, 0\n"
    "  165: SEQI       r117, r119, 255\n"
    "  166: SLLI       r120, r113, 9\n"
    "  167: SEQI       r118, r120, 0\n"
    "  168: AND        r121, r116, r118\n"
    "  169: XORI       r122, r118, 1\n"
    "  170: AND        r123, r117, r122\n"
    "  171: AND        r124, r116, r114\n"
    "  172: OR         r125, r124, r123\n"
    "  173: OR         r126, r121, r123\n"
    "  174: XORI       r127, r126, 1\n"
    "  175: XORI       r128, r115, 1\n"
    "  176: AND        r129, r115, r127\n"
    "  177: AND        r130, r128, r127\n"
    "  178: SLLI       r131, r125, 4\n"
    "  179: SLLI       r132, r129, 3\n"
    "  180: SLLI       r133, r130, 2\n"
    "  181: SLLI       r134, r121, 1\n"
    "  182: OR         r135, r131, r132\n"
    "  183: OR         r136, r133, r134\n"
    "  184: OR         r137, r135, r117\n"
    "  185: OR         r138, r137, r136\n"
    "  186: GET_ALIAS  r139, a4\n"
    "  187: BFINS      r140, r139, r138, 12, 7\n"
    "  188: SET_ALIAS  a4, r140\n"
    "  189: GOTO       L3\n"
    "  190: LABEL      L7\n"
    "  191: GET_ALIAS  r141, a4\n"
    "  192: NOT        r142, r141\n"
    "  193: ORI        r143, r141, 512\n"
    "  194: ANDI       r144, r142, 512\n"
    "  195: SET_ALIAS  a4, r143\n"
    "  196: GOTO_IF_Z  r144, L14\n"
    "  197: ORI        r145, r143, -2147483648\n"
    "  198: SET_ALIAS  a4, r145\n"
    "  199: LABEL      L14\n"
    "  200: ANDI       r146, r38, 128\n"
    "  201: GOTO_IF_NZ r146, L4\n"
    "  202: LOAD_IMM   r147, 17\n"
    "  203: GET_ALIAS  r148, a4\n"
    "  204: BFINS      r149, r148, r147, 12, 7\n"
    "  205: SET_ALIAS  a4, r149\n"
    "  206: LOAD_IMM   r150, nan(0x400000)\n"
    "  207: SET_ALIAS  a7, r150\n"
    "  208: GOTO       L3\n"
    "  209: LABEL      L4\n"
    "  210: GET_ALIAS  r151, a4\n"
    "  211: BFEXT      r152, r151, 12, 7\n"
    "  212: ANDI       r153, r152, 31\n"
    "  213: GET_ALIAS  r154, a4\n"
    "  214: BFINS      r155, r154, r153, 12, 7\n"
    "  215: SET_ALIAS  a4, r155\n"
    "  216: GOTO       L2\n"
    "  217: LABEL      L3\n"
    "  218: GOTO       L15\n"
    "  219: LABEL      L2\n"
    "  220: LOAD_IMM   r156, 1\n"
    "  221: SET_ALIAS  a6, r156\n"
    "  222: LABEL      L15\n"
    "  223: GET_ALIAS  r157, a4\n"
    "  224: BFEXT      r158, r157, 12, 7\n"
    "  225: GET_ALIAS  r159, a7\n"
    "  226: VEXTRACT   r160, r35, 1\n"
    "  227: GET_ALIAS  r161, a4\n"
    "  228: BITCAST    r162, r160\n"
    "  229: LOAD_IMM   r163, 0x80000000\n"
    "  230: AND        r164, r162, r163\n"
    "  231: BFEXT      r165, r162, 0, 23\n"
    "  232: SET_ALIAS  a12, r165\n"
    "  233: BFEXT      r166, r162, 23, 8\n"
    "  234: SET_ALIAS  a11, r166\n"
    "  235: GOTO_IF_Z  r166, L19\n"
    "  236: SEQI       r167, r166, 255\n"
    "  237: GOTO_IF_NZ r167, L20\n"
    "  238: GOTO_IF_NZ r164, L21\n"
    "  239: GOTO       L22\n"
    "  240: LABEL      L20\n"
    "  241: BFEXT      r168, r162, 0, 23\n"
    "  242: GOTO_IF_NZ r168, L23\n"
    "  243: GOTO_IF_NZ r164, L21\n"
    "  244: LOAD_IMM   r169, 0.0f\n"
    "  245: SET_ALIAS  a10, r169\n"
    "  246: LOAD_IMM   r170, 2\n"
    "  247: GET_ALIAS  r171, a4\n"
    "  248: BFINS      r172, r171, r170, 12, 7\n"
    "  249: SET_ALIAS  a4, r172\n"
    "  250: GOTO       L17\n"
    "  251: LABEL      L23\n"
    "  252: ANDI       r173, r165, 4194304\n"
    "  253: GOTO_IF_Z  r173, L24\n"
    "  254: SET_ALIAS  a10, r160\n"
    "  255: LOAD_IMM   r174, 17\n"
    "  256: GET_ALIAS  r175, a4\n"
    "  257: BFINS      r176, r175, r174, 12, 7\n"
    "  258: SET_ALIAS  a4, r176\n"
    "  259: GOTO       L17\n"
    "  260: LABEL      L24\n"
    "  261: GET_ALIAS  r177, a4\n"
    "  262: NOT        r178, r177\n"
    "  263: ORI        r179, r177, 16777216\n"
    "  264: ANDI       r180, r178, 16777216\n"
    "  265: SET_ALIAS  a4, r179\n"
    "  266: GOTO_IF_Z  r180, L25\n"
    "  267: ORI        r181, r179, -2147483648\n"
    "  268: SET_ALIAS  a4, r181\n"
    "  269: LABEL      L25\n"
    "  270: ANDI       r182, r161, 128\n"
    "  271: GOTO_IF_NZ r182, L18\n"
    "  272: LOAD_IMM   r183, 0x400000\n"
    "  273: OR         r184, r162, r183\n"
    "  274: BITCAST    r185, r184\n"
    "  275: SET_ALIAS  a10, r185\n"
    "  276: LOAD_IMM   r186, 17\n"
    "  277: GET_ALIAS  r187, a4\n"
    "  278: BFINS      r188, r187, r186, 12, 7\n"
    "  279: SET_ALIAS  a4, r188\n"
    "  280: GOTO       L17\n"
    "  281: LABEL      L19\n"
    "  282: BFEXT      r189, r162, 0, 23\n"
    "  283: GOTO_IF_NZ r189, L26\n"
    "  284: GET_ALIAS  r190, a4\n"
    "  285: NOT        r191, r190\n"
    "  286: ORI        r192, r190, 67108864\n"
    "  287: ANDI       r193, r191, 67108864\n"
    "  288: SET_ALIAS  a4, r192\n"
    "  289: GOTO_IF_Z  r193, L27\n"
    "  290: ORI        r194, r192, -2147483648\n"
    "  291: SET_ALIAS  a4, r194\n"
    "  292: LABEL      L27\n"
    "  293: ANDI       r195, r161, 16\n"
    "  294: GOTO_IF_NZ r195, L18\n"
    "  295: LOAD_IMM   r196, 0x7F800000\n"
    "  296: OR         r197, r164, r196\n"
    "  297: BITCAST    r198, r197\n"
    "  298: SET_ALIAS  a10, r198\n"
    "  299: LOAD_IMM   r199, 5\n"
    "  300: LOAD_IMM   r200, 9\n"
    "  301: SELECT     r201, r200, r199, r164\n"
    "  302: GET_ALIAS  r202, a4\n"
    "  303: BFINS      r203, r202, r201, 12, 7\n"
    "  304: SET_ALIAS  a4, r203\n"
    "  305: GOTO       L17\n"
    "  306: LABEL      L26\n"
    "  307: GOTO_IF_NZ r164, L21\n"
    "  308: CLZ        r204, r189\n"
    "  309: ADDI       r205, r204, -8\n"
    "  310: ADDI       r206, r204, -9\n"
    "  311: SLL        r207, r189, r205\n"
    "  312: NEG        r208, r206\n"
    "  313: SET_ALIAS  a11, r208\n"
    "  314: BFEXT      r209, r207, 0, 23\n"
    "  315: ZCAST      r210, r209\n"
    "  316: SET_ALIAS  a12, r210\n"
    "  317: LABEL      L22\n"
    "  318: LOAD       r211, 1016(r1)\n"
    "  319: GET_ALIAS  r212, a12\n"
    "  320: SRLI       r213, r212, 19\n"
    "  321: SLLI       r214, r213, 2\n"
    "  322: GET_ALIAS  r215, a11\n"
    "  323: ANDI       r216, r215, 1\n"
    "  324: XORI       r217, r216, 1\n"
    "  325: SLLI       r218, r217, 6\n"
    "  326: OR         r219, r214, r218\n"
    "  327: LOAD_IMM   r220, 380\n"
    "  328: SUB        r221, r220, r215\n"
    "  329: SRLI       r222, r221, 1\n"
    "  330: ZCAST      r223, r219\n"
    "  331: ADD        r224, r211, r223\n"
    "  332: LOAD_U16   r225, 2(r224)\n"
    "  333: LOAD_U16   r226, 0(r224)\n"
    "  334: BFEXT      r227, r212, 8, 11\n"
    "  335: MUL        r228, r227, r225\n"
    "  336: SLLI       r229, r226, 11\n"
    "  337: SUB        r230, r229, r228\n"
    "  338: SRLI       r231, r230, 3\n"
    "  339: SLLI       r232, r222, 23\n"
    "  340: OR         r233, r231, r164\n"
    "  341: OR         r234, r233, r232\n"
    "  342: BITCAST    r235, r234\n"
    "  343: SET_ALIAS  a10, r235\n"
    "  344: BITCAST    r236, r235\n"
    "  345: SGTUI      r237, r236, 0\n"
    "  346: SRLI       r238, r236, 31\n"
    "  347: BFEXT      r242, r236, 23, 8\n"
    "  348: SEQI       r239, r242, 0\n"
    "  349: SEQI       r240, r242, 255\n"
    "  350: SLLI       r243, r236, 9\n"
    "  351: SEQI       r241, r243, 0\n"
    "  352: AND        r244, r239, r241\n"
    "  353: XORI       r245, r241, 1\n"
    "  354: AND        r246, r240, r245\n"
    "  355: AND        r247, r239, r237\n"
    "  356: OR         r248, r247, r246\n"
    "  357: OR         r249, r244, r246\n"
    "  358: XORI       r250, r249, 1\n"
    "  359: XORI       r251, r238, 1\n"
    "  360: AND        r252, r238, r250\n"
    "  361: AND        r253, r251, r250\n"
    "  362: SLLI       r254, r248, 4\n"
    "  363: SLLI       r255, r252, 3\n"
    "  364: SLLI       r256, r253, 2\n"
    "  365: SLLI       r257, r244, 1\n"
    "  366: OR         r258, r254, r255\n"
    "  367: OR         r259, r256, r257\n"
    "  368: OR         r260, r258, r240\n"
    "  369: OR         r261, r260, r259\n"
    "  370: GET_ALIAS  r262, a4\n"
    "  371: BFINS      r263, r262, r261, 12, 7\n"
    "  372: SET_ALIAS  a4, r263\n"
    "  373: GOTO       L17\n"
    "  374: LABEL      L21\n"
    "  375: GET_ALIAS  r264, a4\n"
    "  376: NOT        r265, r264\n"
    "  377: ORI        r266, r264, 512\n"
    "  378: ANDI       r267, r265, 512\n"
    "  379: SET_ALIAS  a4, r266\n"
    "  380: GOTO_IF_Z  r267, L28\n"
    "  381: ORI        r268, r266, -2147483648\n"
    "  382: SET_ALIAS  a4, r268\n"
    "  383: LABEL      L28\n"
    "  384: ANDI       r269, r161, 128\n"
    "  385: GOTO_IF_NZ r269, L18\n"
    "  386: LOAD_IMM   r270, 17\n"
    "  387: GET_ALIAS  r271, a4\n"
    "  388: BFINS      r272, r271, r270, 12, 7\n"
    "  389: SET_ALIAS  a4, r272\n"
    "  390: LOAD_IMM   r273, nan(0x400000)\n"
    "  391: SET_ALIAS  a10, r273\n"
    "  392: GOTO       L17\n"
    "  393: LABEL      L18\n"
    "  394: GET_ALIAS  r274, a4\n"
    "  395: BFEXT      r275, r274, 12, 7\n"
    "  396: ANDI       r276, r275, 31\n"
    "  397: GET_ALIAS  r277, a4\n"
    "  398: BFINS      r278, r277, r276, 12, 7\n"
    "  399: SET_ALIAS  a4, r278\n"
    "  400: GOTO       L16\n"
    "  401: LABEL      L17\n"
    "  402: GOTO       L29\n"
    "  403: LABEL      L16\n"
    "  404: LOAD_IMM   r279, 1\n"
    "  405: SET_ALIAS  a6, r279\n"
    "  406: LABEL      L29\n"
    "  407: GET_ALIAS  r280, a4\n"
    "  408: BFINS      r281, r280, r158, 12, 7\n"
    "  409: SET_ALIAS  a4, r281\n"
    "  410: GET_ALIAS  r282, a10\n"
    "  411: GET_ALIAS  r283, a6\n"
    "  412: GOTO_IF_NZ r283, L30\n"
    "  413: VBUILD2    r284, r159, r282\n"
    "  414: VFCVT      r285, r284\n"
    "  415: SET_ALIAS  a2, r285\n"
    "  416: LABEL      L30\n"
    "  417: LOAD_IMM   r286, 4\n"
    "  418: SET_ALIAS  a1, r286\n"
    "  419: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "Alias 5: float32[2], no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: float32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: float32, no bound storage\n"
    "Alias 11: int32, no bound storage\n"
    "Alias 12: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,36] --> 2\n"
    "Block 2: 1,0 --> [37,51] --> 3,15\n"
    "Block 3: 2 --> [52,53] --> 4,6\n"
    "Block 4: 3 --> [54,54] --> 5,23\n"
    "Block 5: 4 --> [55,55] --> 22\n"
    "Block 6: 3 --> [56,58] --> 7,9\n"
    "Block 7: 6 --> [59,59] --> 8,23\n"
    "Block 8: 7 --> [60,66] --> 28\n"
    "Block 9: 6 --> [67,69] --> 10,11\n"
    "Block 10: 9 --> [70,75] --> 28\n"
    "Block 11: 9 --> [76,82] --> 12,13\n"
    "Block 12: 11 --> [83,84] --> 13\n"
    "Block 13: 12,11 --> [85,87] --> 14,27\n"
    "Block 14: 13 --> [88,96] --> 28\n"
    "Block 15: 2 --> [97,99] --> 16,20\n"
    "Block 16: 15 --> [100,105] --> 17,18\n"
    "Block 17: 16 --> [106,107] --> 18\n"
    "Block 18: 17,16 --> [108,110] --> 19,27\n"
    "Block 19: 18 --> [111,121] --> 28\n"
    "Block 20: 15 --> [122,123] --> 21,23\n"
    "Block 21: 20 --> [124,132] --> 22\n"
    "Block 22: 21,5 --> [133,189] --> 28\n"
    "Block 23: 4,7,20 --> [190,196] --> 24,25\n"
    "Block 24: 23 --> [197,198] --> 25\n"
    "Block 25: 24,23 --> [199,201] --> 26,27\n"
    "Block 26: 25 --> [202,208] --> 28\n"
    "Block 27: 13,18,25 --> [209,216] --> 29\n"
    "Block 28: 8,10,14,19,22,26 --> [217,218] --> 30\n"
    "Block 29: 27 --> [219,221] --> 30\n"
    "Block 30: 29,28 --> [222,235] --> 31,43\n"
    "Block 31: 30 --> [236,237] --> 32,34\n"
    "Block 32: 31 --> [238,238] --> 33,51\n"
    "Block 33: 32 --> [239,239] --> 50\n"
    "Block 34: 31 --> [240,242] --> 35,37\n"
    "Block 35: 34 --> [243,243] --> 36,51\n"
    "Block 36: 35 --> [244,250] --> 56\n"
    "Block 37: 34 --> [251,253] --> 38,39\n"
    "Block 38: 37 --> [254,259] --> 56\n"
    "Block 39: 37 --> [260,266] --> 40,41\n"
    "Block 40: 39 --> [267,268] --> 41\n"
    "Block 41: 40,39 --> [269,271] --> 42,55\n"
    "Block 42: 41 --> [272,280] --> 56\n"
    "Block 43: 30 --> [281,283] --> 44,48\n"
    "Block 44: 43 --> [284,289] --> 45,46\n"
    "Block 45: 44 --> [290,291] --> 46\n"
    "Block 46: 45,44 --> [292,294] --> 47,55\n"
    "Block 47: 46 --> [295,305] --> 56\n"
    "Block 48: 43 --> [306,307] --> 49,51\n"
    "Block 49: 48 --> [308,316] --> 50\n"
    "Block 50: 49,33 --> [317,373] --> 56\n"
    "Block 51: 32,35,48 --> [374,380] --> 52,53\n"
    "Block 52: 51 --> [381,382] --> 53\n"
    "Block 53: 52,51 --> [383,385] --> 54,55\n"
    "Block 54: 53 --> [386,392] --> 56\n"
    "Block 55: 41,46,53 --> [393,400] --> 57\n"
    "Block 56: 36,38,42,47,50,54 --> [401,402] --> 58\n"
    "Block 57: 55 --> [403,405] --> 58\n"
    "Block 58: 57,56 --> [406,412] --> 59,60\n"
    "Block 59: 58 --> [413,415] --> 60\n"
    "Block 60: 59,58 --> [416,419] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
