/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_GUEST_PPC_DECODE_H
#define BINREC_GUEST_PPC_DECODE_H

/*************************************************************************/
/***************************** Opcode tables *****************************/
/*************************************************************************/

typedef enum PPCOpcode {
    OPCD_TWI    = 0x03,
    OPCD_MULLI  = 0x07,
    OPCD_SUBFIC = 0x08,
    OPCD_CMPLI  = 0x0A,
    OPCD_CMPI   = 0x0B,
    OPCD_ADDIC  = 0x0C,
    OPCD_ADDIC_ = 0x0D,
    OPCD_ADDI   = 0x0E,
    OPCD_ADDIS  = 0x0F,
    OPCD_BC     = 0x10,
    OPCD_SC     = 0x11,
    OPCD_B      = 0x12,
    OPCD_x13    = 0x13,
    OPCD_RLWIMI = 0x14,
    OPCD_RLWINM = 0x15,
    OPCD_RLWNM  = 0x17,
    OPCD_ORI    = 0x18,
    OPCD_ORIS   = 0x19,
    OPCD_XORI   = 0x1A,
    OPCD_XORIS  = 0x1B,
    OPCD_ANDI_  = 0x1C,
    OPCD_ANDIS_ = 0x1D,
    OPCD_x1F    = 0x1F,
    OPCD_LWZ    = 0x20,
    OPCD_LWZU   = 0x21,
    OPCD_LBZ    = 0x22,
    OPCD_LBZU   = 0x23,
    OPCD_STW    = 0x24,
    OPCD_STWU   = 0x25,
    OPCD_STB    = 0x26,
    OPCD_STBU   = 0x27,
    OPCD_LHZ    = 0x28,
    OPCD_LHZU   = 0x29,
    OPCD_LHA    = 0x2A,
    OPCD_LHAU   = 0x2B,
    OPCD_STH    = 0x2C,
    OPCD_STHU   = 0x2D,
    OPCD_LMW    = 0x2E,
    OPCD_STMW   = 0x2F,
    OPCD_LFS    = 0x30,
    OPCD_LFSU   = 0x31,
    OPCD_LFD    = 0x32,
    OPCD_LFDU   = 0x33,
    OPCD_STFS   = 0x34,
    OPCD_STFSU  = 0x35,
    OPCD_STFD   = 0x36,
    OPCD_STFDU  = 0x37,
    OPCD_x3B    = 0x3B,
    OPCD_x3F    = 0x3F,

    /* 750CL-specific opcodes */
    OPCD_x04    = 0x04,
} PPCOpcode;

typedef enum PPCExtendedOpcode13 {
    XO_MCRF   = 0x000,
    XO_BCLR   = 0x010,
    XO_CRNOR  = 0x021,
    XO_RFI    = 0x032,
    XO_CRANDC = 0x081,
    XO_ISYNC  = 0x096,
    XO_CRXOR  = 0x0C1,
    XO_CRNAND = 0x0E1,
    XO_CRAND  = 0x101,
    XO_CREQV  = 0x121,
    XO_CRORC  = 0x1A1,
    XO_CROR   = 0x1C1,
    XO_BCCTR  = 0x210,
} PPCExtendedOpcode13;

