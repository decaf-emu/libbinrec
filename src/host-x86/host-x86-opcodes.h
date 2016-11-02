/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_HOST_X86_OPCODES_H
#define BINREC_HOST_X86_OPCODES_H

#include "src/common.h"
#include "src/rtl.h"

/*************************************************************************/
/**************************** x86 opcode list ****************************/
/*************************************************************************/

/**
 * X86Opcode:  Enumeration of x86 opcodes.  All primary x86-64 opcodes are
 * included in this enumeration; opcodes only valid in 32-bit mode are
 * noted in comments but omitted.  x87 (non-MMX/SSE) floating-point
 * instructions are not included.

 * Opcodes should be read from high-order to low-order byte within the
 * value, starting at the first nonzero byte.  (Multi-byte opcodes never
 * have 0x00 as the first byte.)  For example, X86OP_CVTDQ2PD has the value
 * 0xF30FE6, so it should be encoded as the three bytes 0xF3, 0x0F, 0xE6.
 *
 * Suffixes identifying specific operand formats follow the syntax given
 * in the Intel documentation (Software Developer's Manual volume 2,
 * appendix A, Opcode Map).
 *
 * Note that for instructions with mandatory prefixes (0x66/0xF2/0xF3),
 * any REX prefix must be inserted between the mandatory prefix and the
 * rest of the opcode.
 */
