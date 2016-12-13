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
    0xC0,0x43,0x00,0x00,  // lfs f2,0(r3)
    0xC0,0x63,0x00,0x04,  // lfs f3,4(r3)
    0x10,0x22,0x18,0x24,  // ps_div f1,f2,f3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, 0(r5)\n"
    "    6: ZCAST      r7, r3\n"
    "    7: ADD        r8, r2, r7\n"
    "    8: LOAD_BR    r9, 4(r8)\n"
    "    9: VBROADCAST r10, r6\n"
    "   10: VBROADCAST r11, r9\n"
    "   11: FDIV       r12, r10, r11\n"
    "   12: LOAD_IMM   r13, 0x1000000\n"
    "   13: VEXTRACT   r14, r12, 1\n"
    "   14: BITCAST    r15, r14\n"
    "   15: SLLI       r16, r15, 1\n"
    "   16: SEQ        r17, r16, r13\n"
    "   17: GOTO_IF_NZ r17, L1\n"
    "   18: VEXTRACT   r18, r12, 0\n"
    "   19: BITCAST    r19, r18\n"
    "   20: SLLI       r20, r19, 1\n"
    "   21: SEQ        r21, r20, r13\n"
    "   22: GOTO_IF_Z  r21, L2\n"
    "   23: LABEL      L1\n"
    "   24: FGETSTATE  r22\n"
    "   25: FSETROUND  r23, r22, TRUNC\n"
    "   26: FSETSTATE  r23\n"
    "   27: FDIV       r24, r10, r11\n"
    "   28: FGETSTATE  r25\n"
    "   29: FCOPYROUND r26, r25, r22\n"
    "   30: FSETSTATE  r26\n"
    "   31: LABEL      L2\n"
    "   32: FGETSTATE  r27\n"
    "   33: FTESTEXC   r28, r27, INVALID\n"
    "   34: GOTO_IF_Z  r28, L3\n"
    "   35: GET_ALIAS  r29, a3\n"
    "   36: FCLEAREXC  r30, r27\n"
    "   37: FSETSTATE  r30\n"
    "   38: VEXTRACT   r31, r10, 0\n"
    "   39: VEXTRACT   r32, r11, 0\n"
    "   40: FDIV       r33, r31, r32\n"
    "   41: GET_ALIAS  r34, a6\n"
    "   42: FGETSTATE  r35\n"
    "   43: FCLEAREXC  r36, r35\n"
    "   44: FSETSTATE  r36\n"
    "   45: FTESTEXC   r37, r35, INVALID\n"
    "   46: GOTO_IF_Z  r37, L5\n"
    "   47: BITCAST    r38, r31\n"
    "   48: SLLI       r39, r38, 10\n"
    "   49: BFEXT      r40, r38, 22, 9\n"
    "   50: SEQI       r41, r40, 510\n"
    "   51: GOTO_IF_Z  r41, L7\n"
    "   52: GOTO_IF_NZ r39, L6\n"
    "   53: LABEL      L7\n"
    "   54: BITCAST    r42, r32\n"
    "   55: SLLI       r43, r42, 10\n"
    "   56: BFEXT      r44, r42, 22, 9\n"
    "   57: SEQI       r45, r44, 510\n"
    "   58: GOTO_IF_Z  r45, L8\n"
    "   59: GOTO_IF_NZ r43, L6\n"
    "   60: LABEL      L8\n"
    "   61: BITCAST    r46, r31\n"
    "   62: BITCAST    r47, r32\n"
    "   63: OR         r48, r46, r47\n"
    "   64: SLLI       r49, r48, 1\n"
    "   65: GOTO_IF_NZ r49, L9\n"
    "   66: NOT        r50, r34\n"
    "   67: ORI        r51, r34, 2097152\n"
    "   68: ANDI       r52, r50, 2097152\n"
    "   69: SET_ALIAS  a6, r51\n"
    "   70: GOTO_IF_Z  r52, L10\n"
    "   71: ORI        r53, r51, -2147483648\n"
    "   72: SET_ALIAS  a6, r53\n"
    "   73: LABEL      L10\n"
    "   74: GOTO       L11\n"
    "   75: LABEL      L9\n"
    "   76: NOT        r54, r34\n"
    "   77: ORI        r55, r34, 4194304\n"
    "   78: ANDI       r56, r54, 4194304\n"
    "   79: SET_ALIAS  a6, r55\n"
    "   80: GOTO_IF_Z  r56, L12\n"
    "   81: ORI        r57, r55, -2147483648\n"
    "   82: SET_ALIAS  a6, r57\n"
    "   83: LABEL      L12\n"
    "   84: LABEL      L11\n"
    "   85: ANDI       r58, r34, 128\n"
    "   86: GOTO_IF_Z  r58, L13\n"
    "   87: GET_ALIAS  r59, a6\n"
    "   88: BFEXT      r60, r59, 12, 7\n"
    "   89: ANDI       r61, r60, 31\n"
    "   90: GET_ALIAS  r62, a6\n"
    "   91: BFINS      r63, r62, r61, 12, 7\n"
    "   92: SET_ALIAS  a6, r63\n"
    "   93: GOTO       L4\n"
    "   94: LABEL      L13\n"
    "   95: LOAD_IMM   r64, nan(0x400000)\n"
    "   96: FCVT       r65, r64\n"
    "   97: VBROADCAST r66, r65\n"
    "   98: SET_ALIAS  a3, r66\n"
    "   99: LOAD_IMM   r67, 17\n"
    "  100: GET_ALIAS  r68, a6\n"
    "  101: BFINS      r69, r68, r67, 12, 7\n"
    "  102: SET_ALIAS  a6, r69\n"
    "  103: GOTO       L4\n"
    "  104: LABEL      L6\n"
    "  105: NOT        r70, r34\n"
    "  106: ORI        r71, r34, 16777216\n"
    "  107: ANDI       r72, r70, 16777216\n"
    "  108: SET_ALIAS  a6, r71\n"
    "  109: GOTO_IF_Z  r72, L14\n"
    "  110: ORI        r73, r71, -2147483648\n"
    "  111: SET_ALIAS  a6, r73\n"
    "  112: LABEL      L14\n"
    "  113: ANDI       r74, r34, 128\n"
    "  114: GOTO_IF_Z  r74, L5\n"
    "  115: LABEL      L15\n"
    "  116: GET_ALIAS  r75, a6\n"
    "  117: BFEXT      r76, r75, 12, 7\n"
    "  118: ANDI       r77, r76, 31\n"
    "  119: GET_ALIAS  r78, a6\n"
    "  120: BFINS      r79, r78, r77, 12, 7\n"
    "  121: SET_ALIAS  a6, r79\n"
    "  122: GOTO       L4\n"
    "  123: LABEL      L5\n"
    "  124: FTESTEXC   r80, r35, ZERO_DIVIDE\n"
    "  125: GOTO_IF_Z  r80, L16\n"
    "  126: NOT        r81, r34\n"
    "  127: ORI        r82, r34, 67108864\n"
    "  128: ANDI       r83, r81, 67108864\n"
    "  129: SET_ALIAS  a6, r82\n"
    "  130: GOTO_IF_Z  r83, L17\n"
    "  131: ORI        r84, r82, -2147483648\n"
    "  132: SET_ALIAS  a6, r84\n"
    "  133: LABEL      L17\n"
    "  134: ANDI       r85, r34, 16\n"
    "  135: GOTO_IF_NZ r85, L15\n"
    "  136: LABEL      L16\n"
    "  137: FCVT       r86, r33\n"
    "  138: VBROADCAST r87, r86\n"
    "  139: SET_ALIAS  a3, r87\n"
    "  140: BITCAST    r88, r33\n"
    "  141: SGTUI      r89, r88, 0\n"
    "  142: SRLI       r90, r88, 31\n"
    "  143: BFEXT      r94, r88, 23, 8\n"
    "  144: SEQI       r91, r94, 0\n"
    "  145: SEQI       r92, r94, 255\n"
    "  146: SLLI       r95, r88, 9\n"
    "  147: SEQI       r93, r95, 0\n"
    "  148: AND        r96, r91, r93\n"
    "  149: XORI       r97, r93, 1\n"
    "  150: AND        r98, r92, r97\n"
    "  151: AND        r99, r91, r89\n"
    "  152: OR         r100, r99, r98\n"
    "  153: OR         r101, r96, r98\n"
    "  154: XORI       r102, r101, 1\n"
    "  155: XORI       r103, r90, 1\n"
    "  156: AND        r104, r90, r102\n"
    "  157: AND        r105, r103, r102\n"
    "  158: SLLI       r106, r100, 4\n"
    "  159: SLLI       r107, r104, 3\n"
    "  160: SLLI       r108, r105, 2\n"
    "  161: SLLI       r109, r96, 1\n"
    "  162: OR         r110, r106, r107\n"
    "  163: OR         r111, r108, r109\n"
    "  164: OR         r112, r110, r92\n"
    "  165: OR         r113, r112, r111\n"
    "  166: FTESTEXC   r114, r35, INEXACT\n"
    "  167: SLLI       r115, r114, 5\n"
    "  168: OR         r116, r113, r115\n"
    "  169: GET_ALIAS  r117, a6\n"
    "  170: BFINS      r118, r117, r116, 12, 7\n"
    "  171: SET_ALIAS  a6, r118\n"
    "  172: GOTO_IF_Z  r114, L18\n"
    "  173: GET_ALIAS  r119, a6\n"
    "  174: NOT        r120, r119\n"
    "  175: ORI        r121, r119, 33554432\n"
    "  176: ANDI       r122, r120, 33554432\n"
    "  177: SET_ALIAS  a6, r121\n"
    "  178: GOTO_IF_Z  r122, L19\n"
    "  179: ORI        r123, r121, -2147483648\n"
    "  180: SET_ALIAS  a6, r123\n"
    "  181: LABEL      L19\n"
    "  182: LABEL      L18\n"
    "  183: FTESTEXC   r124, r35, OVERFLOW\n"
    "  184: GOTO_IF_Z  r124, L20\n"
    "  185: GET_ALIAS  r125, a6\n"
    "  186: NOT        r126, r125\n"
    "  187: ORI        r127, r125, 268435456\n"
    "  188: ANDI       r128, r126, 268435456\n"
    "  189: SET_ALIAS  a6, r127\n"
    "  190: GOTO_IF_Z  r128, L21\n"
    "  191: ORI        r129, r127, -2147483648\n"
    "  192: SET_ALIAS  a6, r129\n"
    "  193: LABEL      L21\n"
    "  194: LABEL      L20\n"
    "  195: FTESTEXC   r130, r35, UNDERFLOW\n"
    "  196: GOTO_IF_Z  r130, L4\n"
    "  197: GET_ALIAS  r131, a6\n"
    "  198: NOT        r132, r131\n"
    "  199: ORI        r133, r131, 134217728\n"
    "  200: ANDI       r134, r132, 134217728\n"
    "  201: SET_ALIAS  a6, r133\n"
    "  202: GOTO_IF_Z  r134, L22\n"
    "  203: ORI        r135, r133, -2147483648\n"
    "  204: SET_ALIAS  a6, r135\n"
    "  205: LABEL      L22\n"
    "  206: LABEL      L4\n"
    "  207: GET_ALIAS  r136, a3\n"
    "  208: VEXTRACT   r137, r136, 0\n"
    "  209: GET_ALIAS  r138, a6\n"
    "  210: BFEXT      r139, r138, 12, 7\n"
    "  211: VEXTRACT   r140, r10, 1\n"
    "  212: VEXTRACT   r141, r11, 1\n"
    "  213: FDIV       r142, r140, r141\n"
    "  214: GET_ALIAS  r143, a6\n"
    "  215: FGETSTATE  r144\n"
    "  216: FCLEAREXC  r145, r144\n"
    "  217: FSETSTATE  r145\n"
    "  218: FTESTEXC   r146, r144, INVALID\n"
    "  219: GOTO_IF_Z  r146, L24\n"
    "  220: BITCAST    r147, r140\n"
    "  221: SLLI       r148, r147, 10\n"
    "  222: BFEXT      r149, r147, 22, 9\n"
    "  223: SEQI       r150, r149, 510\n"
    "  224: GOTO_IF_Z  r150, L26\n"
    "  225: GOTO_IF_NZ r148, L25\n"
    "  226: LABEL      L26\n"
    "  227: BITCAST    r151, r141\n"
    "  228: SLLI       r152, r151, 10\n"
    "  229: BFEXT      r153, r151, 22, 9\n"
    "  230: SEQI       r154, r153, 510\n"
    "  231: GOTO_IF_Z  r154, L27\n"
    "  232: GOTO_IF_NZ r152, L25\n"
    "  233: LABEL      L27\n"
    "  234: BITCAST    r155, r140\n"
    "  235: BITCAST    r156, r141\n"
    "  236: OR         r157, r155, r156\n"
    "  237: SLLI       r158, r157, 1\n"
    "  238: GOTO_IF_NZ r158, L28\n"
    "  239: NOT        r159, r143\n"
    "  240: ORI        r160, r143, 2097152\n"
    "  241: ANDI       r161, r159, 2097152\n"
    "  242: SET_ALIAS  a6, r160\n"
    "  243: GOTO_IF_Z  r161, L29\n"
    "  244: ORI        r162, r160, -2147483648\n"
    "  245: SET_ALIAS  a6, r162\n"
    "  246: LABEL      L29\n"
    "  247: GOTO       L30\n"
    "  248: LABEL      L28\n"
    "  249: NOT        r163, r143\n"
    "  250: ORI        r164, r143, 4194304\n"
    "  251: ANDI       r165, r163, 4194304\n"
    "  252: SET_ALIAS  a6, r164\n"
    "  253: GOTO_IF_Z  r165, L31\n"
    "  254: ORI        r166, r164, -2147483648\n"
    "  255: SET_ALIAS  a6, r166\n"
    "  256: LABEL      L31\n"
    "  257: LABEL      L30\n"
    "  258: ANDI       r167, r143, 128\n"
    "  259: GOTO_IF_Z  r167, L32\n"
    "  260: GET_ALIAS  r168, a6\n"
    "  261: BFEXT      r169, r168, 12, 7\n"
    "  262: ANDI       r170, r169, 31\n"
    "  263: GET_ALIAS  r171, a6\n"
    "  264: BFINS      r172, r171, r170, 12, 7\n"
    "  265: SET_ALIAS  a6, r172\n"
    "  266: GOTO       L23\n"
    "  267: LABEL      L32\n"
    "  268: LOAD_IMM   r173, nan(0x400000)\n"
    "  269: FCVT       r174, r173\n"
    "  270: VBROADCAST r175, r174\n"
    "  271: SET_ALIAS  a3, r175\n"
    "  272: LOAD_IMM   r176, 17\n"
    "  273: GET_ALIAS  r177, a6\n"
    "  274: BFINS      r178, r177, r176, 12, 7\n"
    "  275: SET_ALIAS  a6, r178\n"
    "  276: GOTO       L23\n"
    "  277: LABEL      L25\n"
    "  278: NOT        r179, r143\n"
    "  279: ORI        r180, r143, 16777216\n"
    "  280: ANDI       r181, r179, 16777216\n"
    "  281: SET_ALIAS  a6, r180\n"
    "  282: GOTO_IF_Z  r181, L33\n"
    "  283: ORI        r182, r180, -2147483648\n"
    "  284: SET_ALIAS  a6, r182\n"
    "  285: LABEL      L33\n"
    "  286: ANDI       r183, r143, 128\n"
    "  287: GOTO_IF_Z  r183, L24\n"
    "  288: LABEL      L34\n"
    "  289: GET_ALIAS  r184, a6\n"
    "  290: BFEXT      r185, r184, 12, 7\n"
    "  291: ANDI       r186, r185, 31\n"
    "  292: GET_ALIAS  r187, a6\n"
    "  293: BFINS      r188, r187, r186, 12, 7\n"
    "  294: SET_ALIAS  a6, r188\n"
    "  295: GOTO       L23\n"
    "  296: LABEL      L24\n"
    "  297: FTESTEXC   r189, r144, ZERO_DIVIDE\n"
    "  298: GOTO_IF_Z  r189, L35\n"
    "  299: NOT        r190, r143\n"
    "  300: ORI        r191, r143, 67108864\n"
    "  301: ANDI       r192, r190, 67108864\n"
    "  302: SET_ALIAS  a6, r191\n"
    "  303: GOTO_IF_Z  r192, L36\n"
    "  304: ORI        r193, r191, -2147483648\n"
    "  305: SET_ALIAS  a6, r193\n"
    "  306: LABEL      L36\n"
    "  307: ANDI       r194, r143, 16\n"
    "  308: GOTO_IF_NZ r194, L34\n"
    "  309: LABEL      L35\n"
    "  310: FCVT       r195, r142\n"
    "  311: VBROADCAST r196, r195\n"
    "  312: SET_ALIAS  a3, r196\n"
    "  313: BITCAST    r197, r142\n"
    "  314: SGTUI      r198, r197, 0\n"
    "  315: SRLI       r199, r197, 31\n"
    "  316: BFEXT      r203, r197, 23, 8\n"
    "  317: SEQI       r200, r203, 0\n"
    "  318: SEQI       r201, r203, 255\n"
    "  319: SLLI       r204, r197, 9\n"
    "  320: SEQI       r202, r204, 0\n"
    "  321: AND        r205, r200, r202\n"
    "  322: XORI       r206, r202, 1\n"
    "  323: AND        r207, r201, r206\n"
    "  324: AND        r208, r200, r198\n"
    "  325: OR         r209, r208, r207\n"
    "  326: OR         r210, r205, r207\n"
    "  327: XORI       r211, r210, 1\n"
    "  328: XORI       r212, r199, 1\n"
    "  329: AND        r213, r199, r211\n"
    "  330: AND        r214, r212, r211\n"
    "  331: SLLI       r215, r209, 4\n"
    "  332: SLLI       r216, r213, 3\n"
    "  333: SLLI       r217, r214, 2\n"
    "  334: SLLI       r218, r205, 1\n"
    "  335: OR         r219, r215, r216\n"
    "  336: OR         r220, r217, r218\n"
    "  337: OR         r221, r219, r201\n"
    "  338: OR         r222, r221, r220\n"
    "  339: FTESTEXC   r223, r144, INEXACT\n"
    "  340: SLLI       r224, r223, 5\n"
    "  341: OR         r225, r222, r224\n"
    "  342: GET_ALIAS  r226, a6\n"
    "  343: BFINS      r227, r226, r225, 12, 7\n"
    "  344: SET_ALIAS  a6, r227\n"
    "  345: GOTO_IF_Z  r223, L37\n"
    "  346: GET_ALIAS  r228, a6\n"
    "  347: NOT        r229, r228\n"
    "  348: ORI        r230, r228, 33554432\n"
    "  349: ANDI       r231, r229, 33554432\n"
    "  350: SET_ALIAS  a6, r230\n"
    "  351: GOTO_IF_Z  r231, L38\n"
    "  352: ORI        r232, r230, -2147483648\n"
    "  353: SET_ALIAS  a6, r232\n"
    "  354: LABEL      L38\n"
    "  355: LABEL      L37\n"
    "  356: FTESTEXC   r233, r144, OVERFLOW\n"
    "  357: GOTO_IF_Z  r233, L39\n"
    "  358: GET_ALIAS  r234, a6\n"
    "  359: NOT        r235, r234\n"
    "  360: ORI        r236, r234, 268435456\n"
    "  361: ANDI       r237, r235, 268435456\n"
    "  362: SET_ALIAS  a6, r236\n"
    "  363: GOTO_IF_Z  r237, L40\n"
    "  364: ORI        r238, r236, -2147483648\n"
    "  365: SET_ALIAS  a6, r238\n"
    "  366: LABEL      L40\n"
    "  367: LABEL      L39\n"
    "  368: FTESTEXC   r239, r144, UNDERFLOW\n"
    "  369: GOTO_IF_Z  r239, L23\n"
    "  370: GET_ALIAS  r240, a6\n"
    "  371: NOT        r241, r240\n"
    "  372: ORI        r242, r240, 134217728\n"
    "  373: ANDI       r243, r241, 134217728\n"
    "  374: SET_ALIAS  a6, r242\n"
    "  375: GOTO_IF_Z  r243, L41\n"
    "  376: ORI        r244, r242, -2147483648\n"
    "  377: SET_ALIAS  a6, r244\n"
    "  378: LABEL      L41\n"
    "  379: LABEL      L23\n"
    "  380: GET_ALIAS  r245, a3\n"
    "  381: VEXTRACT   r246, r245, 0\n"
    "  382: GET_ALIAS  r247, a6\n"
    "  383: BFEXT      r248, r247, 12, 7\n"
    "  384: ANDI       r249, r248, 32\n"
    "  385: OR         r250, r139, r249\n"
    "  386: GET_ALIAS  r251, a6\n"
    "  387: BFINS      r252, r251, r250, 12, 7\n"
    "  388: SET_ALIAS  a6, r252\n"
    "  389: GET_ALIAS  r253, a6\n"
    "  390: ANDI       r254, r253, 128\n"
    "  391: GOTO_IF_Z  r254, L42\n"
    "  392: SET_ALIAS  a3, r29\n"
    "  393: GOTO       L43\n"
    "  394: LABEL      L42\n"
    "  395: VBUILD2    r255, r137, r246\n"
    "  396: SET_ALIAS  a3, r255\n"
    "  397: GOTO       L43\n"
    "  398: LABEL      L3\n"
    "  399: GET_ALIAS  r256, a6\n"
    "  400: FGETSTATE  r257\n"
    "  401: FCLEAREXC  r258, r257\n"
    "  402: FSETSTATE  r258\n"
    "  403: FTESTEXC   r259, r257, ZERO_DIVIDE\n"
    "  404: GOTO_IF_Z  r259, L45\n"
    "  405: NOT        r260, r256\n"
    "  406: ORI        r261, r256, 67108864\n"
    "  407: ANDI       r262, r260, 67108864\n"
    "  408: SET_ALIAS  a6, r261\n"
    "  409: GOTO_IF_Z  r262, L46\n"
    "  410: ORI        r263, r261, -2147483648\n"
    "  411: SET_ALIAS  a6, r263\n"
    "  412: LABEL      L46\n"
    "  413: ANDI       r264, r256, 16\n"
    "  414: GOTO_IF_Z  r264, L45\n"
    "  415: VEXTRACT   r265, r10, 0\n"
    "  416: VEXTRACT   r266, r10, 1\n"
    "  417: VEXTRACT   r267, r11, 0\n"
    "  418: VEXTRACT   r268, r11, 1\n"
    "  419: FDIV       r269, r265, r267\n"
    "  420: FGETSTATE  r270\n"
    "  421: FDIV       r271, r266, r268\n"
    "  422: FTESTEXC   r272, r270, ZERO_DIVIDE\n"
    "  423: BITCAST    r273, r269\n"
    "  424: SGTUI      r274, r273, 0\n"
    "  425: SRLI       r275, r273, 31\n"
    "  426: BFEXT      r279, r273, 23, 8\n"
    "  427: SEQI       r276, r279, 0\n"
    "  428: SEQI       r277, r279, 255\n"
    "  429: SLLI       r280, r273, 9\n"
    "  430: SEQI       r278, r280, 0\n"
    "  431: AND        r281, r276, r278\n"
    "  432: XORI       r282, r278, 1\n"
    "  433: AND        r283, r277, r282\n"
    "  434: AND        r284, r276, r274\n"
    "  435: OR         r285, r284, r283\n"
    "  436: OR         r286, r281, r283\n"
    "  437: XORI       r287, r286, 1\n"
    "  438: XORI       r288, r275, 1\n"
    "  439: AND        r289, r275, r287\n"
    "  440: AND        r290, r288, r287\n"
    "  441: SLLI       r291, r285, 4\n"
    "  442: SLLI       r292, r289, 3\n"
    "  443: SLLI       r293, r290, 2\n"
    "  444: SLLI       r294, r281, 1\n"
    "  445: OR         r295, r291, r292\n"
    "  446: OR         r296, r293, r294\n"
    "  447: OR         r297, r295, r277\n"
    "  448: OR         r298, r297, r296\n"
    "  449: LOAD_IMM   r299, 0\n"
    "  450: SELECT     r300, r299, r298, r272\n"
    "  451: FGETSTATE  r301\n"
    "  452: FSETSTATE  r258\n"
    "  453: FTESTEXC   r302, r301, INEXACT\n"
    "  454: GOTO_IF_Z  r302, L47\n"
    "  455: GET_ALIAS  r303, a6\n"
    "  456: NOT        r304, r303\n"
    "  457: ORI        r305, r303, 33554432\n"
    "  458: ANDI       r306, r304, 33554432\n"
    "  459: SET_ALIAS  a6, r305\n"
    "  460: GOTO_IF_Z  r306, L48\n"
    "  461: ORI        r307, r305, -2147483648\n"
    "  462: SET_ALIAS  a6, r307\n"
    "  463: LABEL      L48\n"
    "  464: LABEL      L47\n"
    "  465: FTESTEXC   r308, r301, OVERFLOW\n"
    "  466: GOTO_IF_Z  r308, L49\n"
    "  467: GET_ALIAS  r309, a6\n"
    "  468: NOT        r310, r309\n"
    "  469: ORI        r311, r309, 268435456\n"
    "  470: ANDI       r312, r310, 268435456\n"
    "  471: SET_ALIAS  a6, r311\n"
    "  472: GOTO_IF_Z  r312, L50\n"
    "  473: ORI        r313, r311, -2147483648\n"
    "  474: SET_ALIAS  a6, r313\n"
    "  475: LABEL      L50\n"
    "  476: LABEL      L49\n"
    "  477: FTESTEXC   r314, r301, UNDERFLOW\n"
    "  478: GOTO_IF_Z  r314, L51\n"
    "  479: GET_ALIAS  r315, a6\n"
    "  480: NOT        r316, r315\n"
    "  481: ORI        r317, r315, 134217728\n"
    "  482: ANDI       r318, r316, 134217728\n"
    "  483: SET_ALIAS  a6, r317\n"
    "  484: GOTO_IF_Z  r318, L52\n"
    "  485: ORI        r319, r317, -2147483648\n"
    "  486: SET_ALIAS  a6, r319\n"
    "  487: LABEL      L52\n"
    "  488: LABEL      L51\n"
    "  489: SLLI       r320, r302, 5\n"
    "  490: OR         r321, r320, r300\n"
    "  491: GET_ALIAS  r322, a6\n"
    "  492: BFINS      r323, r322, r321, 12, 7\n"
    "  493: SET_ALIAS  a6, r323\n"
    "  494: GOTO       L44\n"
    "  495: LABEL      L45\n"
    "  496: VFCVT      r324, r12\n"
    "  497: SET_ALIAS  a3, r324\n"
    "  498: VEXTRACT   r325, r12, 0\n"
    "  499: BITCAST    r326, r325\n"
    "  500: SGTUI      r327, r326, 0\n"
    "  501: SRLI       r328, r326, 31\n"
    "  502: BFEXT      r332, r326, 23, 8\n"
    "  503: SEQI       r329, r332, 0\n"
    "  504: SEQI       r330, r332, 255\n"
    "  505: SLLI       r333, r326, 9\n"
    "  506: SEQI       r331, r333, 0\n"
    "  507: AND        r334, r329, r331\n"
    "  508: XORI       r335, r331, 1\n"
    "  509: AND        r336, r330, r335\n"
    "  510: AND        r337, r329, r327\n"
    "  511: OR         r338, r337, r336\n"
    "  512: OR         r339, r334, r336\n"
    "  513: XORI       r340, r339, 1\n"
    "  514: XORI       r341, r328, 1\n"
    "  515: AND        r342, r328, r340\n"
    "  516: AND        r343, r341, r340\n"
    "  517: SLLI       r344, r338, 4\n"
    "  518: SLLI       r345, r342, 3\n"
    "  519: SLLI       r346, r343, 2\n"
    "  520: SLLI       r347, r334, 1\n"
    "  521: OR         r348, r344, r345\n"
    "  522: OR         r349, r346, r347\n"
    "  523: OR         r350, r348, r330\n"
    "  524: OR         r351, r350, r349\n"
    "  525: FTESTEXC   r352, r257, INEXACT\n"
    "  526: SLLI       r353, r352, 5\n"
    "  527: OR         r354, r351, r353\n"
    "  528: GET_ALIAS  r355, a6\n"
    "  529: BFINS      r356, r355, r354, 12, 7\n"
    "  530: SET_ALIAS  a6, r356\n"
    "  531: GOTO_IF_Z  r352, L53\n"
    "  532: GET_ALIAS  r357, a6\n"
    "  533: NOT        r358, r357\n"
    "  534: ORI        r359, r357, 33554432\n"
    "  535: ANDI       r360, r358, 33554432\n"
    "  536: SET_ALIAS  a6, r359\n"
    "  537: GOTO_IF_Z  r360, L54\n"
    "  538: ORI        r361, r359, -2147483648\n"
    "  539: SET_ALIAS  a6, r361\n"
    "  540: LABEL      L54\n"
    "  541: LABEL      L53\n"
    "  542: FTESTEXC   r362, r257, OVERFLOW\n"
    "  543: GOTO_IF_Z  r362, L55\n"
    "  544: GET_ALIAS  r363, a6\n"
    "  545: NOT        r364, r363\n"
    "  546: ORI        r365, r363, 268435456\n"
    "  547: ANDI       r366, r364, 268435456\n"
    "  548: SET_ALIAS  a6, r365\n"
    "  549: GOTO_IF_Z  r366, L56\n"
    "  550: ORI        r367, r365, -2147483648\n"
    "  551: SET_ALIAS  a6, r367\n"
    "  552: LABEL      L56\n"
    "  553: LABEL      L55\n"
    "  554: FTESTEXC   r368, r257, UNDERFLOW\n"
    "  555: GOTO_IF_Z  r368, L44\n"
    "  556: GET_ALIAS  r369, a6\n"
    "  557: NOT        r370, r369\n"
    "  558: ORI        r371, r369, 134217728\n"
    "  559: ANDI       r372, r370, 134217728\n"
    "  560: SET_ALIAS  a6, r371\n"
    "  561: GOTO_IF_Z  r372, L57\n"
    "  562: ORI        r373, r371, -2147483648\n"
    "  563: SET_ALIAS  a6, r373\n"
    "  564: LABEL      L57\n"
    "  565: LABEL      L44\n"
    "  566: LABEL      L43\n"
    "  567: FGETSTATE  r374\n"
    "  568: FCMP       r375, r6, r6, UN\n"
    "  569: FCVT       r376, r6\n"
    "  570: SET_ALIAS  a7, r376\n"
    "  571: GOTO_IF_Z  r375, L58\n"
    "  572: BITCAST    r377, r6\n"
    "  573: ANDI       r378, r377, 4194304\n"
    "  574: GOTO_IF_NZ r378, L58\n"
    "  575: BITCAST    r379, r376\n"
    "  576: LOAD_IMM   r380, 0x8000000000000\n"
    "  577: XOR        r381, r379, r380\n"
    "  578: BITCAST    r382, r381\n"
    "  579: SET_ALIAS  a7, r382\n"
    "  580: LABEL      L58\n"
    "  581: GET_ALIAS  r383, a7\n"
    "  582: VBROADCAST r384, r383\n"
    "  583: FSETSTATE  r374\n"
    "  584: SET_ALIAS  a4, r384\n"
    "  585: FGETSTATE  r385\n"
    "  586: FCMP       r386, r9, r9, UN\n"
    "  587: FCVT       r387, r9\n"
    "  588: SET_ALIAS  a8, r387\n"
    "  589: GOTO_IF_Z  r386, L59\n"
    "  590: BITCAST    r388, r9\n"
    "  591: ANDI       r389, r388, 4194304\n"
    "  592: GOTO_IF_NZ r389, L59\n"
    "  593: BITCAST    r390, r387\n"
    "  594: LOAD_IMM   r391, 0x8000000000000\n"
    "  595: XOR        r392, r390, r391\n"
    "  596: BITCAST    r393, r392\n"
    "  597: SET_ALIAS  a8, r393\n"
    "  598: LABEL      L59\n"
    "  599: GET_ALIAS  r394, a8\n"
    "  600: VBROADCAST r395, r394\n"
    "  601: FSETSTATE  r385\n"
    "  602: SET_ALIAS  a5, r395\n"
    "  603: LOAD_IMM   r396, 12\n"
    "  604: SET_ALIAS  a1, r396\n"
    "  605: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2] @ 416(r1)\n"
    "Alias 5: float64[2] @ 432(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "Alias 7: float64, no bound storage\n"
    "Alias 8: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,17] --> 1,2\n"
    "Block 1: 0 --> [18,22] --> 2,3\n"
    "Block 2: 1,0 --> [23,30] --> 3\n"
    "Block 3: 2,1 --> [31,34] --> 4,77\n"
    "Block 4: 3 --> [35,46] --> 5,23\n"
    "Block 5: 4 --> [47,51] --> 6,7\n"
    "Block 6: 5 --> [52,52] --> 7,19\n"
    "Block 7: 6,5 --> [53,58] --> 8,9\n"
    "Block 8: 7 --> [59,59] --> 9,19\n"
    "Block 9: 8,7 --> [60,65] --> 10,13\n"
    "Block 10: 9 --> [66,70] --> 11,12\n"
    "Block 11: 10 --> [71,72] --> 12\n"
    "Block 12: 11,10 --> [73,74] --> 16\n"
    "Block 13: 9 --> [75,80] --> 14,15\n"
    "Block 14: 13 --> [81,82] --> 15\n"
    "Block 15: 14,13 --> [83,83] --> 16\n"
    "Block 16: 15,12 --> [84,86] --> 17,18\n"
    "Block 17: 16 --> [87,93] --> 39\n"
    "Block 18: 16 --> [94,103] --> 39\n"
    "Block 19: 6,8 --> [104,109] --> 20,21\n"
    "Block 20: 19 --> [110,111] --> 21\n"
    "Block 21: 20,19 --> [112,114] --> 22,23\n"
    "Block 22: 21,26 --> [115,122] --> 39\n"
    "Block 23: 4,21 --> [123,125] --> 24,27\n"
    "Block 24: 23 --> [126,130] --> 25,26\n"
    "Block 25: 24 --> [131,132] --> 26\n"
    "Block 26: 25,24 --> [133,135] --> 27,22\n"
    "Block 27: 26,23 --> [136,172] --> 28,31\n"
    "Block 28: 27 --> [173,178] --> 29,30\n"
    "Block 29: 28 --> [179,180] --> 30\n"
    "Block 30: 29,28 --> [181,181] --> 31\n"
    "Block 31: 30,27 --> [182,184] --> 32,35\n"
    "Block 32: 31 --> [185,190] --> 33,34\n"
    "Block 33: 32 --> [191,192] --> 34\n"
    "Block 34: 33,32 --> [193,193] --> 35\n"
    "Block 35: 34,31 --> [194,196] --> 36,39\n"
    "Block 36: 35 --> [197,202] --> 37,38\n"
    "Block 37: 36 --> [203,204] --> 38\n"
    "Block 38: 37,36 --> [205,205] --> 39\n"
    "Block 39: 38,17,18,22,35 --> [206,219] --> 40,58\n"
    "Block 40: 39 --> [220,224] --> 41,42\n"
    "Block 41: 40 --> [225,225] --> 42,54\n"
    "Block 42: 41,40 --> [226,231] --> 43,44\n"
    "Block 43: 42 --> [232,232] --> 44,54\n"
    "Block 44: 43,42 --> [233,238] --> 45,48\n"
    "Block 45: 44 --> [239,243] --> 46,47\n"
    "Block 46: 45 --> [244,245] --> 47\n"
    "Block 47: 46,45 --> [246,247] --> 51\n"
    "Block 48: 44 --> [248,253] --> 49,50\n"
    "Block 49: 48 --> [254,255] --> 50\n"
    "Block 50: 49,48 --> [256,256] --> 51\n"
    "Block 51: 50,47 --> [257,259] --> 52,53\n"
    "Block 52: 51 --> [260,266] --> 74\n"
    "Block 53: 51 --> [267,276] --> 74\n"
    "Block 54: 41,43 --> [277,282] --> 55,56\n"
    "Block 55: 54 --> [283,284] --> 56\n"
    "Block 56: 55,54 --> [285,287] --> 57,58\n"
    "Block 57: 56,61 --> [288,295] --> 74\n"
    "Block 58: 39,56 --> [296,298] --> 59,62\n"
    "Block 59: 58 --> [299,303] --> 60,61\n"
    "Block 60: 59 --> [304,305] --> 61\n"
    "Block 61: 60,59 --> [306,308] --> 62,57\n"
    "Block 62: 61,58 --> [309,345] --> 63,66\n"
    "Block 63: 62 --> [346,351] --> 64,65\n"
    "Block 64: 63 --> [352,353] --> 65\n"
    "Block 65: 64,63 --> [354,354] --> 66\n"
    "Block 66: 65,62 --> [355,357] --> 67,70\n"
    "Block 67: 66 --> [358,363] --> 68,69\n"
    "Block 68: 67 --> [364,365] --> 69\n"
    "Block 69: 68,67 --> [366,366] --> 70\n"
    "Block 70: 69,66 --> [367,369] --> 71,74\n"
    "Block 71: 70 --> [370,375] --> 72,73\n"
    "Block 72: 71 --> [376,377] --> 73\n"
    "Block 73: 72,71 --> [378,378] --> 74\n"
    "Block 74: 73,52,53,57,70 --> [379,391] --> 75,76\n"
    "Block 75: 74 --> [392,393] --> 107\n"
    "Block 76: 74 --> [394,397] --> 107\n"
    "Block 77: 3 --> [398,404] --> 78,94\n"
    "Block 78: 77 --> [405,409] --> 79,80\n"
    "Block 79: 78 --> [410,411] --> 80\n"
    "Block 80: 79,78 --> [412,414] --> 81,94\n"
    "Block 81: 80 --> [415,454] --> 82,85\n"
    "Block 82: 81 --> [455,460] --> 83,84\n"
    "Block 83: 82 --> [461,462] --> 84\n"
    "Block 84: 83,82 --> [463,463] --> 85\n"
    "Block 85: 84,81 --> [464,466] --> 86,89\n"
    "Block 86: 85 --> [467,472] --> 87,88\n"
    "Block 87: 86 --> [473,474] --> 88\n"
    "Block 88: 87,86 --> [475,475] --> 89\n"
    "Block 89: 88,85 --> [476,478] --> 90,93\n"
    "Block 90: 89 --> [479,484] --> 91,92\n"
    "Block 91: 90 --> [485,486] --> 92\n"
    "Block 92: 91,90 --> [487,487] --> 93\n"
    "Block 93: 92,89 --> [488,494] --> 106\n"
    "Block 94: 77,80 --> [495,531] --> 95,98\n"
    "Block 95: 94 --> [532,537] --> 96,97\n"
    "Block 96: 95 --> [538,539] --> 97\n"
    "Block 97: 96,95 --> [540,540] --> 98\n"
    "Block 98: 97,94 --> [541,543] --> 99,102\n"
    "Block 99: 98 --> [544,549] --> 100,101\n"
    "Block 100: 99 --> [550,551] --> 101\n"
    "Block 101: 100,99 --> [552,552] --> 102\n"
    "Block 102: 101,98 --> [553,555] --> 103,106\n"
    "Block 103: 102 --> [556,561] --> 104,105\n"
    "Block 104: 103 --> [562,563] --> 105\n"
    "Block 105: 104,103 --> [564,564] --> 106\n"
    "Block 106: 105,93,102 --> [565,565] --> 107\n"
    "Block 107: 106,75,76 --> [566,571] --> 108,110\n"
    "Block 108: 107 --> [572,574] --> 109,110\n"
    "Block 109: 108 --> [575,579] --> 110\n"
    "Block 110: 109,107,108 --> [580,589] --> 111,113\n"
    "Block 111: 110 --> [590,592] --> 112,113\n"
    "Block 112: 111 --> [593,597] --> 113\n"
    "Block 113: 112,110,111 --> [598,605] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