typedef enum PPCExtendedOpcode1F {
    XO_CMP    = 0x000,
    XO_CMPL   = 0x020,
    XO_MCRXR  = 0x200,

    XO_TW     = 0x004,

    XO_SUBFC  = 0x008,  // 9 bits + OE
    XO_SUBF   = 0x028,  // 9 bits + OE
    XO_NEG    = 0x068,  // 9 bits + OE
    XO_SUBFE  = 0x088,  // 9 bits + OE
    XO_SUBFZE = 0x0C8,  // 9 bits + OE
    XO_SUBFME = 0x0E8,  // 9 bits + OE

    XO_ADDC   = 0x00A,  // 9 bits + OE
    XO_ADDE   = 0x08A,  // 9 bits + OE
    XO_ADDZE  = 0x0CA,  // 9 bits + OE
    XO_ADDME  = 0x0EA,  // 9 bits + OE
    XO_ADD    = 0x10A,  // 9 bits + OE

    XO_MULHWU = 0x00B,
    XO_MULHW  = 0x04B,
    XO_MULLW  = 0x0EB,  // 9 bits + OE
    XO_DIVWU  = 0x1CB,  // 9 bits + OE
    XO_DIVW   = 0x1EB,  // 9 bits + OE

    XO_MTCRF  = 0x090,

    XO_MTMSR  = 0x092,
    XO_MTSR   = 0x0D2,
    XO_MTSRIN = 0x0F2,
    XO_TLBIE  = 0x132,

    XO_MFCR   = 0x013,
    XO_MFMSR  = 0x053,
    XO_MFSPR  = 0x153,
    XO_MFTB   = 0x173,
    XO_MTSPR  = 0x1D3,
    XO_MFSR   = 0x253,
    XO_MFSRIN = 0x293,

    XO_LWARX  = 0x014,

    XO_LSWX   = 0x215,
    XO_LSWI   = 0x255,
    XO_STSWX  = 0x295,
    XO_STSWI  = 0x2D5,

    XO_DCBST  = 0x036,
    XO_DCBF   = 0x056,
    XO_STWCX_ = 0x096,
    XO_DCBTST = 0x0F6,
    XO_DCBT   = 0x116,
    XO_ECIWX  = 0x136,
    XO_ECOWX  = 0x1B6,
    XO_DCBI   = 0x1D6,
    XO_LWBRX  = 0x216,
    XO_TLBSYNC= 0x236,
    XO_SYNC   = 0x256,
    XO_STWBRX = 0x296,
    XO_LHBRX  = 0x316,
    XO_EIEIO  = 0x356,
    XO_STHBRX = 0x396,
    XO_ICBI   = 0x3D6,
    XO_DCBZ   = 0x3F6,

    XO_LWZX   = 0x017,
    XO_LWZUX  = 0x037,
    XO_LBZX   = 0x057,
    XO_LBZUX  = 0x077,
    XO_STWX   = 0x097,
    XO_STWUX  = 0x0B7,
    XO_STBX   = 0x0D7,
    XO_STBUX  = 0x0F7,
    XO_LHZX   = 0x117,
    XO_LHZUX  = 0x137,
    XO_LHAX   = 0x157,
    XO_LHAUX  = 0x177,
    XO_STHX   = 0x197,
    XO_STHUX  = 0x1B7,
    XO_LFSX   = 0x217,
    XO_LFSUX  = 0x237,
    XO_LFDX   = 0x257,
    XO_LFDUX  = 0x277,
    XO_STFSX  = 0x297,
    XO_STFSUX = 0x2B7,
    XO_STFDX  = 0x2D7,
    XO_STFDUX = 0x2F7,
    XO_STFIWX = 0x3D7,

    XO_SLW    = 0x018,
    XO_SRW    = 0x218,
    XO_SRAW   = 0x318,
    XO_SRAWI  = 0x338,

    XO_CNTLZW = 0x01A,
    XO_EXTSH  = 0x39A,
    XO_EXTSB  = 0x3BA,

    XO_ANDX   = 0x01C,
    XO_ANDC   = 0x03C,
    XO_NOR    = 0x07C,
    XO_EQV    = 0x11C,
    XO_XOR    = 0x13C,
    XO_ORC    = 0x19C,
    XO_OR     = 0x1BC,
    XO_NAND   = 0x1DC,
} PPCExtendedOpcode1F;

typedef enum PPCExtendedOpcode3B {
    XO_FDIVS  = 0x12,
    XO_FSUBS  = 0x14,
    XO_FADDS  = 0x15,
    XO_FRES   = 0x18,
    XO_FMULS  = 0x19,
    XO_FMSUBS = 0x1C,
    XO_FMADDS = 0x1D,
    XO_FNMSUBS= 0x1E,
    XO_FNMADDS= 0x1F,
} PPCExtendedOpcode3B;