typedef enum X86Opcode {

    /* 1-byte opcodes and prefix bytes */

    X86OP_ADD_Eb_Gb     = 0x00,
    X86OP_ADD_Ev_Gv     = 0x01,
    X86OP_ADD_Gb_Eb     = 0x02,
    X86OP_ADD_Gv_Ev     = 0x03,
    X86OP_ADD_AL_Ib     = 0x04,
    X86OP_ADD_rAX_Iz    = 0x05,
    // 32-bit only:       0x06 = PUSH ES
    // 32-bit only:       0x07 = POP ES
    X86OP_OR_Eb_Gb      = 0x08,
    X86OP_OR_Ev_Gv      = 0x09,
    X86OP_OR_Gb_Eb      = 0x0A,
    X86OP_OR_Gv_Ev      = 0x0B,
    X86OP_OR_AL_Ib      = 0x0C,
    X86OP_OR_rAX_Iz     = 0x0D,
    // 32-bit only:       0x0E = PUSH CS
    // 2-byte escape:     0x0F

    X86OP_ADC_Eb_Gb     = 0x10,
    X86OP_ADC_Ev_Gv     = 0x11,
    X86OP_ADC_Gb_Eb     = 0x12,
    X86OP_ADC_Gv_Ev     = 0x13,
    X86OP_ADC_AL_Ib     = 0x14,
    X86OP_ADC_rAX_Iz    = 0x15,
    // 32-bit only:       0x16 = PUSH SS
    // 32-bit only:       0x17 = POP SS
    X86OP_SBB_Eb_Gb     = 0x18,
    X86OP_SBB_Ev_Gv     = 0x19,
    X86OP_SBB_Gb_Eb     = 0x1A,
    X86OP_SBB_Gv_Ev     = 0x1B,
    X86OP_SBB_AL_Ib     = 0x1C,
    X86OP_SBB_rAX_Iz    = 0x1D,
    // 32-bit only:       0x1E = PUSH SS
    // 32-bit only:       0x1F = POP SS

    X86OP_AND_Eb_Gb     = 0x20,
    X86OP_AND_Ev_Gv     = 0x21,
    X86OP_AND_Gb_Eb     = 0x22,
    X86OP_AND_Gv_Ev     = 0x23,
    X86OP_AND_AL_Ib     = 0x24,
    X86OP_AND_rAX_Iz    = 0x25,
    X86OP_ES            = 0x26,
    // 32-bit only:       0x27 = DAA
    X86OP_SUB_Eb_Gb     = 0x28,
    X86OP_SUB_Ev_Gv     = 0x29,
    X86OP_SUB_Gb_Eb     = 0x2A,
    X86OP_SUB_Gv_Ev     = 0x2B,
    X86OP_SUB_AL_Ib     = 0x2C,
    X86OP_SUB_rAX_Iz    = 0x2D,
    X86OP_CS            = 0x2E,
    // 32-bit only:       0x2F = DAS

    X86OP_XOR_Eb_Gb     = 0x30,
    X86OP_XOR_Ev_Gv     = 0x31,
    X86OP_XOR_Gb_Eb     = 0x32,
    X86OP_XOR_Gv_Ev     = 0x33,
    X86OP_XOR_AL_Ib     = 0x34,
    X86OP_XOR_rAX_Iz    = 0x35,
    X86OP_SS            = 0x36,
    // 32-bit only:       0x37 = AAA
    X86OP_CMP_Eb_Gb     = 0x38,
    X86OP_CMP_Ev_Gv     = 0x39,
    X86OP_CMP_Gb_Eb     = 0x3A,
    X86OP_CMP_Gv_Ev     = 0x3B,
    X86OP_CMP_AL_Ib     = 0x3C,
    X86OP_CMP_rAX_Iz    = 0x3D,
    X86OP_DS            = 0x3E,
    // 32-bit only:       0x3F = AAS

    X86OP_REX           = 0x40,  // 64-bit only
    X86OP_REX_B         = 0x41,  // 64-bit only
    X86OP_REX_X         = 0x42,  // 64-bit only
    X86OP_REX_XB        = 0x43,  // 64-bit only
    X86OP_REX_R         = 0x44,  // 64-bit only
    X86OP_REX_RB        = 0x45,  // 64-bit only
    X86OP_REX_RX        = 0x46,  // 64-bit only
    X86OP_REX_RXB       = 0x47,  // 64-bit only
    X86OP_REX_W         = 0x48,  // 64-bit only
    X86OP_REX_WB        = 0x49,  // 64-bit only
    X86OP_REX_WX        = 0x4A,  // 64-bit only
    X86OP_REX_WXB       = 0x4B,  // 64-bit only
    X86OP_REX_WR        = 0x4C,  // 64-bit only
    X86OP_REX_WRB       = 0x4D,  // 64-bit only
    X86OP_REX_WRX       = 0x4E,  // 64-bit only
    X86OP_REX_WRXB      = 0x4F,  // 64-bit only
    // 32-bit only:       0x40 = INC eAX
    // 32-bit only:       0x41 = INC eCX
    // 32-bit only:       0x42 = INC eDX
    // 32-bit only:       0x43 = INC eBX
    // 32-bit only:       0x44 = INC eSP
    // 32-bit only:       0x45 = INC eBP
    // 32-bit only:       0x46 = INC eSI
    // 32-bit only:       0x47 = INC eDI
    // 32-bit only:       0x48 = DEC eAX
    // 32-bit only:       0x49 = DEC eCX
    // 32-bit only:       0x4A = DEC eDX
    // 32-bit only:       0x4B = DEC eBX
    // 32-bit only:       0x4C = DEC eSP
    // 32-bit only:       0x4D = DEC eBP
    // 32-bit only:       0x4E = DEC eSI
    // 32-bit only:       0x4F = DEC eDI

    X86OP_PUSH_rAX      = 0x50,
    X86OP_PUSH_rCX      = 0x51,
    X86OP_PUSH_rDX      = 0x52,
    X86OP_PUSH_rBX      = 0x53,
    X86OP_PUSH_rSP      = 0x54,
    X86OP_PUSH_rBP      = 0x55,
    X86OP_PUSH_rSI      = 0x56,
    X86OP_PUSH_rDI      = 0x57,
    X86OP_POP_rAX       = 0x58,
    X86OP_POP_rCX       = 0x59,
    X86OP_POP_rDX       = 0x5A,
    X86OP_POP_rBX       = 0x5B,
    X86OP_POP_rSP       = 0x5C,
    X86OP_POP_rBP       = 0x5D,
    X86OP_POP_rSI       = 0x5E,
    X86OP_POP_rDI       = 0x5F,

    // 32-bit only:       0x60 = PUSHA
    // 32-bit only:       0x61 = POPA
    // 32-bit only:       0x62 = BOUND Gv,Ma
    X86OP_MOVSXD_Gv_Ev  = 0x63,
    // 32-bit only:       0x63 = ARPL Ew,Gw
    X86OP_FS            = 0x64,
    X86OP_GS            = 0x65,
    X86OP_OPERAND_SIZE  = 0x66,
    X86OP_ADDRESS_SIZE  = 0x67,
    X86OP_PUSH_Iz       = 0x68,
    X86OP_IMUL_Gv_Ev_Iz = 0x69,
    X86OP_PUSH_Ib       = 0x6A,
    X86OP_IMUL_Gv_Ev_Ib = 0x6B,
    X86OP_INSB          = 0x6C,
    X86OP_INSW          = 0x6D,  // Also INSD
    X86OP_OUTSB         = 0x6E,
    X86OP_OUTSW         = 0x6F,  // Also OUTSD

    X86OP_Jcc_Jb        = 0x70,
    X86OP_JO_Jb         = 0x70,
    X86OP_JNO_Jb        = 0x71,
    X86OP_JB_Jb         = 0x72,
    X86OP_JC_Jb         = X86OP_JB_Jb,
    X86OP_JAE_Jb        = 0x73,
    X86OP_JNC_Jb        = X86OP_JAE_Jb,
    X86OP_JZ_Jb         = 0x74,
    X86OP_JNZ_Jb        = 0x75,
    X86OP_JBE_Jb        = 0x76,
    X86OP_JA_Jb         = 0x77,
    X86OP_JS_Jb         = 0x78,
    X86OP_JNS_Jb        = 0x79,
    X86OP_JP_Jb         = 0x7A,
    X86OP_JNP_Jb        = 0x7B,
    X86OP_JL_Jb         = 0x7C,
    X86OP_JGE_Jb        = 0x7D,
    X86OP_JLE_Jb        = 0x7E,
    X86OP_JG_Jb         = 0x7F,

    X86OP_IMM_Eb_Ib     = 0x80,  // See X86ImmOpcode for ModR/M opcode field.
    X86OP_IMM_Ev_Iz     = 0x81,
    // 32-bit only:       0x82 = IMM Eb,Ib (alternate encoding)
    X86OP_IMM_Ev_Ib     = 0x83,
    X86OP_TEST_Eb_Gb    = 0x84,
    X86OP_TEST_Ev_Gv    = 0x85,
    X86OP_XCHG_Eb_Gb    = 0x86,
    X86OP_XCHG_Ev_Gv    = 0x87,
    X86OP_MOV_Eb_Gb     = 0x88,
    X86OP_MOV_Ev_Gv     = 0x89,
    X86OP_MOV_Gb_Eb     = 0x8A,
    X86OP_MOV_Gv_Ev     = 0x8B,
    X86OP_MOV_Ev_Sw     = 0x8C,
    X86OP_LEA_Gv_M      = 0x8D,
    X86OP_MOV_Sw_Ew     = 0x8E,
    X86OP_POP_Ev        = 0x8F,  // Set opcode field of ModR/M byte to 0.

    X86OP_XCHG_rAX_rAX  = 0x90,
    X86OP_NOP           = X86OP_XCHG_rAX_rAX,
    X86OP_XCHG_rAX_rCX  = 0x91,
    X86OP_XCHG_rAX_rDX  = 0x92,
    X86OP_XCHG_rAX_rBX  = 0x93,
    X86OP_XCHG_rAX_rSP  = 0x94,
    X86OP_XCHG_rAX_rBP  = 0x95,
    X86OP_XCHG_rAX_rSI  = 0x96,
    X86OP_XCHG_rAX_rDI  = 0x97,
    X86OP_CBW           = 0x98,  // Also CWDE, CDQE
    X86OP_CWD           = 0x99,  // Also CDQ, CQO
    // 32-bit only:       0x9A = CALLF Ap
    X86OP_WAIT          = 0x9B,
    X86OP_PUSHF         = 0x9C,
    X86OP_POPF          = 0x9D,
    X86OP_SAHF          = 0x9E,
    X86OP_LAHF          = 0x9F,

    X86OP_MOV_AL_Ob     = 0xA0,
    X86OP_MOV_rAX_Ov    = 0xA1,
    X86OP_MOV_Ob_AL     = 0xA2,
    X86OP_MOV_Ov_rAX    = 0xA3,
    X86OP_MOVSB         = 0xA4,
    X86OP_MOVSW         = 0xA5,  // Also MOVSD, MOVSQ
    X86OP_CMPSB         = 0xA6,
    X86OP_CMPSW         = 0xA7,  // Also CMPSD, CMPSQ
    X86OP_TEST_AL_Ib    = 0xA8,
    X86OP_TEST_rAX_Iz   = 0xA9,
    X86OP_STOSB         = 0xAA,
    X86OP_STOSW         = 0xAB,  // Also STOSD, STOSQ
    X86OP_LODSB         = 0xAC,
    X86OP_LODSW         = 0xAD,  // Also LODSD, LODSQ
    X86OP_SCASB         = 0xAE,
    X86OP_SCASW         = 0xAF,  // Also SCASD, SCASQ

    X86OP_MOV_AL_Ib     = 0xB0,
    X86OP_MOV_CL_Ib     = 0xB1,
    X86OP_MOV_DL_Ib     = 0xB2,
    X86OP_MOV_BL_Ib     = 0xB3,
    X86OP_MOV_AH_Ib     = 0xB4,
    X86OP_MOV_CH_Ib     = 0xB5,
    X86OP_MOV_DH_Ib     = 0xB6,
    X86OP_MOV_BH_Ib     = 0xB7,
    X86OP_MOV_rAX_Iv    = 0xB8,
    X86OP_MOV_rCX_Iv    = 0xB9,
    X86OP_MOV_rDX_Iv    = 0xBA,
    X86OP_MOV_rBX_Iv    = 0xBB,
    X86OP_MOV_rSP_Iv    = 0xBC,
    X86OP_MOV_rBP_Iv    = 0xBD,
    X86OP_MOV_rSI_Iv    = 0xBE,
    X86OP_MOV_rDI_Iv    = 0xBF,

    X86OP_SHIFT_Eb_Ib   = 0xC0,  // See X86ShiftOpcode for ModR/M opcode field.
    X86OP_SHIFT_Ev_Ib   = 0xC1,
    X86OP_RET_Iw        = 0xC2,
    X86OP_RET           = 0xC3,
    X86OP_VEX_C4        = 0xC4,
    // 32-bit only:       0xC4 = LES Gz,Mp
    X86OP_VEX_C5        = 0xC5,
    // 32-bit only:       0xC5 = LDS Gz,Mp
    X86OP_MOV_Eb_Ib     = 0xC6,  // Set opcode field of ModR/M byte to 0.
    X86OP_MOV_Ev_Iz     = 0xC7,  // Set opcode field of ModR/M byte to 0.
    X86OP_ENTER_Iw_Ib   = 0xC8,
    X86OP_LEAVE         = 0xC9,
    X86OP_RETF_Iw       = 0xCA,
    X86OP_RETF          = 0xCB,
    X86OP_INT3          = 0xCC,
    X86OP_INT_Ib        = 0xCD,
    // 32-bit only:       0xCE = INTO
    X86OP_IRET          = 0xCF,

    X86OP_SHIFT_Eb_1    = 0xD0,
    X86OP_SHIFT_Ev_1    = 0xD1,
    X86OP_SHIFT_Eb_CL   = 0xD2,
    X86OP_SHIFT_Ev_CL   = 0xD3,
    // 32-bit only:       0xD4 = AAM Ib
    // 32-bit only:       0xD5 = AAD Ib
    // Undefined:         0xD6
    X86OP_XLAT          = 0xD7,
    // x87 prefix:        0xD8
    // x87 prefix:        0xD9
    // x87 prefix:        0xDA
    // x87 prefix:        0xDB
    // x87 prefix:        0xDC
    // x87 prefix:        0xDD
    // x87 prefix:        0xDE
    // x87 prefix:        0xDF

    X86OP_LOOPNZ        = 0xE0,
    X86OP_LOOPZ         = 0xE1,
    X86OP_LOOP          = 0xE2,
    X86OP_JCXZ          = 0xE3,
    X86OP_IN_AL_Ib      = 0xE4,
    X86OP_IN_eAX_Ib     = 0xE5,
    X86OP_OUT_Ib_AL     = 0xE6,
    X86OP_OUT_Ib_eAX    = 0xE7,
    X86OP_CALL          = 0xE8,
    X86OP_JMP_Jz        = 0xE9,
    X86OP_JMPF          = 0xEA,
    X86OP_JMP_Jb        = 0xEB,
    X86OP_IN_AL_DX      = 0xEC,
    X86OP_IN_eAX_DX     = 0xED,
    X86OP_OUT_DX_AL     = 0xEE,
    X86OP_OUT_DX_eAX    = 0xEF,

    X86OP_LOCK          = 0xF0,
    // Undefined:         0xF1
    X86OP_REPNE         = 0xF2,  // Also starts multi-byte opcodes.
    X86OP_REP           = 0xF3,  // Also starts multi-byte opcodes.
    X86OP_HLT           = 0xF4,
    X86OP_CMC           = 0xF5,
    X86OP_UNARY_Eb      = 0xF6,
    X86OP_UNARY_Ev      = 0xF7,
    X86OP_CLC           = 0xF8,
    X86OP_STC           = 0xF9,
    X86OP_CLI           = 0xFA,
    X86OP_STI           = 0xFB,
    X86OP_CLD           = 0xFC,
    X86OP_STD           = 0xFD,
    X86OP_MISC_FE       = 0xFE,  // See X86MiscFEOpcode for ModR/M opcode.
    X86OP_MISC_FF       = 0xFF,  // See X86MiscFFOpcode for ModR/M opcode.

    /* 2-byte opcodes (0F xx) */

    X86OP_0F00          = 0x0F00,  // Various privileged instructions.
    X86OP_0F01          = 0x0F01,  // Various privileged instructions.
    X86OP_LAR_Gv_Ew     = 0x0F02,
    X86OP_LSL_Gv_Ew     = 0x0F03,
    X86OP_SYSCALL       = 0x0F05,
    X86OP_CLTS          = 0x0F06,
    X86OP_SYSRET        = 0x0F07,
    X86OP_INVD          = 0x0F08,
    X86OP_WBINVD        = 0x0F09,
    X86OP_UD2           = 0x0F0B,
    X86OP_PREFETCHW     = 0x0F0D,

    X86OP_MOVUPS_V_W    = 0x0F10,
    X86OP_MOVUPD_V_W    = 0x660F10,
    X86OP_MOVSS_V_W     = 0xF30F10,
    X86OP_MOVSD_V_W     = 0xF20F10,
    X86OP_MOVUPS_W_V    = 0x0F11,
    X86OP_MOVUPD_W_V    = 0x660F11,
    X86OP_MOVSS_W_V     = 0xF30F11,
    X86OP_MOVSD_W_V     = 0xF20F11,
    X86OP_MOVLPS_V_M    = 0x0F12,
    X86OP_MOVLPD_V_M    = 0x660F12,
    X86OP_MOVSLDUP      = 0xF30F12,
    X86OP_MOVDDUP       = 0xF20F12,
    X86OP_MOVLPS_M_V    = 0x0F13,
    X86OP_MOVLPD_M_V    = 0x660F13,
    X86OP_UNPCKLPS      = 0x0F14,
    X86OP_UNPCKLPD      = 0x660F14,
    X86OP_UNPCKHPS      = 0x0F15,
    X86OP_UNPCKHPD      = 0x660F15,
    X86OP_MOVHPS_V_M    = 0x0F16,
    X86OP_MOVHPD_V_M    = 0x660F16,
    X86OP_MOVSHDUP      = 0xF30F16,
    X86OP_MOVHPS_M_V    = 0x0F17,
    X86OP_MOVHPD_M_V    = 0x660F17,
    X86OP_PREFETCH      = 0x0F18,  // See X86PrefetchOpcode for ModR/M opcode.
    X86OP_NOP_Ev        = 0x0F1F,  // Set opcode field of ModR/M byte to 0.

    X86OP_MOV_Rd_Cd     = 0x0F20,
    X86OP_MOV_Rd_Dd     = 0x0F21,
    X86OP_MOV_Cd_Rd     = 0x0F22,
    X86OP_MOV_Dd_Rd     = 0x0F23,
    X86OP_MOVAPS_V_W    = 0x0F28,
    X86OP_MOVAPD_V_W    = 0x660F28,
    X86OP_MOVAPS_W_V    = 0x0F29,
    X86OP_MOVAPD_W_V    = 0x660F29,
    X86OP_CVTPI2PS      = 0x0F2A,
    X86OP_CVTPI2PD      = 0x660F2A,
    X86OP_CVTSI2SS      = 0xF30F2A,
    X86OP_CVTSI2SD      = 0xF20F2A,
    X86OP_MOVNTPS       = 0x0F2B,
    X86OP_MOVNTPD       = 0x660F2B,
    X86OP_CVTTPS2PI     = 0x0F2C,
    X86OP_CVTTPD2PI     = 0x660F2C,
    X86OP_CVTTSS2SI     = 0xF30F2C,
    X86OP_CVTTSD2SI     = 0xF20F2C,
    X86OP_CVTPS2PI      = 0x0F2D,
    X86OP_CVTPD2PI      = 0x660F2D,
    X86OP_CVTSS2SI      = 0xF30F2D,
    X86OP_CVTSD2SI      = 0xF20F2D,
    X86OP_UCOMISS       = 0x0F2E,
    X86OP_UCOMISD       = 0x660F2E,
    X86OP_COMISS        = 0x0F2F,
    X86OP_COMISD        = 0x660F2F,

    X86OP_WRMSR         = 0x0F30,
    X86OP_RDTSC         = 0x0F31,
    X86OP_RDMSR         = 0x0F32,
    X86OP_RDPMC         = 0x0F33,
    X86OP_SYSENTER      = 0x0F34,
    X86OP_SYSEXIT       = 0x0F35,
    X86OP_GETSEC        = 0x0F37,

    X86OP_CMOVcc        = 0x0F40,
    X86OP_CMOVO         = 0x0F40,
    X86OP_CMOVNO        = 0x0F41,
    X86OP_CMOVB         = 0x0F42,
    X86OP_CMOVAE        = 0x0F43,
    X86OP_CMOVZ         = 0x0F44,
    X86OP_CMOVNZ        = 0x0F45,
    X86OP_CMOVBE        = 0x0F46,
    X86OP_CMOVA         = 0x0F47,
    X86OP_CMOVS         = 0x0F48,
    X86OP_CMOVNS        = 0x0F49,
    X86OP_CMOVP         = 0x0F4A,
    X86OP_CMOVNP        = 0x0F4B,
    X86OP_CMOVL         = 0x0F4C,
    X86OP_CMOVGE        = 0x0F4D,
    X86OP_CMOVLE        = 0x0F4E,
    X86OP_CMOVG         = 0x0F4F,

    X86OP_MOVMSKPS      = 0x0F50,
    X86OP_MOVMSKPD      = 0x660F50,
    X86OP_SQRTPS        = 0x0F51,
    X86OP_SQRTPD        = 0x660F51,
    X86OP_SQRTSS        = 0xF30F51,
    X86OP_SQRTSD        = 0xF20F51,
    X86OP_RSQRTPS       = 0x0F52,
    X86OP_RSQRTSS       = 0xF30F52,
    X86OP_RCPPS         = 0x0F53,
    X86OP_RCPSS         = 0xF30F53,
    X86OP_ANDPS         = 0x0F54,
    X86OP_ANDPD         = 0x660F54,
    X86OP_ANDNPS        = 0x0F55,
    X86OP_ANDNPD        = 0x660F55,
    X86OP_ORPS          = 0x0F56,
    X86OP_ORPD          = 0x660F56,
    X86OP_XORPS         = 0x0F57,
    X86OP_XORPD         = 0x660F57,
    X86OP_ADDPS         = 0x0F58,
    X86OP_ADDPD         = 0x660F58,
    X86OP_ADDSS         = 0xF30F58,
    X86OP_ADDSD         = 0xF20F58,
    X86OP_MULPS         = 0x0F59,
    X86OP_MULPD         = 0x660F59,
    X86OP_MULSS         = 0xF30F59,
    X86OP_MULSD         = 0xF20F59,
    X86OP_CVTPS2PD      = 0x0F5A,
    X86OP_CVTPD2PS      = 0x660F5A,
    X86OP_CVTSS2SD      = 0xF30F5A,
    X86OP_CVTSD2SS      = 0xF20F5A,
    X86OP_CVTDQ2PS      = 0x0F5B,
    X86OP_CVTPS2DQ      = 0x660F5B,
    X86OP_CVTTPS2DQ     = 0xF30F5B,
    X86OP_SUBPS         = 0x0F5C,
    X86OP_SUBPD         = 0x660F5C,
    X86OP_SUBSS         = 0xF30F5C,
    X86OP_SUBSD         = 0xF20F5C,
    X86OP_MINPS         = 0x0F5D,
    X86OP_MINPD         = 0x660F5D,
    X86OP_MINSS         = 0xF30F5D,
    X86OP_MINSD         = 0xF20F5D,
    X86OP_DIVPS         = 0x0F5E,
    X86OP_DIVPD         = 0x660F5E,
    X86OP_DIVSS         = 0xF30F5E,
    X86OP_DIVSD         = 0xF20F5E,
    X86OP_MAXPS         = 0x0F5F,
    X86OP_MAXPD         = 0x660F5F,
    X86OP_MAXSS         = 0xF30F5F,
    X86OP_MAXSD         = 0xF20F5F,

    X86OP_PUNPCKLBW_P_Q = 0x0F60,
    X86OP_PUNPCKLBW_V_W = 0x660F60,
    X86OP_PUNPCKLWD_P_Q = 0x0F61,
    X86OP_PUNPCKLWD_V_W = 0x660F61,
    X86OP_PUNPCKLDQ_P_Q = 0x0F62,
    X86OP_PUNPCKLDQ_V_W = 0x660F62,
    X86OP_PACKSSWB_P_Q  = 0x0F63,
    X86OP_PACKSSWB_V_W  = 0x660F63,
    X86OP_PCMPGTB_P_Q   = 0x0F64,
    X86OP_PCMPGTB_V_W   = 0x660F64,
    X86OP_PCMPGTW_P_Q   = 0x0F65,
    X86OP_PCMPGTW_V_W   = 0x660F65,
    X86OP_PCMPGTD_P_Q   = 0x0F66,
    X86OP_PCMPGTD_V_W   = 0x660F66,
    X86OP_PACKUSWB_P_Q  = 0x0F67,
    X86OP_PACKUSWB_V_W  = 0x660F67,
    X86OP_PUNPCKHBW_P_Q = 0x0F68,
    X86OP_PUNPCKHBW_V_W = 0x660F68,
    X86OP_PUNPCKHWD_P_Q = 0x0F69,
    X86OP_PUNPCKHWD_V_W = 0x660F69,
    X86OP_PUNPCKHDQ_P_Q = 0x0F6A,
    X86OP_PUNPCKHDQ_V_W = 0x660F6A,
    X86OP_PACKSSDW_P_Q  = 0x0F6B,
    X86OP_PACKSSDW_V_W  = 0x660F6B,
    X86OP_PUNPCKLQDQ    = 0x660F6C,
    X86OP_PUNPCKHQDQ    = 0x660F6D,
    X86OP_MOVD_P_E      = 0x0F6E,
    X86OP_MOVD_V_E      = 0x660F6E,
    X86OP_MOVQ_P_Q      = 0x0F6F,
    X86OP_MOVDQA_V_W    = 0x660F6F,
    X86OP_MOVDQU_V_W    = 0xF30F6F,

    X86OP_PSHUFW        = 0x0F70,
    X86OP_PSHUFD        = 0x660F70,
    X86OP_PSHUFHW       = 0xF30F70,
    X86OP_PSHUFLW       = 0xF20F70,
    X86OP_PSHIFTW_N_I   = 0x0F71,    // See X86PshiftOpcode for ModR/M opcode.
    X86OP_PSHIFTW_U_I   = 0x660F71,  // See X86PshiftOpcode for ModR/M opcode.
    X86OP_PSHIFTD_N_I   = 0x0F72,    // See X86PshiftOpcode for ModR/M opcode.
    X86OP_PSHIFTD_U_I   = 0x660F72,  // See X86PshiftOpcode for ModR/M opcode.
    X86OP_PSHIFTQ_N_I   = 0x0F73,    // See X86PshiftOpcode for ModR/M opcode.
    X86OP_PSHIFTQ_U_I   = 0x660F73,  // See X86PshiftOpcode for ModR/M opcode.
    X86OP_PCMPEQB_P_Q   = 0x0F74,
    X86OP_PCMPEQB_V_W   = 0x660F74,
    X86OP_PCMPEQW_P_Q   = 0x0F75,
    X86OP_PCMPEQW_V_W   = 0x660F75,
    X86OP_PCMPEQD_P_Q   = 0x0F76,
    X86OP_PCMPEQD_V_W   = 0x660F76,
    X86OP_EMMS          = 0x0F77,  // VZEROUPPER/VZEROALL when in VEX form.
    X86OP_VMREAD        = 0x0F78,
    X86OP_VMWRITE       = 0x0F79,
    X86OP_HADDPD        = 0x660F7C,
    X86OP_HADDPS        = 0xF20F7C,
    X86OP_HSUBPD        = 0x660F7D,
    X86OP_HSUBPS        = 0xF20F7D,
    X86OP_MOVD_E_P      = 0x0F7E,
    X86OP_MOVD_E_V      = 0x660F7E,
    X86OP_MOVQ_V_W      = 0xF30F7E,
    X86OP_MOVQ_Q_P      = 0x0F7F,
    X86OP_MOVDQA_W_V    = 0x660F7F,
    X86OP_MOVDQU_W_V    = 0xF30F7F,

    X86OP_Jcc_Jz        = 0x0F80,
    X86OP_JO_Jz         = 0x0F80,
    X86OP_JNO_Jz        = 0x0F81,
    X86OP_JB_Jz         = 0x0F82,
    X86OP_JC_Jz         = X86OP_JB_Jz,
    X86OP_JAE_Jz        = 0x0F83,
    X86OP_JNC_Jz        = X86OP_JAE_Jz,
    X86OP_JZ_Jz         = 0x0F84,
    X86OP_JNZ_Jz        = 0x0F85,
    X86OP_JBE_Jz        = 0x0F86,
    X86OP_JA_Jz         = 0x0F87,
    X86OP_JS_Jz         = 0x0F88,
    X86OP_JNS_Jz        = 0x0F89,
    X86OP_JP_Jz         = 0x0F8A,
    X86OP_JNP_Jz        = 0x0F8B,
    X86OP_JL_Jz         = 0x0F8C,
    X86OP_JGE_Jz        = 0x0F8D,
    X86OP_JLE_Jz        = 0x0F8E,
    X86OP_JG_Jz         = 0x0F8F,

    X86OP_SETcc         = 0x0F90,  // Set opcode field of R/M byte to 0.
    X86OP_SETO          = 0x0F90,
    X86OP_SETNO         = 0x0F91,
    X86OP_SETB          = 0x0F92,
    X86OP_SETC          = X86OP_SETB,
    X86OP_SETAE         = 0x0F93,
    X86OP_SETNC         = X86OP_SETAE,
    X86OP_SETZ          = 0x0F94,
    X86OP_SETNZ         = 0x0F95,
    X86OP_SETBE         = 0x0F96,
    X86OP_SETA          = 0x0F97,
    X86OP_SETS          = 0x0F98,
    X86OP_SETNS         = 0x0F99,
    X86OP_SETP          = 0x0F9A,
    X86OP_SETNP         = 0x0F9B,
    X86OP_SETL          = 0x0F9C,
    X86OP_SETGE         = 0x0F9D,
    X86OP_SETLE         = 0x0F9E,
    X86OP_SETG          = 0x0F9F,

    X86OP_PUSH_FS       = 0x0FA0,
    X86OP_POP_FS        = 0x0FA1,
    X86OP_CPUID         = 0x0FA2,
    X86OP_BT_Ev_Gv      = 0x0FA3,
    X86OP_SHLD_Ev_Gv_Ib = 0x0FA4,
    X86OP_SHLD_Ev_Gv_CL = 0x0FA5,
    X86OP_PUSH_GS       = 0x0FA8,
    X86OP_POP_GS        = 0x0FA9,
    X86OP_RSM           = 0x0FAA,
    X86OP_BTS_Ev_Gv     = 0x0FAB,
    X86OP_SHRD_Ev_Gv_Ib = 0x0FAC,
    X86OP_SHRD_Ev_Gv_CL = 0x0FAD,
    X86OP_MISC_0FAE     = 0x0FAE,  // See X86Misc0FAEOpcode for ModR/M opcode.
    X86OP_MISC_F30FAE   = 0xF30FAE,  // See X86MiscF30FAEOpcode for ModR/M opcode.
    X86OP_IMUL_Gv_Ev    = 0x0FAF,

    X86OP_CMPXCHG_Eb_Gb = 0x0FB0,
    X86OP_CMPXCHG_Ev_Gv = 0x0FB1,
    X86OP_LSS           = 0x0FB2,
    X86OP_BTR_Ev_Gv     = 0x0FB3,
    X86OP_LFS           = 0x0FB4,
    X86OP_LGS           = 0x0FB5,
    X86OP_MOVZX_Gv_Eb   = 0x0FB6,
    X86OP_MOVZX_Gv_Ew   = 0x0FB7,
    X86OP_JMPE          = 0x0FB8,
    X86OP_POPCNT        = 0xF30FB8,
    X86OP_UD1           = 0x0FB9,  // Takes a ModR/M byte (ignored).
    X86OP_BTx_Ev_Ib     = 0x0FBA,  // See X86BitTestOpcode for ModR/M opcode.
    X86OP_BTC_Ev_Gv     = 0x0FBB,
    X86OP_BSF           = 0x0FBC,
    X86OP_TZCNT         = 0xF30FBC,
    X86OP_BSR           = 0x0FBD,
    X86OP_LZCNT         = 0xF30FBD,
    X86OP_MOVSX_Gv_Eb   = 0x0FBE,
    X86OP_MOVSX_Gv_Ew   = 0x0FBF,

    X86OP_XADD_Eb_Gb    = 0x0FC0,
    X86OP_XADD_Ev_Gv    = 0x0FC1,
    X86OP_CMPPS         = 0x0FC2,
    X86OP_CMPPD         = 0x660FC2,
    X86OP_CMPSS         = 0xF30FC2,
    X86OP_CMPSD         = 0xF20FC2,
    X86OP_MOVNTI        = 0x0FC3,
    X86OP_PINSRW_P      = 0x0FC4,
    X86OP_PINSRW_V      = 0x660FC4,
    X86OP_PEXTRW_P      = 0x0FC5,
    X86OP_PEXTRW_V      = 0x660FC5,
    X86OP_SHUFPS        = 0x0FC6,
    X86OP_SHUFPD        = 0x660FC6,
    X86OP_MISC_0FC7     = 0x0FC7,  // See X86Misc0FC7Opcode for ModR/M opcode.
    X86OP_VMCLEAR       = 0x660FC7,  // Set opcode field of ModR/M byte to 6.
    X86OP_VMXMON        = 0xF30FC7,  // Set opcode field of ModR/M byte to 6.
    X86OP_RDRAND        = 0xF20FC7,  // Set opcode field of ModR/M byte to 6.
    X86OP_RDSEED        = X86OP_RDRAND,  // Set opcode field of ModR/M to 7.
    X86OP_BSWAP_rAX     = 0x0FC8,
    X86OP_BSWAP_rCX     = 0x0FC9,
    X86OP_BSWAP_rDX     = 0x0FCA,
    X86OP_BSWAP_rBX     = 0x0FCB,
    X86OP_BSWAP_rSP     = 0x0FCC,
    X86OP_BSWAP_rBP     = 0x0FCD,
    X86OP_BSWAP_rSI     = 0x0FCE,
    X86OP_BSWAP_rDI     = 0x0FCF,

    X86OP_ADDSUBPD      = 0x660FD0,
    X86OP_ADDSUBPS      = 0xF20FD0,
    X86OP_PSRLW_P       = 0x0FD1,
    X86OP_PSRLW_V       = 0x660FD1,
    X86OP_PSRLD_P       = 0x0FD2,
    X86OP_PSRLD_V       = 0x660FD2,
    X86OP_PSRLQ_P       = 0x0FD3,
    X86OP_PSRLQ_V       = 0x660FD3,
    X86OP_PADDQ_P       = 0x0FD4,
    X86OP_PADDQ_V       = 0x660FD4,
    X86OP_PMULLW_P      = 0x0FD5,
    X86OP_PMULLW_V      = 0x660FD5,
    X86OP_MOVQ_W_V      = 0x660FD6,
    X86OP_MOVQ2DQ       = 0xF30FD6,
    X86OP_MOVDQ2Q       = 0xF20FD6,
    X86OP_PMOVMSK_P     = 0x0FD7,
    X86OP_PMOVMSK_V     = 0x660FD7,
    X86OP_PSUBUSB_P     = 0x0FD8,
    X86OP_PSUBUSB_V     = 0x660FD8,
    X86OP_PSUBUSW_P     = 0x0FD9,
    X86OP_PSUBUSW_V     = 0x660FD9,
    X86OP_PMINUB_P      = 0x0FDA,
    X86OP_PMINUB_V      = 0x660FDA,
    X86OP_PAND_P        = 0x0FDB,
    X86OP_PAND_V        = 0x660FDB,
    X86OP_PADDUSB_P     = 0x0FDC,
    X86OP_PADDUSB_V     = 0x660FDC,
    X86OP_PADDUSW_P     = 0x0FDD,
    X86OP_PADDUSW_V     = 0x660FDD,
    X86OP_PMAXUB_P      = 0x0FDE,
    X86OP_PMAXUB_V      = 0x660FDE,
    X86OP_PANDN_P       = 0x0FDF,
    X86OP_PANDN_V       = 0x660FDF,

    X86OP_PAVGB_P       = 0x0FE0,
    X86OP_PAVGB_V       = 0x660FE0,
    X86OP_PSRAW_P       = 0x0FE1,
    X86OP_PSRAW_V       = 0x660FE1,
    X86OP_PSRAD_P       = 0x0FE2,
    X86OP_PSRAD_V       = 0x660FE2,
    X86OP_PSRAQ_P       = 0x0FE3,
    X86OP_PSRAQ_V       = 0x660FE3,
    X86OP_PMULHUW_P     = 0x0FE4,
    X86OP_PMULHUW_V     = 0x660FE4,
    X86OP_PMULHW_P      = 0x0FE5,
    X86OP_PMULHW_V      = 0x660FE5,
    X86OP_CVTTPD2DQ     = 0x660FE6,
    X86OP_CVTDQ2PD      = 0xF30FE6,
    X86OP_CVTPD2DQ      = 0xF20FE6,
    X86OP_MOVNTQ_P      = 0x0FE7,
    X86OP_MOVNTQ_V      = 0x660FE7,
    X86OP_PSUBSB_P      = 0x0FE8,
    X86OP_PSUBSB_V      = 0x660FE8,
    X86OP_PSUBSW_P      = 0x0FE9,
    X86OP_PSUBSW_V      = 0x660FE9,
    X86OP_PMINSW_P      = 0x0FEA,
    X86OP_PMINSW_V      = 0x660FEA,
    X86OP_POR_P         = 0x0FEB,
    X86OP_POR_V         = 0x660FEB,
    X86OP_PADDSB_P      = 0x0FEC,
    X86OP_PADDSB_V      = 0x660FEC,
    X86OP_PADDSW_P      = 0x0FED,
    X86OP_PADDSW_V      = 0x660FED,
    X86OP_PMAXSW_P      = 0x0FEE,
    X86OP_PMAXSW_V      = 0x660FEE,
    X86OP_PXOR_P        = 0x0FEF,
    X86OP_PXOR_V        = 0x660FEF,

    X86OP_LDDQU_P       = 0xF20FF0,
    X86OP_PSLLW_P       = 0x0FF1,
    X86OP_PSLLW_V       = 0x660FF1,
    X86OP_PSLLD_P       = 0x0FF2,
    X86OP_PSLLD_V       = 0x660FF2,
    X86OP_PSLLQ_P       = 0x0FF3,
    X86OP_PSLLQ_V       = 0x660FF3,
    X86OP_PMULUDQ_P     = 0x0FF4,
    X86OP_PMULUDQ_V     = 0x660FF4,
    X86OP_PMADDWD_P     = 0x0FF5,
    X86OP_PMADDWD_V     = 0x660FF5,
    X86OP_PSADBW_P      = 0x0FF6,
    X86OP_PSADBW_V      = 0x660FF6,
    X86OP_MASKMOVQ      = 0x0FF7,
    X86OP_MASKMOVDQU    = 0x660FF7,
    X86OP_PSUBB_P       = 0x0FF8,
    X86OP_PSUBB_V       = 0x660FF8,
    X86OP_PSUBW_P       = 0x0FF9,
    X86OP_PSUBW_V       = 0x660FF9,
    X86OP_PSUBD_P       = 0x0FFA,
    X86OP_PSUBD_V       = 0x660FFA,
    X86OP_PSUBQ_P       = 0x0FFB,
    X86OP_PSUBQ_V       = 0x660FFB,
    X86OP_PADDB_P       = 0x0FFC,
    X86OP_PADDB_V       = 0x660FFC,
    X86OP_PADDW_P       = 0x0FFD,
    X86OP_PADDW_V       = 0x660FFD,
    X86OP_PADDD_P       = 0x0FFE,
    X86OP_PADDD_V       = 0x660FFE,

    /* 3-byte opcodes (0F xx yy) */

    X86OP_PSHUFB_P      = 0x0F3800,
    X86OP_PSHUFB_V      = 0x660F3800,
    X86OP_PHADDW_P      = 0x0F3801,
    X86OP_PHADDW_V      = 0x660F3801,
    X86OP_PHADDD_P      = 0x0F3802,
    X86OP_PHADDD_V      = 0x660F3802,
    X86OP_PHADDSW_P     = 0x0F3803,
    X86OP_PHADDSW_V     = 0x660F3803,
    X86OP_PMADDUBSW_P   = 0x0F3804,
    X86OP_PMADDUBSW_V   = 0x660F3804,
    X86OP_PHSUBW_P      = 0x0F3805,
    X86OP_PHSUBW_V      = 0x660F3805,
    X86OP_PHSUBD_P      = 0x0F3806,
    X86OP_PHSUBD_V      = 0x660F3806,
    X86OP_PHSUBSW_P     = 0x0F3807,
    X86OP_PHSUBSW_V     = 0x660F3807,
    X86OP_PSIGNB_P      = 0x0F3808,
    X86OP_PSIGNB_V      = 0x660F3808,
    X86OP_PSIGNW_P      = 0x0F3809,
    X86OP_PSIGNW_V      = 0x660F3809,
    X86OP_PSIGND_P      = 0x0F380A,
    X86OP_PSIGND_V      = 0x660F380A,
    X86OP_PMULHRSW_P    = 0x0F380B,
    X86OP_PMULHRSW_V    = 0x660F380B,
    X86OP_VPERMILPS_V   = 0x660F380C,  // VEX only.
    X86OP_VPERMILPD_V   = 0x660F380D,  // VEX only.
    X86OP_VTESTPS_V     = 0x660F380E,  // VEX only.
    X86OP_VTESTPD_V     = 0x660F380F,  // VEX only.

    X86OP_PBLENDVB      = 0x660F3810,
    X86OP_VCVTPH2PS     = 0x660F3813,  // VEX only.
    X86OP_BLENDVPS      = 0x660F3814,
    X86OP_BLENDVPD      = 0x660F3815,
    X86OP_VPERMPS       = 0x660F3816,  // VEX only.
    X86OP_PTEST         = 0x660F3817,
    X86OP_VBROADCASTSS  = 0x660F3818,  // VEX only.
    X86OP_VBROADCASTSD  = 0x660F3819,  // VEX only.
    X86OP_VBROADCASTF128= 0x660F381A,  // VEX only.
    X86OP_PABSB_P       = 0x0F381C,
    X86OP_PABSB_V       = 0x660F381C,
    X86OP_PABSW_P       = 0x0F381D,
    X86OP_PABSW_V       = 0x660F381D,
    X86OP_PABSD_P       = 0x0F381E,
    X86OP_PABSD_V       = 0x660F381E,

    X86OP_PMOVSXBW      = 0x660F3820,
    X86OP_PMOVSXBD      = 0x660F3821,
    X86OP_PMOVSXBQ      = 0x660F3822,
    X86OP_PMOVSXWD      = 0x660F3823,
    X86OP_PMOVSXWQ      = 0x660F3824,
    X86OP_PMOVSXDQ      = 0x660F3825,
    X86OP_PMULDQ        = 0x660F3828,
    X86OP_PCMPEQQ       = 0x660F3829,
    X86OP_MOVNTDQA      = 0x660F382A,
    X86OP_PACKUSDW      = 0x660F382B,
    X86OP_VMASKMOVPS_V_M= 0x660F382C,  // VEX only.
    X86OP_VMASKMOVPD_V_M= 0x660F382C,  // VEX only.
    X86OP_VMASKMOVPS_M_V= 0x660F382C,  // VEX only.
    X86OP_VMASKMOVPD_M_V= 0x660F382C,  // VEX only.

    X86OP_PMOVZXBW      = 0x660F3830,
    X86OP_PMOVZXBD      = 0x660F3831,
    X86OP_PMOVZXBQ      = 0x660F3832,
    X86OP_PMOVZXWD      = 0x660F3833,
    X86OP_PMOVZXWQ      = 0x660F3834,
    X86OP_PMOVZXDQ      = 0x660F3835,
    X86OP_VPERMD        = 0x660F3836,  // VEX only.
    X86OP_PCMPGTQ       = 0x660F3837,
    X86OP_PMINSB        = 0x660F3838,
    X86OP_PMINSD        = 0x660F3839,
    X86OP_PMINUW        = 0x660F383A,
    X86OP_PMINUD        = 0x660F383B,
    X86OP_PMAXSB        = 0x660F383C,
    X86OP_PMAXSD        = 0x660F383D,
    X86OP_PMAXUW        = 0x660F383E,
    X86OP_PMAXUD        = 0x660F383F,

    X86OP_PMULLD        = 0x660F3840,
    X86OP_PHMINPOSUW    = 0x660F3841,
    X86OP_VPSRLVD       = 0x660F3845,  // VEX only.
    X86OP_VPSRAVD       = 0x660F3846,  // VEX only.
    X86OP_VPSLLVD       = 0x660F3847,  // VEX only.

    X86OP_VPBROADCASTD  = 0x660F3858,  // VEX only.
    X86OP_VPBROADCASTQ  = 0x660F3859,  // VEX only.
    X86OP_VPBROADCASTI128=0x660F385A,  // VEX only.

    X86OP_VPBROADCASTB  = 0x660F3878,  // VEX only.
    X86OP_VPBROADCASTW  = 0x660F3879,  // VEX only.

    X86OP_INVEPT        = 0x660F3880,
    X86OP_INVVPID       = 0x660F3881,
    X86OP_INVPCID       = 0x660F3882,
    X86OP_VPMASKMOVD_V_M= 0x660F388C,  // VEX only.
    X86OP_VPMASKMOVD_M_V= 0x660F388E,  // VEX only.

    X86OP_VGATHERDD     = 0x660F3890,  // VEX only.
    X86OP_VGATHERQD     = 0x660F3891,  // VEX only.
    X86OP_VGATHERDPS    = 0x660F3892,  // VEX only.
    X86OP_VGATHERQPD    = 0x660F3893,  // VEX only.
    X86OP_VFMADDSUB132PS= 0x660F3896,  // VEX only.
    X86OP_VFMSUBADD132PS= 0x660F3897,  // VEX only.
    X86OP_VFMADD132PS   = 0x660F3898,  // VEX only.
    X86OP_VFMADD132SS   = 0x660F3899,  // VEX only.
    X86OP_VFMSUB132PS   = 0x660F389A,  // VEX only.
    X86OP_VFMSUB132SS   = 0x660F389B,  // VEX only.
    X86OP_VFNMADD132PS  = 0x660F389C,  // VEX only.
    X86OP_VFNMADD132SS  = 0x660F389D,  // VEX only.
    X86OP_VFNMSUB132PS  = 0x660F389E,  // VEX only.
    X86OP_VFNMSUB132SS  = 0x660F389F,  // VEX only.

    X86OP_VFMADDSUB213PS= 0x660F38A6,  // VEX only.
    X86OP_VFMSUBADD213PS= 0x660F38A7,  // VEX only.
    X86OP_VFMADD213PS   = 0x660F38A8,  // VEX only.
    X86OP_VFMADD213SS   = 0x660F38A9,  // VEX only.
    X86OP_VFMSUB213PS   = 0x660F38AA,  // VEX only.
    X86OP_VFMSUB213SS   = 0x660F38AB,  // VEX only.
    X86OP_VFNMADD213PS  = 0x660F38AC,  // VEX only.
    X86OP_VFNMADD213SS  = 0x660F38AD,  // VEX only.
    X86OP_VFNMSUB213PS  = 0x660F38AE,  // VEX only.
    X86OP_VFNMSUB213SS  = 0x660F38AF,  // VEX only.

    X86OP_VFMADDSUB231PS= 0x660F38B6,  // VEX only.
    X86OP_VFMSUBADD231PS= 0x660F38B7,  // VEX only.
    X86OP_VFMADD231PS   = 0x660F38B8,  // VEX only.
    X86OP_VFMADD231SS   = 0x660F38B9,  // VEX only.
    X86OP_VFMSUB231PS   = 0x660F38BA,  // VEX only.
    X86OP_VFMSUB231SS   = 0x660F38BB,  // VEX only.
    X86OP_VFNMADD231PS  = 0x660F38BC,  // VEX only.
    X86OP_VFNMADD231SS  = 0x660F38BD,  // VEX only.
    X86OP_VFNMSUB231PS  = 0x660F38BE,  // VEX only.
    X86OP_VFNMSUB231SS  = 0x660F38BF,  // VEX only.

    X86OP_VAESIMC       = 0x660F38DB,
    X86OP_VAESENC       = 0x660F38DC,
    X86OP_VAESENCLAST   = 0x660F38DD,
    X86OP_VAESDEC       = 0x660F38DE,
    X86OP_VAESDECLAST   = 0x660F38DF,

    X86OP_MOVBE_Gy_My   = 0x0F38F0,
    X86OP_MOVBE_Gw_Mw   = 0x660F38F0,
    X86OP_CRC32_Gd_Eb   = 0xF20F38F0,
    X86OP_MOVBE_My_Gy   = 0x0F38F1,
    X86OP_MOVBE_Mw_Gw   = 0x660F38F1,
    X86OP_CRC32_Gd_Ey   = 0xF20F38F0,  // 16-bit with additional 0x66 prefix.
    X86OP_ANDN_Gy_By_Ey = 0x0F38F2,  // VEX only.
    X86OP_BLS           = 0x0F38F3,  // VEX only.  See X86BLSOpcode for ModR/M.
    X86OP_BZHI          = 0x0F38F5,  // VEX only.
    X86OP_PEXT          = 0xF30F38F5,  // VEX only.
    X86OP_PDEP          = 0xF20F38F5,  // VEX only.
    X86OP_ADCX          = 0x660F38F6,  // VEX only.
    X86OP_ADOX          = 0xF30F38F6,  // VEX only.
    X86OP_MULX          = 0xF20F38F6,  // VEX only.
    X86OP_BEXTR         = 0x0F38F7,  // VEX only.
    X86OP_SHLX          = 0x660F38F7,  // VEX only.
    X86OP_SARX          = 0xF30F38F7,  // VEX only.
    X86OP_SHRX          = 0xF20F38F7,  // VEX only.

    X86OP_VPERMQ        = 0x660F3A00,  // VEX only.
    X86OP_VPERMPD       = 0x660F3A01,  // VEX only.
    X86OP_VPBLENDD      = 0x660F3A02,  // VEX only.
    X86OP_VPERMILPS     = 0x660F3A04,  // VEX only.
    X86OP_VPERMILPD     = 0x660F3A05,  // VEX only.
    X86OP_VPERM2F128    = 0x660F3A06,  // VEX only.
    X86OP_ROUNDPS       = 0x660F3A08,
    X86OP_ROUNDPD       = 0x660F3A09,
    X86OP_ROUNDSS       = 0x660F3A0A,
    X86OP_ROUNDSD       = 0x660F3A0B,
    X86OP_BLENDPS       = 0x660F3A0C,
    X86OP_BLENDPD       = 0x660F3A0D,
    X86OP_BLENDW        = 0x660F3A0E,
    X86OP_PALIGNR_P     = 0x0F3A0F,
    X86OP_PALIGNR_V     = 0x660F3A0F,

    X86OP_PEXTRB        = 0x660F3A14,
    X86OP_PEXTRW        = 0x660F3A15,
    X86OP_PEXTRD        = 0x660F3A16,
    X86OP_PEXTRACTPS    = 0x660F3A17,
    X86OP_VINSERTF128   = 0x660F3A18,  // VEX only.
    X86OP_VEXTRACTF128  = 0x660F3A19,  // VEX only.
    X86OP_VCVTPS2PH     = 0x660F3A1D,  // VEX only.

    X86OP_PINSRB        = 0x660F3A20,
    X86OP_PINSERTPS     = 0x660F3A21,
    X86OP_PINSRD        = 0x660F3A22,

    X86OP_VINSERTI128   = 0x660F3A38,  // VEX only.
    X86OP_VEXTRACTI128  = 0x660F3A39,  // VEX only.

    X86OP_DPPS          = 0x660F3A40,
    X86OP_DPPD          = 0x660F3A41,
    X86OP_MPSADBW       = 0x660F3A42,
    X86OP_CLMULQDQ      = 0x660F3A44,
    X86OP_VPERM2I128    = 0x660F3A46,  // VEX only.
    X86OP_VBLENDVPS     = 0x660F3A4A,  // VEX only.
    X86OP_VBLENDVPD     = 0x660F3A4A,  // VEX only.
    X86OP_VBLENDVB      = 0x660F3A4A,  // VEX only.

    X86OP_PCMPESTRM     = 0x660F3A60,
    X86OP_PCMPESTRI     = 0x660F3A61,
    X86OP_PCMPISTRM     = 0x660F3A62,
    X86OP_PCMPISTRI     = 0x660F3A63,

    X86OP_AESKEYGEN     = 0x660F3ADF,

    X86OP_RORX          = 0xF20F3AF0,  // VEX only.

} X86Opcode;