typedef enum PPCExtendedOpcode3F {
    XO_FCMPU  = 0x000,
    XO_FCMPO  = 0x020,
    XO_MCRFS  = 0x040,

    XO_MTFSB1 = 0x026,
    XO_MTFSB0 = 0x046,
    XO_MTFSFI = 0x086,

    XO_MFFS   = 0x247,
    XO_MTFSF  = 0x2C7,

    XO_FNEG   = 0x028,
    XO_FMR    = 0x048,
    XO_FNABS  = 0x088,
    XO_FABS   = 0x108,

    XO_FRSP   = 0x00C,

    XO_FCTIW  = 0x00E,

    XO_FCTIWZ = 0x00F,

    XO_FDIV   = 0x12,
    XO_FSUB   = 0x14,
    XO_FADD   = 0x15,
    XO_FSEL   = 0x17,
    XO_FMUL   = 0x19,
    XO_FRSQRTE= 0x1A,
    XO_FMSUB  = 0x1C,
    XO_FMADD  = 0x1D,
    XO_FNMSUB = 0x1E,
    XO_FNMADD = 0x1F,
} PPCExtendedOpcode3F;

/* 750CL-specific extended opcodes */
typedef enum PPCExtendedOpcode04_750CL {
    XO_PS_CMPU0   = 0x000,
    XO_PS_CMPO0   = 0x020,
    XO_PS_CMPU1   = 0x040,
    XO_PS_CMPO1   = 0x060,

    XO_PSQ_LX     = 0x06,  // 6 bits
    XO_PSQ_LUX    = 0x26,  // 6 bits

    XO_PSQ_STX    = 0x07,  // 6 bits
    XO_PSQ_STUX   = 0x27,  // 6 bits

    XO_PS_NEG     = 0x028,
    XO_PS_MR      = 0x048,
    XO_PS_NABS    = 0x088,
    XO_PS_ABS     = 0x108,

    XO_PS_SUM0    = 0x0A,
    XO_PS_SUM1    = 0x0B,
    XO_PS_MULS0   = 0x0C,
    XO_PS_MULS1   = 0x0D,
    XO_PS_MADDS0  = 0x0E,
    XO_PS_MADDS1  = 0x0F,

    XO_PS_MERGE00 = 0x210,
    XO_PS_MERGE01 = 0x230,
    XO_PS_MERGE10 = 0x250,
    XO_PS_MERGE11 = 0x270,

    XO_PS_DIV     = 0x12,
    XO_PS_SUB     = 0x14,
    XO_PS_ADD     = 0x15,

    XO_DCBZ_L     = 0x3F6,

    XO_PS_SEL     = 0x17,
    XO_PS_RES     = 0x18,
    XO_PS_MUL     = 0x19,
    XO_PS_RSQRTE  = 0x1A,
    XO_PS_MSUB    = 0x1C,
    XO_PS_MADD    = 0x1D,
    XO_PS_NMSUB   = 0x1E,
    XO_PS_NMADD   = 0x1F,
} PPCExtendedOpcode04_750CL;

/*************************************************************************/
/******************** Instruction decoding functions *********************/
/*************************************************************************/

/*
 * Each function here returns the value of the correpsonding field from
 * the passed-in 32-bit instruction word.  The field names generally
 * follow the naming conventions used in IBM's documentation.
 *
 * Functions which return signed values assume that the library execution
 * environment uses two's-complement representations for integer values.
 */

/* Primary and secondary opcodes */
static inline CONST_FUNCTION PPCOpcode get_OPCD(const uint32_t insn)
    {return (PPCOpcode)(insn >> 26);}
static inline CONST_FUNCTION unsigned int get_XO_10(const uint32_t insn)
    {return (insn >> 1) & 0x3FF;}
static inline CONST_FUNCTION unsigned int get_XO_5(const uint32_t insn)
    {return (insn >> 1) & 0x1F;}