/* And you thought we were finished... */

/**
 * X86ImmOpcode:  Enumeration of sub-opcodes for the immediate opcode group
 * (1-byte opcodes 0x80 through 0x83).  Note that these match bits 3-5 of
 * the primary opcode for the equivalent non-immediate instructions (which
 * is also the bit location of the opcode field in the ModR/M byte).
 */
typedef enum X86ImmOpcode {
    X86OP_IMM_ADD = 0,
    X86OP_IMM_OR  = 1,
    X86OP_IMM_ADC = 2,
    X86OP_IMM_SBB = 3,
    X86OP_IMM_AND = 4,
    X86OP_IMM_SUB = 5,
    X86OP_IMM_XOR = 6,
    X86OP_IMM_CMP = 7,
} X86ImmOpcode;

/**
 * X86ShiftOpcode:  Enumeration of sub-opcodes for the shift/rotate opcode
 * group (1-byte opcodes 0xC0/0xC1 and 0xD0-0xD3).
 */
typedef enum X86ShiftOpcode {
    X86OP_SHIFT_ROL = 0,
    X86OP_SHIFT_ROR = 1,
    X86OP_SHIFT_RCL = 2,
    X86OP_SHIFT_RCR = 3,
    X86OP_SHIFT_SHL = 4,
    X86OP_SHIFT_SHR = 5,
    X86OP_SHIFT_SAR = 7,
} X86ShiftOpcode;

/**
 * X86UnaryOpcode:  Enumeration of sub-opcodes for the unary opcode group
 * (1-byte opcodes 0xF6/0xF7).
 */
typedef enum X86UnaryOpcode {
    X86OP_UNARY_TEST     = 0,
    X86OP_UNARY_NOT      = 2,
    X86OP_UNARY_NEG      = 3,
    X86OP_UNARY_MUL_rAX  = 4,
    X86OP_UNARY_IMUL_rAX = 5,
    X86OP_UNARY_DIV_rAX  = 6,
    X86OP_UNARY_IDIV_rAX = 7,
} X86UnaryOpcode;

/**
 * X86MiscFEOpcode:  Enumeration of sub-opcodes for opcode 0xFE.
 */
typedef enum X86MiscFEOpcode {
    X86OP_MISC_FE_INC_Eb = 0,
    X86OP_MISC_FE_DEC_Eb = 1,
} X86MiscFEOpcode;

/**
 * X86MiscFFOpcode:  Enumeration of sub-opcodes for opcode 0xFF.
 */
typedef enum X86MiscFFOpcode {
    X86OP_MISC_FF_INC_Ev  = 0,
    X86OP_MISC_FF_DEC_Ev  = 1,
    X86OP_MISC_FF_CALL_Ev = 2,  // 0xFF only.
    X86OP_MISC_FF_CALL_Ep = 3,  // 0xFF only.
    X86OP_MISC_FF_JMP_Ev  = 4,  // 0xFF only.
    X86OP_MISC_FF_JMP_Mp  = 5,  // 0xFF only.
    X86OP_MISC_FF_PUSH_Ev = 6,  // 0xFF only.
} X86MiscFFOpcode;