/* Generic source/destination registers */
static inline CONST_FUNCTION unsigned int get_rD(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_rS(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_frD(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_frS(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_crbD(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_crfD(const uint32_t insn)
    {return (insn >> 23) & 0x7;}

static inline CONST_FUNCTION unsigned int get_rA(const uint32_t insn)
    {return (insn >> 16) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_frA(const uint32_t insn)
    {return (insn >> 16) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_crbA(const uint32_t insn)
    {return (insn >> 16) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_crfS(const uint32_t insn)
    {return (insn >> 18) & 0x7;}

static inline CONST_FUNCTION unsigned int get_rB(const uint32_t insn)
    {return (insn >> 11) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_frB(const uint32_t insn)
    {return (insn >> 11) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_crbB(const uint32_t insn)
    {return (insn >> 11) & 0x1F;}

static inline CONST_FUNCTION unsigned int get_frC(const uint32_t insn)
    {return (insn >> 6) & 0x1F;}

/* Shift/mask fields */
static inline CONST_FUNCTION unsigned int get_SH(const uint32_t insn)
    {return (insn >> 11) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_MB(const uint32_t insn)
    {return (insn >> 6) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_ME(const uint32_t insn)
    {return (insn >> 1) & 0x1F;}

/* Immediate/offset operands */
static inline CONST_FUNCTION int get_SIMM(const uint32_t insn)
    {return (int16_t)insn;}
static inline CONST_FUNCTION unsigned int get_UIMM(const uint32_t insn)
    {return (uint16_t)insn;}
static inline CONST_FUNCTION int get_d(const uint32_t insn)
    {return (int16_t)insn;}

/* Branch fields (note that displacements are returned as byte offsets,
 * not raw field values) */
static inline CONST_FUNCTION unsigned int get_BO(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_BI(const uint32_t insn)
    {return (insn >> 16) & 0x1F;}
static inline CONST_FUNCTION int get_BD(const uint32_t insn)
    {return (int16_t)(insn & ~3);}
static inline CONST_FUNCTION int32_t get_LI(const uint32_t insn)
    {return (int32_t)((insn & ~3) << 6) >> 6;}
static inline CONST_FUNCTION unsigned int get_AA(const uint32_t insn)
    {return (insn >> 1) & 0x1;}
static inline CONST_FUNCTION unsigned int get_LK(const uint32_t insn)
    {return insn & 0x1;}

/* Paired-single instruction fields */
static inline CONST_FUNCTION unsigned int get_I_17(const uint32_t insn)
    {return (insn >> 12) & 0x7;}
static inline CONST_FUNCTION unsigned int get_I_22(const uint32_t insn)
    {return (insn >> 7) & 0x7;}

/* SPR/TBR fields */
static inline CONST_FUNCTION unsigned int get_spr(const uint32_t insn)
    {return ((insn >> (11-5)) & 0x3E0) | ((insn >> 16) & 0x1F);}
static inline CONST_FUNCTION unsigned int get_tbr(const uint32_t insn)
    {return ((insn >> (11-5)) & 0x3E0) | ((insn >> 16) & 0x1F);}

/* Miscellaneous fields */
static inline CONST_FUNCTION unsigned int get_CRM(const uint32_t insn)
    {return (insn >> 12) & 0xFF;}
static inline CONST_FUNCTION unsigned int get_IMM(const uint32_t insn)
    {return (insn >> 12) & 0xF;}
static inline CONST_FUNCTION unsigned int get_L(const uint32_t insn)
    {return (insn >> 21) & 0x1;}
static inline CONST_FUNCTION unsigned int get_NB(const uint32_t insn)
    {return (insn >> 11) & 0x1F;}
static inline CONST_FUNCTION unsigned int get_OE(const uint32_t insn)
    {return (insn >> 10) & 0x1;}
static inline CONST_FUNCTION unsigned int get_Rc(const uint32_t insn)
    {return insn & 0x1;}
static inline CONST_FUNCTION unsigned int get_TO(const uint32_t insn)
    {return (insn >> 21) & 0x1F;}

/*************************************************************************/
/*************************** Utility functions ***************************/
/*************************************************************************/

/**
 * is_valid_opcode:  Return whether the given opcode (OPCD field) is valid
 * (corresponds to at least one valid instruction).
 */
static inline bool is_valid_opcode(PPCOpcode opcd)
{
    const uint64_t valid_opcodes = UINT64_C(0x88FFFFFFBFBFFD88);
    return (valid_opcodes & (UINT64_C(1) << opcd)) != 0;
}

/*-----------------------------------------------------------------------*/

/**
 * is_valid_insn:  Return whether the given instruction is valid.
 */
static inline bool is_valid_insn(uint32_t insn)
{
    #define TEST_BIT(mask, bit)  (((mask) & (1 << (bit))) != 0)

    const PPCOpcode opcd = get_OPCD(insn);
    switch (opcd) {
      case OPCD_x04:
        switch (get_XO_5(insn)) {
            case 0x00: return !(get_XO_10(insn) & 0x380);
            case 0x08: return TEST_BIT(0x00000116, get_XO_10(insn) >> 5);
            case 0x10: return TEST_BIT(0x000F0000, get_XO_10(insn) >> 5);
            case 0x16: return get_XO_10(insn) == XO_DCBZ_L;
            default:   return TEST_BIT(0xF7B4FCC0, get_XO_5(insn));
        }
        break;

      case OPCD_x13:
        switch (get_XO_5(insn)) {
            case 0x00: return get_XO_10(insn) == XO_MCRF;
            case 0x01: return TEST_BIT(0x000063D3, get_XO_10(insn) >> 5);
            case 0x10: return get_XO_10(insn) == XO_BCLR
                           || (get_XO_10(insn) == XO_BCCTR
                               && !(get_BO(insn) & 0x04));
            case 0x12: return get_XO_10(insn) == XO_RFI;
            case 0x16: return get_XO_10(insn) == XO_ISYNC;
            default:   return false;
        }
        break;

      case OPCD_x1F: {
        static const uint32_t valid_insns_x1F[32] = {
            0x00010003, 0x00000000, 0x00000000, 0x00000000,  // 0x00
            0x00000001, 0x00000000, 0x00000000, 0x00000000,  // 0x04
            0x00DB00DB, 0x00000000, 0x01D101D1, 0xC080C085,  // 0x08
            0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0C
            0x00000010, 0x00000000, 0x000002D0, 0x00144C05,  // 0x10
            0x00000001, 0x00550000, 0xD5176396, 0x40FF3FFF,  // 0x14
            0x03010001, 0x00000000, 0x30000001, 0x00000000,  // 0x18
            0x0000730B, 0x00000000, 0x00000000, 0x00000000,  // 0x1C
        };
        return TEST_BIT(valid_insns_x1F[get_XO_5(insn)], get_XO_10(insn) >> 5);
      }

      case OPCD_x3B:
        return TEST_BIT(0xF3340000, get_XO_5(insn));

      case OPCD_x3F:
        switch (get_XO_5(insn)) {
            case 0x00: return get_XO_10(insn) <= 0x040;
            case 0x06: return TEST_BIT(0x00000016, get_XO_10(insn) >> 5);
            case 0x07: return TEST_BIT(0x00440000, get_XO_10(insn) >> 5);
            case 0x08: return TEST_BIT(0x00000116, get_XO_10(insn) >> 5);
            case 0x0C: // fall through
            case 0x0E: // fall through
            case 0x0F: return (get_XO_10(insn) & 0x3E0) == 0;
            case 0x10: return TEST_BIT(0x000F0000, get_XO_10(insn) >> 5);
            case 0x16: return get_XO_10(insn) == XO_DCBZ_L;
            default:   return TEST_BIT(0xF6B40000, get_XO_5(insn));
        }
        break;

      default:
        return is_valid_opcode(opcd);
    }

    #undef TEST_BIT
}

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_GUEST_PPC_DECODE_H