/**
 * X86PrefetchOpcode:  Enumeration of sub-opcodes for opcode 0x0F 0x18.
 */
typedef enum X86PrefetchOpcode {
    X86OP_PREFETCH_NTA  = 0,
    X86OP_PREFETCH_T0   = 1,
    X86OP_PREFETCH_T1   = 2,
    X86OP_PREFETCH_T2   = 3,
} X86PrefetchOpcode;

/**
 * X86PshiftOpcode:  Enumeration of sub-opcodes for the SSE shift/rotate
 * opcode group (2-byte opcodes 0x0F 0x71-0x73 with/without 0x66 prefix).
 */
typedef enum X86PshiftOpcode {
    X86OP_PSHIFT_SRL   = 2,
    X86OP_PSHIFT_SRLDQ = 3,  // PSHIFTQ XMMn (0x66 0x0F 0x73) only.
    X86OP_PSHIFT_SRA   = 4,
    X86OP_PSHIFT_SLL   = 6,
    X86OP_PSHIFT_SLLDQ = 7,  // PSHIFTQ XMMn (0x66 0x0F 0x73) only.
} X86PshiftOpcode;

/**
 * X86BitTestOpcode:  Enumeration of sub-opcodes for the SSE bit-test
 * opcode group (2-byte opcode 0x0F 0xBA).
 */
typedef enum X86BitTestOpcode {
    X86OP_BITTEST_BT  = 4,
    X86OP_BITTEST_BTS = 5,
    X86OP_BITTEST_BTR = 6,
    X86OP_BITTEST_BTC = 7,
} X86BitTestOpcode;

/**
 * X86Misc0FAEOpcode:  Enumeration of sub-opcodes for opcode 0x0F 0xAE.
 */
typedef enum X86Misc0FAEOpcode {
    X86OP_MISC0FAE_FXSAVE   = 0,
    X86OP_MISC0FAE_FXRSTOR  = 1,
    X86OP_MISC0FAE_LDMXCSR  = 2,
    X86OP_MISC0FAE_STMXCSR  = 3,
    X86OP_MISC0FAE_XSAVE    = 4,
    X86OP_MISC0FAE_XRSTOR   = 5,
    X86OP_MISC0FAE_XSAVEOPT = 6,
    X86OP_MISC0FAE_CLFLUSH  = 7,

    X86OP_MISC0FAE_LFENCE   = 5,  // When used with mod = 3.
    X86OP_MISC0FAE_MFENCE   = 6,  // When used with mod = 3.
    X86OP_MISC0FAE_SFENCE   = 7,  // When used with mod = 3.
} X86Misc0FAEOpcode;

/**
 * X86MiscF30FAEOpcode:  Enumeration of sub-opcodes for opcode 0xF3 0x0F 0xAE.
 */
typedef enum X86MiscF30FAEOpcode {
    X86OP_MISCF30FAE_RDFSBASE = 0,
    X86OP_MISCF30FAE_RDGSBASE = 1,
    X86OP_MISCF30FAE_WRFSBASE = 2,
    X86OP_MISCF30FAE_WRGSBASE = 3,
} X86MiscF30FAEOpcode;

/**
 * X86Misc0FC7Opcode:  Enumeration of sub-opcodes for opcode 0x0F 0xC7.
 */
typedef enum X86Misc0FC7Opcode {
    X86OP_MISC0FC7_CMPXCHG8B = 1,  // CMPXCHG16B if REX.W is set.
    X86OP_MISC0FC7_VMPTRLD   = 6,
    X86OP_MISC0FC7_VMPTRST   = 7,
} X86Misc0FC7Opcode;

/**
 * X86BLSOpcode:  Enumeration of sub-opcodes for the lowest-bit-set opcode
 * group (0x0F 0x38 0xF3).
 */
typedef enum X86BLSOpcode {
    X86OP_BLS_BLSR   = 1,
    X86OP_BLS_BLSMSK = 2,
    X86OP_BLS_BLSI   = 3,
} X86BLSOpcode;

/*************************************************************************/
/******* Other constants and utility functions for code generation *******/
/*************************************************************************/

/**
 * X86CondCode:  Constants for the low 4 bits of a conditional instruction
 * (Jcc, SETcc, etc.).
 */
typedef enum X86CondCode {
    X86CC_O  = 0x0,
    X86CC_NO = 0x1,
    X86CC_B  = 0x2,
    X86CC_C  = X86CC_B,
    X86CC_AE = 0x3,
    X86CC_NC = X86CC_AE,
    X86CC_E  = 0x4,
    X86CC_Z  = X86CC_E,
    X86CC_NE = 0x5,
    X86CC_NZ = X86CC_NE,
    X86CC_BE = 0x6,
    X86CC_A  = 0x7,
    X86CC_S  = 0x8,
    X86CC_NS = 0x9,
    X86CC_P  = 0xA,
    X86CC_NP = 0xB,
    X86CC_L  = 0xC,
    X86CC_GE = 0xD,
    X86CC_LE = 0xE,
    X86CC_G  = 0xF,
} X86CondCode;

/**
 * X86Mod:  Constants for the "mod" field of a ModR/M byte.
 */
typedef enum X86Mod {
    X86MOD_DISP0 = 0,   // [reg] (if R/M == SP: [SIB], if R/M == BP: disp32)
    X86MOD_DISP8 = 1,   // [reg]+disp8 (if R/M == SP: [SIB]+disp8)
    X86MOD_DISP32 = 2,  // [reg]+disp32 (if R/M == SP: [SIB]+disp32)
    X86MOD_REG = 3,     // reg
} X86Mod;

/* Value for the R/M field of a ModR/M byte indicating that a SIB byte
 * follows. */
enum { X86MODRM_SIB = 4 };

/* Value for the R/M field of a ModR/M byte selecting RIP-relative
 * address (assuming mod is X86MOD_DISP0). */
enum { X86MODRM_RIP_REL = 5 };

/* Value for the index field of a SIB byte indicating that no index is used. */
enum { X86SIB_NOINDEX = 4 };

/**
 * x86_ModRM:  Return a ModR/M byte with the given fields.
 */
static inline CONST_FUNCTION uint8_t x86_ModRM(
    X86Mod mod, int reg_opcode, int r_m)
{
    return mod<<6 | reg_opcode<<3 | r_m;
}

/**
 * x86_SIB:  Return a SIB byte with the given fields.
 */
static inline CONST_FUNCTION uint8_t x86_SIB(int scale, int index, int base)
{
    return scale<<6 | index<<3 | base;
}

/**
 * x86_REX:  Return a REX prefix byte with the given fields.
 */
static inline CONST_FUNCTION uint8_t x86_REX(int W, int R, int X, int B)
{
    return X86OP_REX | W<<3 | R<<2 | X<<1 | B;
}

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_HOST_X86_OPCODES_H
