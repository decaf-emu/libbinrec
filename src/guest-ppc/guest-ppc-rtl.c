/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/bitutils.h"
#include "src/common.h"
#include "src/endian.h"
#include "src/guest-ppc/guest-ppc-decode.h"
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"

/*************************************************************************/
/********************** Low-level Utility routines ***********************/
/*************************************************************************/

/**
 * rtl_imm32:  Allocate and return a new RTL register of type INT32
 * containing the given immediate value.
 */
static inline int rtl_imm32(RTLUnit * const unit, uint32_t value)
{
    const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, value);
    return reg;
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_imm64:  Allocate and return a new RTL register of type INT64
 * containing the given immediate value.
 */
static inline int rtl_imm64(RTLUnit * const unit, uint64_t value)
{
    const int reg = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, value);
    return reg;
}

/*-----------------------------------------------------------------------*/

/**
 * fcast_32to64:  Convert an RTL register from FLOAT32 to FLOAT64,
 * optionally checking for SNaNs and preserving their non-quiet state.
 *
 * [Parameters]
 *     unit: RTLUnit to add operate on.
 *     reg: RTL register to convert.
 *     check_snan: True to check for SNaN inputs.
 * [Return value]
 *     RTL register holding the converted value.
 */
static int fcast_32to64(RTLUnit *unit, int reg, bool check_snan)
{
    int is_nan = 0;
    if (check_snan) {
        is_nan = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FCMP, is_nan, reg, reg, RTLFCMP_UN);
    }

    int new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_FCVT, new_reg, reg, 0, 0);

    if (check_snan) {
        int alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_reg, 0, alias);
        const int label_not_snan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, is_nan, 0, label_not_snan);

        const int bits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, bits, reg, 0, 0);
        const int is_qnan = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, is_qnan, bits, 0, 1<<22);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, is_qnan, 0, label_not_snan);

        const int bits64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, bits64, new_reg, 0, 0);
        const int quietbit = rtl_imm64(unit, UINT64_C(1)<<51);
        const int newbits64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_XOR, newbits64, bits64, quietbit, 0);
        const int newval = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_BITCAST, newval, newbits64, 0, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, newval, 0, alias);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_snan);
        new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, new_reg, 0, 0, alias);
    }

    return new_reg;
}

/*-----------------------------------------------------------------------*/

/**
 * fcast_64to32:  Convert an RTL register from FLOAT64 to FLOAT32,
 * optionally checking for SNaNs and preserving their non-quiet state.
 *
 * [Parameters]
 *     unit: RTLUnit to add operate on.
 *     reg: RTL register to convert.
 *     check_snan: True to check for SNaN inputs.
 * [Return value]
 *     RTL register holding the converted value.
 */
static int fcast_64to32(RTLUnit *unit, int reg, bool check_snan)
{
    int is_nan = 0;
    if (check_snan) {
        is_nan = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_FCMP, is_nan, reg, reg, RTLFCMP_UN);
    }

    int new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_FCVT, new_reg, reg, 0, 0);

    if (check_snan) {
        int alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_reg, 0, alias);
        const int label_not_snan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, is_nan, 0, label_not_snan);

        const int bits = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, bits, reg, 0, 0);
        const int is_qnan = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BFEXT, is_qnan, bits, 0, 51 | 1<<8);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, is_qnan, 0, label_not_snan);

        const int bits32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, bits32, new_reg, 0, 0);
        const int newbits32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, newbits32, bits32, 0, 1<<22);
        const int newval = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_BITCAST, newval, newbits32, 0, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, newval, 0, alias);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_snan);
        new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, new_reg, 0, 0, alias);
    }

    return new_reg;
}

/*-----------------------------------------------------------------------*/

/**
 * vfcast_32to64:  Convert an RTL register from V2_FLOAT32 to V2_FLOAT64,
 * optionally checking for SNaNs and preserving their non-quiet state.
 *
 * [Parameters]
 *     unit: RTLUnit to add operate on.
 *     reg: RTL register to convert.
 *     check_snan: True to check for SNaN inputs.
 * [Return value]
 *     RTL register holding the converted value.
 */
static int vfcast_32to64(RTLUnit *unit, int reg, bool check_snan)
{
    int nan_check = 0;
    if (check_snan) {
        nan_check = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_VFCMP, nan_check, reg, reg, RTLFCMP_UN);
    }

    int new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
    rtl_add_insn(unit, RTLOP_VFCVT, new_reg, reg, 0, 0);

    if (check_snan) {
        int alias = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_reg, 0, alias);
        const int label_not_nan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, nan_check, 0, label_not_nan);

        // FIXME: This could be done more efficiently if we had integer
        // vector types and an AND-complement instruction.
        const int ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_VEXTRACT, ps0, reg, 0, 0);
        const int ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_VEXTRACT, ps1, reg, 0, 1);
        const int nan0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SLLI, nan0, nan_check, 0, 32);
        const int nan1 = nan_check;
        const int bits0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, bits0, ps0, 0, 0);
        const int bits1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, bits1, ps1, 0, 0);
        const int notbits0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, notbits0, bits0, 0, 0);
        const int notbits1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, notbits1, bits1, 0, 0);
        const int quiet32_0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, quiet32_0, notbits0, 0, 1<<22);
        const int quiet32_1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, quiet32_1, notbits1, 0, 1<<22);
        const int result0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, result0, new_reg, 0, 0);
        const int result1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, result1, new_reg, 0, 1);
        const int temp64_0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_ZCAST, temp64_0, quiet32_0, 0, 0);
        const int temp64_1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_ZCAST, temp64_1, quiet32_1, 0, 0);
        const int quiet64_0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SLLI, quiet64_0, temp64_0, 0, 29);
        const int quiet64_1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SLLI, quiet64_1, temp64_1, 0, 29);
        const int oldbits0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, oldbits0, result0, 0, 0);
        const int oldbits1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, oldbits1, result1, 0, 0);
        const int flipbit0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_AND, flipbit0, quiet64_0, nan0, 0);
        const int flipbit1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_AND, flipbit1, quiet64_1, nan1, 0);
        const int newbits0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_XOR, newbits0, oldbits0, flipbit0, 0);
        const int newbits1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_XOR, newbits1, oldbits1, flipbit1, 0);
        const int newval0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_BITCAST, newval0, newbits0, 0, 0);
        const int newval1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_BITCAST, newval1, newbits1, 0, 0);
        const int newval = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_VBUILD2, newval, newval0, newval1, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, newval, 0, alias);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_nan);
        new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, new_reg, 0, 0, alias);
    }

    return new_reg;
}

/*-----------------------------------------------------------------------*/

/**
 * vfcast_64to32:  Convert an RTL register from V2_FLOAT64 to V2_FLOAT32,
 * optionally checking for SNaNs and preserving their non-quiet state.
 *
 * [Parameters]
 *     unit: RTLUnit to add operate on.
 *     reg: RTL register to convert.
 *     check_snan: True to check for SNaN inputs.
 * [Return value]
 *     RTL register holding the converted value.
 */
static int vfcast_64to32(RTLUnit *unit, int reg, bool check_snan)
{
    int nan_check = 0;
    if (check_snan) {
        nan_check = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_VFCMP, nan_check, reg, reg, RTLFCMP_UN);
    }

    int new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
    rtl_add_insn(unit, RTLOP_VFCVT, new_reg, reg, 0, 0);

    if (check_snan) {
        int alias = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_reg, 0, alias);
        const int label_not_nan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, nan_check, 0, label_not_nan);

        // FIXME: This could be done more efficiently if we had integer
        // vector types and an AND-complement instruction.
        const int ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, ps0, reg, 0, 0);
        const int ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, ps1, reg, 0, 1);
        const int nan0_64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BFEXT, nan0_64, nan_check, 0, 0 | 32<<8);
        const int nan1_64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BFEXT, nan1_64, nan_check, 0, 32 | 32<<8);
        const int nan0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, nan0, nan0_64, 0, 0);
        const int nan1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, nan1, nan1_64, 0, 0);
        const int bits0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, bits0, ps0, 0, 0);
        const int bits1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, bits1, ps1, 0, 0);
        const int notbits0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_NOT, notbits0, bits0, 0, 0);
        const int notbits1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_NOT, notbits1, bits1, 0, 0);
        const int quietbit = rtl_imm64(unit, UINT64_C(1)<<51);
        const int quiet64_0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_AND, quiet64_0, notbits0, quietbit, 0);
        const int quiet64_1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_AND, quiet64_1, notbits1, quietbit, 0);
        const int result0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_VEXTRACT, result0, new_reg, 0, 0);
        const int result1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_VEXTRACT, result1, new_reg, 0, 1);
        const int temp64_0 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SRLI, temp64_0, quiet64_0, 0, 29);
        const int temp64_1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SRLI, temp64_1, quiet64_1, 0, 29);
        const int quiet32_0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, quiet32_0, temp64_0, 0, 0);
        const int quiet32_1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, quiet32_1, temp64_1, 0, 0);
        const int oldbits0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, oldbits0, result0, 0, 0);
        const int oldbits1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, oldbits1, result1, 0, 0);
        const int flipbit0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_AND, flipbit0, quiet32_0, nan0, 0);
        const int flipbit1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_AND, flipbit1, quiet32_1, nan1, 0);
        const int newbits0 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XOR, newbits0, oldbits0, flipbit0, 0);
        const int newbits1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XOR, newbits1, oldbits1, flipbit1, 0);
        const int newval0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_BITCAST, newval0, newbits0, 0, 0);
        const int newval1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_BITCAST, newval1, newbits1, 0, 0);
        const int newval = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_VBUILD2, newval, newval0, newval1, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, newval, 0, alias);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_nan);
        new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, new_reg, 0, 0, alias);
    }

    return new_reg;
}

/*-----------------------------------------------------------------------*/

/**
 * crm_to_mask:  Return a 32-bit mask corresponding to an 8-bit CRM field
 * for mtcrf or mtfsf.
 */
static inline CONST_FUNCTION uint32_t crm_to_mask(uint8_t crm)
{
    uint32_t mask = 0;
    for (int i = 0; i < 8; i++) {
        if (crm & (1 << i)) {
            mask |= 0xF << (i*4);
        }
    }
    return mask;
}

/*************************************************************************/
/********************** Translation tility routines **********************/
/*************************************************************************/

/**
 * convert_fpr:  Convert a floating-point value from one type to another,
 * and return an RTL register containing the converted value.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: FPR index.
 *     reg: RTL register containing FPR's value.
 *     old_type: Type of "reg".
 *     new_type: Type to convert value to.  Must be different from old_type.
 *     snan_safe: True if the value is known not to be a signaling NaN.
 * [Return value]
 *     Index of RTL register containing converted value.
 */
static int convert_fpr(GuestPPCContext *ctx, int index, int reg,
                       RTLDataType old_type, RTLDataType new_type,
                       bool snan_safe)
{
    if (UNLIKELY(!reg)) {
        return 0;  // Don't ASSERT() over an error that already occurred.
    }

    ASSERT(reg == ctx->live.fpr[index]);
    ASSERT(old_type != new_type);
    ASSERT(rtl_type_is_float(old_type)
           || (rtl_type_is_vector(old_type) && rtl_vector_length(old_type) == 2
               && rtl_type_is_float(rtl_vector_element_type(old_type))));
    ASSERT(rtl_type_is_float(new_type)
           || (rtl_type_is_vector(new_type) && rtl_vector_length(new_type) == 2
               && rtl_type_is_float(rtl_vector_element_type(new_type))));

    const bool need_snan_check =
        !snan_safe
        && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_ASSUME_NO_SNAN);

    RTLUnit * const unit = ctx->unit;
    int new_reg;

    /* If converting between 32-bit and 64-bit formats, save and restore
     * floating-point state to avoid leaving stale exceptions, unless
     * NO_FPSCR_STATE is disabled (in which case we don't care about host
     * exceptions in the first place).  But if converting from FLOAT32 to
     * FLOAT64 (scalar or vector) and not checking for SNaNs, we don't
     * need to bother because no exceptions can be raised in that case. */
    int fpstate = 0;
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const RTLDataType old_stype = (rtl_type_is_vector(old_type)
                                       ? rtl_vector_element_type(old_type)
                                       : old_type);
        const RTLDataType new_stype = (rtl_type_is_vector(new_type)
                                       ? rtl_vector_element_type(new_type)
                                       : new_type);
        if (old_stype != new_stype
         && !(new_stype == RTLTYPE_FLOAT64 && !need_snan_check)) {
            fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
        }
    }

    if (old_type == RTLTYPE_V2_FLOAT64) {
        if (new_type == RTLTYPE_V2_FLOAT32) {
            new_reg = vfcast_64to32(unit, reg, need_snan_check);
        } else {
            const int f64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_VEXTRACT, f64, reg, 0, 0);
            if (new_type == RTLTYPE_FLOAT64) {
                new_reg = f64;
            } else {
                ASSERT(new_type == RTLTYPE_FLOAT32);
                new_reg = fcast_64to32(unit, f64, need_snan_check);
            }
        }

    } else if (old_type == RTLTYPE_V2_FLOAT32) {
        if (new_type == RTLTYPE_FLOAT32) {
            new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_VEXTRACT, new_reg, reg, 0, 0);
        } else {
            const int f64x2 = vfcast_32to64(unit, reg, need_snan_check);
            if (new_type == RTLTYPE_V2_FLOAT64) {
                new_reg = f64x2;
            } else {
                ASSERT(new_type == RTLTYPE_FLOAT64);
                ASSERT(unit->aliases[ctx->alias.fpr[index]].type
                       == RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, f64x2, 0, ctx->alias.fpr[index]);
                ctx->live.fpr[index] = f64x2;
                new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
                rtl_add_insn(unit, RTLOP_VEXTRACT, new_reg, f64x2, 0, 0);
            }
        }

    } else if (old_type == RTLTYPE_FLOAT64) {
        if (new_type == RTLTYPE_FLOAT32) {
            new_reg = fcast_64to32(unit, reg, need_snan_check);
        } else {
            ASSERT(unit->aliases[ctx->alias.fpr[index]].type
                   == RTLTYPE_V2_FLOAT64);
            const int old_f64x2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         old_f64x2, 0, 0, ctx->alias.fpr[index]);
            const int f64x2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_VINSERT, f64x2, old_f64x2, reg, 0);
            if (new_type == RTLTYPE_V2_FLOAT64) {
                new_reg = f64x2;
            } else {
                ASSERT(new_type == RTLTYPE_V2_FLOAT32);
                new_reg = vfcast_64to32(unit, f64x2, need_snan_check);
            }
        }

    } else {
        ASSERT(old_type == RTLTYPE_FLOAT32);
        if (new_type == RTLTYPE_V2_FLOAT32) {
            new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBROADCAST, new_reg, reg, 0, 0);
        } else {
            const int f64 = fcast_32to64(unit, reg, need_snan_check);
            if (new_type == RTLTYPE_V2_FLOAT64) {
                new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VBROADCAST, new_reg, f64, 0, 0);
            } else {
                ASSERT(new_type == RTLTYPE_FLOAT64);
                new_reg = f64;
            }
        }
    }

    if (fpstate) {
        rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
    }

    return new_reg;
}

/*-----------------------------------------------------------------------*/

/**
 * get_gpr, get_fpr, get_cr, get_crb, get_lr, get_ctr, get_xer, get_xer_so,
 * get_fpscr, get_fr_fi_fprf:  Return an RTL register containing the value
 * of the given PowerPC register or register field.  This will either be
 * the register last used in a corresponding set or get operation, or a
 * newly allocated register (in which case an appropriate GET_ALIAS
 * instruction will also be added).
 *
 * For get_crb(), get_xer_so(), and get_fr_fi_fprf(), if the USE_SPLIT_FIELDS
 * optimization is not enabled, the value will be extracted from CR/XER/FPSCR
 * respectively.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (get_gpr(), get_fpr()) or CR bit index
 *         (get_crb()).
 * [Return value]
 *     RTL register index.
 */
static inline int get_gpr(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.gpr[index]) {
        return ctx->live.gpr[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.gpr[index]);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.gpr[index]);
        ctx->live.gpr[index] = reg;
        return reg;
    }
}

static inline int get_fpr(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.fpr[index]) {
        return ctx->live.fpr[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        const RTLDataType base_type = (ctx->fpr_is_ps & (1 << index)
                                       ? RTLTYPE_V2_FLOAT64 : RTLTYPE_FLOAT64);
        const int reg = rtl_alloc_register(unit, base_type);
        ASSERT(ctx->alias.fpr[index]);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fpr[index]);
        ctx->live.fpr[index] = reg;
        return reg;
    }
}

static inline int get_cr(GuestPPCContext * const ctx)
{
    if (ctx->live.cr) {
        return ctx->live.cr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.cr);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.cr);
        ctx->live.cr = reg;
        return reg;
    }
}

static inline int get_crb(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.crb[index]) {
        return ctx->live.crb[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        int reg;
        if (ctx->crb_changed_bitrev & (0x80000000 >> index)) {
            ASSERT(ctx->alias.crb[index]);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.crb[index]);
        } else {
            const int cr = get_cr(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT,
                         reg, cr, 0, (31-index) | (1<<8));
        }
        if (ctx->use_split_fields) {
            ctx->live.crb[index] = reg;
        }
        return reg;
    }
}

static inline int get_lr(GuestPPCContext * const ctx)
{
    if (ctx->live.lr) {
        return ctx->live.lr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.lr);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.lr);
        ctx->live.lr = reg;
        return reg;
    }
}

static inline int get_ctr(GuestPPCContext * const ctx)
{
    if (ctx->live.ctr) {
        return ctx->live.ctr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.ctr);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.ctr);
        ctx->live.ctr = reg;
        return reg;
    }
}

static inline int get_xer(GuestPPCContext * const ctx)
{
    if (ctx->live.xer) {
        return ctx->live.xer;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.xer);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.xer);
        ctx->live.xer = reg;
        return reg;
    }
}

static inline int get_fpscr(GuestPPCContext * const ctx)
{
    if (ctx->live.fpscr) {
        return ctx->live.fpscr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.fpscr);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fpscr);
        ctx->live.fpscr = reg;
        return reg;
    }
}

static inline int get_fr_fi_fprf(GuestPPCContext * const ctx)
{
    if (ctx->live.fr_fi_fprf) {
        return ctx->live.fr_fi_fprf;
    } else {
        RTLUnit * const unit = ctx->unit;
        int reg;
        if (ctx->use_split_fields) {
            ASSERT(ctx->alias.fr_fi_fprf);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.fr_fi_fprf);
            ctx->live.fr_fi_fprf = reg;
        } else {
            int fpscr;
            if (ctx->live.fpscr) {
                fpscr = ctx->live.fpscr;
            } else {
                /* If FPSCR was not already live, it's generally not safe
                 * to make it live here (because so many code paths set
                 * FPSCR/FPRF conditionally). */
                fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_GET_ALIAS,
                             fpscr, 0, 0, ctx->alias.fpscr);
            }
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT,
                         reg, fpscr, 0, FPSCR_FPRF_SHIFT | 7<<8);
        }
        return reg;
    }
}

static inline int get_xer_so(GuestPPCContext * const ctx)
{
    if (ctx->live.xer_so) {
        return ctx->live.xer_so;
    } else {
        RTLUnit * const unit = ctx->unit;
        int reg;
        if (ctx->use_split_fields) {
            ASSERT(ctx->alias.xer_so);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.xer_so);
        } else {
            const int xer = get_xer(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT, reg, xer, 0, XER_SO_SHIFT | 1<<8);
        }
        ctx->live.xer_so = reg;
        return reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_fpr_as_type:  Return the value of the given floating-point register
 * converted to the given type.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: CR bit index.
 *     type: Type of value to return.
 * [Return value]
 *     RTL register index.
 */
static inline int get_fpr_as_type(GuestPPCContext * const ctx, int index,
                                  RTLDataType type)
{
    RTLUnit * const unit = ctx->unit;

    int reg = get_fpr(ctx, index);
    const RTLDataType current_type= unit->regs[reg].type;
    if (type != current_type) {
        uint32_t safe_set = ctx->fpr_is_safe;
        if (rtl_type_is_vector(type)) {
            safe_set &= ctx->ps1_is_safe;
        }
        const bool snan_safe = (safe_set & (1 << index)) != 0;
        reg = convert_fpr(ctx, index, reg, current_type, type, snan_safe);
    }
    return reg;
}

/*-----------------------------------------------------------------------*/

/**
 * get_ps1:  Return the value in the second paired-single slot of a
 * floating-point register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: CR bit index.
 *     type: Type of value to return (either RTLTYPE_FLOAT32 or
 *         RTLTYPE_FLOAT64).
 * [Return value]
 *     RTL register index.
 */
static inline int get_ps1(GuestPPCContext * const ctx, int index,
                          RTLDataType type)
{
    RTLUnit * const unit = ctx->unit;

    int reg;
    if (ctx->live.fpr[index]) {
        const RTLDataType live_type = unit->regs[ctx->live.fpr[index]].type;
        if (rtl_type_is_vector(live_type)) {
            reg = rtl_alloc_register(unit, rtl_vector_element_type(live_type));
            rtl_add_insn(unit, RTLOP_VEXTRACT, reg, ctx->live.fpr[index], 0, 1);
        } else {
            /* We never write a scalar FLOAT64 to a paired-single register. */
            ASSERT(live_type == RTLTYPE_FLOAT32);
            reg = ctx->live.fpr[index];
        }
    } else {
        const int pair = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, pair, 0, 0, ctx->alias.fpr[index]);
        ctx->live.fpr[index] = pair;
        reg = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, reg, pair, 0, 1);
    }

    const RTLDataType current_type = unit->regs[reg].type;
    if (current_type != type) {
        const int need_snan_check =
            !(ctx->ps1_is_safe & (1 << index))
            && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_ASSUME_NO_SNAN);

        /* Avoid raising exceptions, as in convert_fpr(). */
        int fpstate = 0;
        if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)
         && !(type == RTLTYPE_FLOAT64 && !need_snan_check)) {
            fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
        }

        if (type == RTLTYPE_FLOAT64) {
            reg = fcast_32to64(unit, reg, need_snan_check);
        } else {
            reg = fcast_64to32(unit, reg, need_snan_check);
        }

        if (fpstate) {
            rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
        }
    }

    return reg;
}

/*-----------------------------------------------------------------------*/

/**
 * test_crb:  Return an RTL register containing a value which is nonzero if
 * the given CR bit is set and zero if it is clear.  This function does not
 * initialize the CR bit alias if it has not already been loaded.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: CR bit index.
 * [Return value]
 *     RTL register index.
 */
static inline int test_crb(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.crb[index]) {
        return ctx->live.crb[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        int reg;
        if (ctx->alias.crb[index]) {
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.crb[index]);
            ctx->live.crb[index] = reg;
        } else {
            const int cr = get_cr(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, reg, cr, 0, 0x80000000 >> index);
        }
        return reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_gpr, set_fpr, set_cr, set_crb, set_lr, set_ctr, set_xer, set_fpscr,
 * set_fr_fi_fprf:  Store the given RTL register to the given PowerPC
 * register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (set_gpr(), set_fpr()) or CR bit index
 *         (set_crb()).
 *     reg: Register to store.
 *     so: Register holding the value of XER[SO], 0 if not available, or
 *         -1 if XER[SO] has not changed (set_xer() only).
 */
static inline void set_gpr(GuestPPCContext * const ctx, int index, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.gpr[index] >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.gpr[index], false, false);
    }
    ctx->last_set.gpr[index] = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.gpr[index]);
    ctx->live.gpr[index] = reg;
}

static inline void set_fpr(GuestPPCContext * const ctx, int index, int reg)
{
    RTLUnit * const unit = ctx->unit;

    /* If overwriting a different type with FLOAT64, we need to store the
     * second half of the old value to the state block or alias. */
    if (ctx->live.fpr[index]
     && unit->regs[reg].type == RTLTYPE_FLOAT64
     && unit->regs[ctx->live.fpr[index]].type != RTLTYPE_FLOAT64) {
        const int old_reg = ctx->live.fpr[index];
        if (unit->regs[old_reg].type == RTLTYPE_V2_FLOAT64) {
            /* This typically happens if a register is used in paired-single
             * mode at a different point in the unit but is being used in
             * double-precision mode here.  Just insert the new value into
             * the old vector and use the updated vector as the current
             * FPR value. */
            const int new = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_VINSERT, new, old_reg, reg, 0);
            reg = new;
        } else {
            int ps1;
            if (unit->regs[old_reg].type == RTLTYPE_FLOAT32) {
                ps1 = old_reg;
            } else {
                ASSERT(unit->regs[old_reg].type == RTLTYPE_V2_FLOAT32);
                ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_VEXTRACT, ps1, old_reg, 0, 1);
            }
            int fpstate = 0;
            if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
                fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
                rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
            }
            const bool snan_safe = (ctx->ps1_is_safe & (1 << index)) != 0;
            const int ps1_64 = fcast_32to64(unit, ps1, !snan_safe);
            if (ctx->fpr_is_ps & (1 << index)) {
                const int new = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VBUILD2, new, reg, ps1_64, 1);
                reg = new;
            } else {
                rtl_add_insn(
                    unit, RTLOP_STORE, 0, ctx->psb_reg, ps1_64,
                    ctx->handle->setup.state_offset_fpr + index*16 + 8);
            }
            if (fpstate) {
                rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
            }
        }
    }

    ctx->live.fpr[index] = reg;
    ctx->fpr_dirty |= 1 << index;
}

static inline void set_cr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.cr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.cr, false, false);
    }
    ctx->last_set.cr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.cr);
    ctx->live.cr = reg;
}

static inline void set_crb(GuestPPCContext * const ctx, int index, int reg)
{
    RTLUnit * const unit = ctx->unit;

    if (ctx->use_split_fields) {
        ASSERT(ctx->crb_changed_bitrev & (0x80000000 >> index));
        if (ctx->last_set.crb[index] >= 0) {
            rtl_opt_kill_insn(unit, ctx->last_set.crb[index], false, false);
        }
        ctx->last_set.crb[index] = unit->num_insns;
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.crb[index]);
        ctx->live.crb[index] = reg;
        ctx->crb_dirty |= 1 << index;
    } else {
        const int old_cr = get_cr(ctx);
        const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS,
                     new_cr, old_cr, reg, (31-index) | 1<<8);
        set_cr(ctx, new_cr);
    }
}

static inline void set_lr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.lr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.lr, false, false);
    }
    ctx->last_set.lr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.lr);
    ctx->live.lr = reg;
}

static inline void set_ctr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.ctr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.ctr, false, false);
    }
    ctx->last_set.ctr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.ctr);
    ctx->live.ctr = reg;
}

static inline void set_xer(GuestPPCContext * const ctx, int reg, int so)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.xer >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.xer, false, false);
    }
    ctx->last_set.xer = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.xer);
    ctx->live.xer = reg;
    if (so >= 0) {
        if (ctx->alias.xer_so) {
            if (ctx->last_set.xer_so >= 0) {
                rtl_opt_kill_insn(unit, ctx->last_set.xer_so, false, false);
            }
            if (!so) {
                so = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT,
                             so, reg, 0, XER_SO_SHIFT | 1<<8);
            }
            ctx->last_set.xer_so = unit->num_insns;
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, so, 0, ctx->alias.xer_so);
        }
        ctx->live.xer_so = so;
    }
}

static inline void set_fpscr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.fpscr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.fpscr, false, false);
    }
    ctx->last_set.fpscr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.fpscr);
    ctx->live.fpscr = reg;
}

static inline void set_fr_fi_fprf(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->use_split_fields) {
        if (ctx->last_set.fr_fi_fprf >= 0) {
            rtl_opt_kill_insn(unit, ctx->last_set.fr_fi_fprf, false, false);
        }
        ctx->last_set.fr_fi_fprf = unit->num_insns;
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.fr_fi_fprf);
        ctx->live.fr_fi_fprf = reg;
    } else {
        const int old_fpscr = get_fpscr(ctx);
        const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_fpscr, old_fpscr, reg,
                     FPSCR_FPRF_SHIFT | 7<<8);
        set_fpscr(ctx, new_fpscr);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_crf:  Set the given CR 4-bit field from the given four registers,
 * each assumed to hold a 1-bit value.  This generates more efficient code
 * than a sequence of calls to set_crb() if the USE_SPLIT_FIELDS
 * optimization is not enabled.
 */
static inline void set_crf(GuestPPCContext * const ctx, int index,
                           int bit0, int bit1, int bit2, int bit3)
{
    RTLUnit * const unit = ctx->unit;

    if (ctx->use_split_fields) {
        set_crb(ctx, index*4+0, bit0);
        set_crb(ctx, index*4+1, bit1);
        set_crb(ctx, index*4+2, bit2);
        set_crb(ctx, index*4+3, bit3);
    } else {
        const int old_cr = get_cr(ctx);
        const int bit0_sll3 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, bit0_sll3, bit0, 0, 3);
        const int bit1_sll2 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, bit1_sll2, bit1, 0, 2);
        const int bit2_sll1 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, bit2_sll1, bit2, 0, 1);
        const int bit01 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, bit01, bit0_sll3, bit1_sll2, 0);
        const int bit23 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, bit23, bit2_sll1, bit3, 0);
        const int bits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, bits, bit01, bit23, 0);
        const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS,
                     new_cr, old_cr, bits, ((7-index)*4) | 4<<8);
        set_cr(ctx, new_cr);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_fr_fi_fprf_and_flush:  Store the given value to fr_fi_fprf, and
 * immediately flush it out.
 */
static void set_fr_fi_fprf_and_flush(GuestPPCContext *ctx, int reg)
{
    set_fr_fi_fprf(ctx, reg);
    if (ctx->use_split_fields) {
        ctx->live.fr_fi_fprf = 0;
        ctx->last_set.fr_fi_fprf = -1;
    } else {
        ctx->live.fpscr = 0;
        ctx->last_set.fpscr = -1;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_fpr_scalar_type:  Return the scalar type corresponding to the
 * current mode of the given floating-point register.
 */
static inline RTLDataType get_fpr_scalar_type(GuestPPCContext *ctx, int index)
{
    if (ctx->live.fpr[index]) {
        RTLDataType type = ctx->unit->regs[ctx->live.fpr[index]].type;
        if (rtl_type_is_vector(type)) {
            type = rtl_vector_element_type(type);
        }
        return type;
    } else {
        return RTLTYPE_FLOAT64;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_fpscr_fex_vx:  Return RTL registers containing the FEX and VX bits
 * for the given value of FPSCR.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     fpscr: RTL register containing the FPSCR value.
 *     fex_ret: Pointer to variable to receive an RTL register containing
 *        the value of the FEX bit.
 *     vx_ret: Pointer to variable to receive an RTL register containing
 *        the value of the VX bit.
 */
static void get_fpscr_fex_vx(GuestPPCContext *ctx, int fpscr,
                             int *fex_ret, int *vx_ret)
{
    ASSERT(fex_ret);
    ASSERT(vx_ret);

    RTLUnit * const unit = ctx->unit;

    const int vx_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, vx_test, fpscr, 0, FPSCR_ALL_VXFOO);
    const int vx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTUI, vx, vx_test, 0, 0);
    *vx_ret = vx;

    const int x_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, x_bits, fpscr, 0, FPSCR_XX_SHIFT | 4<<8);
    const int vx_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, vx_shifted, vx, 0, 4);
    const int e_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, e_bits, fpscr, 0, FPSCR_XE_SHIFT);
    const int x_bits_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, x_bits_vx, x_bits, vx_shifted, 0);
    const int both_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, both_bits, x_bits_vx, e_bits, 0);
    const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, test, both_bits, 0, 31);
    const int fex = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTUI, fex, test, 0, 0);
    *fex_ret = fex;
}

/*-----------------------------------------------------------------------*/

/**
 * get_ea_base:  Allocate and return a new RTL register of ADDRESS type
 * containing the host address for the base EA (without offset) in the
 * given D-form instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 * [Return value]
 *     RTL register containing the host address corresponding to (rA|0).
 */
static inline int get_ea_base(GuestPPCContext *ctx, uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (insn_rA(insn)) {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, address, rA, 0, 0);
        const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     host_address, ctx->membase_reg, address, 0);
        return host_address;
    } else {
        return ctx->membase_reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_ea_indexed:  Allocate and return a new RTL register of ADDRESS type
 * containing the host address for the EA in the given X-form instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     guest_ea_ret: Pointer to variable to receive the RTL register
 *         containing ((rA|0) + rB), or NULL if not needed.
 * [Return value]
 *     RTL register containing the host address corresponding to ((rA|0) + rB).
 */
static inline int get_ea_indexed(GuestPPCContext *ctx, uint32_t insn,
                                 int *guest_ea_ret)
{
    RTLUnit * const unit = ctx->unit;

    int addr32;
    if (insn_rA(insn)) {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        addr32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, addr32, rA, rB, 0);
    } else {
        addr32 = get_gpr(ctx, insn_rB(insn));
    }
    if (guest_ea_ret) {
        *guest_ea_ret = addr32;
    }
    const int address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ZCAST, address, addr32, 0, 0);
    const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ADD, host_address, ctx->membase_reg, address, 0);
    return host_address;
}

/*-----------------------------------------------------------------------*/

/**
 * gen_load_store_address:  Generate the address for a load or store operation.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_indexed: True if the access is an indexed access, false if not.
 *     update: True if rA will be updated, false if not.
 *     disp_ret: Pointer to variable to receive the access offset for the
 *         load/store instruction.  Always receives zero if is_indexed or
 *         update is true.
 *     ea_ret: Pointer to variable to receive the effective address to
 *         write back to rA for an update operation.  Not modified (and may
 *         be NULL) if update is false.
 * [Return value]
 *     RTL register containing the host address for the load/store instruction.
 */
static int gen_load_store_address(GuestPPCContext *ctx, uint32_t insn,
                                  bool is_indexed, bool update,
                                  int *disp_ret, int *ea_ret)
{
    ASSERT(ctx);
    ASSERT(disp_ret);
    ASSERT(!update || ea_ret);

    RTLUnit * const unit = ctx->unit;
    int host_address;
    const int disp =
        insn_OPCD(insn) >= OPCD_PSQ_L ? insn_d12(insn) : insn_d(insn);

    if (update) {
        ASSERT(insn_rA(insn) != 0);
        if (is_indexed) {
            host_address = get_ea_indexed(ctx, insn, ea_ret);
        } else {
            int ea;
            const int rA = get_gpr(ctx, insn_rA(insn));
            if (disp != 0) {
                ea = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ADDI, ea, rA, 0, disp);
            } else {
                ea = rA;
            }
            const int ea_zcast = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ZCAST, ea_zcast, ea, 0, 0);
            host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ADD,
                         host_address, ctx->membase_reg, ea_zcast, 0);
            *ea_ret = ea;
        }
        *disp_ret = 0;
    } else {
        if (is_indexed) {
            host_address = get_ea_indexed(ctx, insn, NULL);
            *disp_ret = 0;
        } else {
            if (insn_rA(insn) != 0 || disp >= 0) {
                host_address = get_ea_base(ctx, insn);
                *disp_ret = disp;
            } else {
                const int offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
                rtl_add_insn(unit, RTLOP_LOAD_IMM,
                             offset, 0, 0, (uint32_t)disp);
                host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
                rtl_add_insn(unit, RTLOP_ADD,
                             host_address, ctx->membase_reg, offset, 0);
                *disp_ret = 0;
            }
        }
    }

    return host_address;
}

/*-----------------------------------------------------------------------*/

/**
 * flush_fpr:  Finalize any pending store for the given floating-point
 * register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: FPR index.
 *     clear_live: True to clear the live state of the register, false to
 *         leave it live.
 */
static void flush_fpr(GuestPPCContext *ctx, int index, bool clear_live)
{
    if (ctx->fpr_dirty & (1 << index)) {
        int reg = ctx->live.fpr[index];
        const RTLDataType base_type = (ctx->fpr_is_ps & (1 << index)
                                       ? RTLTYPE_V2_FLOAT64 : RTLTYPE_FLOAT64);
        const RTLDataType current_type = ctx->unit->regs[reg].type;
        if (current_type != base_type) {
            uint32_t safe_set = ctx->fpr_is_safe;
            if (rtl_type_is_vector(current_type)) {
                safe_set &= ctx->ps1_is_safe;
            }
            const bool snan_safe = (safe_set & (1 << index)) != 0;
            reg = convert_fpr(ctx, index, reg, current_type, base_type,
                              snan_safe);
            if (current_type==RTLTYPE_FLOAT32 && base_type==RTLTYPE_FLOAT64) {
                rtl_add_insn(
                    ctx->unit, RTLOP_STORE, 0, ctx->psb_reg, reg,
                    ctx->handle->setup.state_offset_fpr + 16*index + 8);
            }
        }
        rtl_add_insn(ctx->unit, RTLOP_SET_ALIAS,
                     0, reg, 0, ctx->alias.fpr[index]);
        ctx->fpr_dirty &= ~(1 << index);
    }
    if (clear_live) {
        ctx->live.fpr[index] = 0;
        ctx->fpr_is_safe &= ~(1 << index);
        ctx->ps1_is_safe &= ~(1 << index);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_fpr_and_flush:  Store the given RTL register to the given PowerPC
 * floating-point register, and immediately flush its value out.  The value
 * is assumed to be safe for conversion.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: FPR index.
 *     reg: RTL register to store.
 *     snan_safe: True if the value to store is known not to contain SNaNs.
 */
static void set_fpr_and_flush(GuestPPCContext *ctx, int index, int reg,
                              bool snan_safe)
{
    set_fpr(ctx, index, reg);
    if (snan_safe) {
        ctx->fpr_is_safe |= 1 << index;
        if (ctx->unit->regs[reg].type != RTLTYPE_FLOAT64) {
            ctx->ps1_is_safe |= 1 << index;
        }
    }
    flush_fpr(ctx, index, true);
}

/*-----------------------------------------------------------------------*/

/**
 * flush_live_regs:  Finalize all pending stores of guest registers.
 * This does not handle merging split bitfields back to their primary
 * registers.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     clear: True to clear all live registers after flushing them; false
 *         to leave them live.
 */
static void flush_live_regs(GuestPPCContext *ctx, bool clear)
{
    uint32_t fpr_dirty = ctx->fpr_dirty;
    while (fpr_dirty) {
        const int index = ctz32(fpr_dirty);
        fpr_dirty ^= 1 << index;
        flush_fpr(ctx, index, false);
    }

    if (clear) {
        memset(&ctx->last_set, -1, sizeof(ctx->last_set));
        memset(&ctx->live, 0, sizeof(ctx->live));
        ctx->fpr_is_safe = 0;
        ctx->ps1_is_safe = 0;
        ctx->crb_dirty = 0;
    } else {
        /* Only clear last_set for registers which are mapped to the PSB. */
        memset(ctx->last_set.gpr, -1, sizeof(ctx->last_set.gpr));
        ctx->last_set.cr = -1;
        ctx->last_set.lr = -1;
        ctx->last_set.ctr = -1;
        ctx->last_set.xer = -1;
        ctx->last_set.fpscr = -1;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * merge_cr:  Merge all CR bit aliases (and untouched CR bits from the
 * processor state block) into a 32-bit CR word.  Helper for
 * guest_ppc_flush_cr().
 *
 * [Parameters]
 *     ctx: Translation context.
 *     make_live: True to always leave CR live in its alias, false to not
 *         call get_cr() if CR is not live.
 * [Return value]
 *     RTL register containing merged value of CR.
 */
static int merge_cr(GuestPPCContext *ctx, bool make_live)
{
    ASSERT(ctx->use_split_fields);

    RTLUnit * const unit = ctx->unit;

    uint32_t crb_changed = ctx->crb_changed_bitrev;
    ASSERT(crb_changed != 0);  // We won't be called if nothing to merge.

    int cr;
    if (crb_changed == ~UINT32_C(0)) {
        cr = 0;
    } else {
        int old_cr;
        if (make_live) {
            old_cr = get_cr(ctx);
        } else if (ctx->live.cr) {
            old_cr = ctx->live.cr;
        } else {
            old_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, old_cr, 0, 0, ctx->alias.cr);
        }
        cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, cr, old_cr, 0, ~crb_changed);
    }

    while (crb_changed) {
        const int bit = clz32(crb_changed);
        crb_changed ^= 0x80000000 >> bit;
        const int crbN = get_crb(ctx, bit);
        int shifted_crbN;
        if (bit == 31) {
            shifted_crbN = crbN;
        } else {
            shifted_crbN = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, shifted_crbN, crbN, 0, 31 - bit);
        }
        if (cr) {
            const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, new_cr, cr, shifted_crbN, 0);
            cr = new_cr;
        } else {
            cr = shifted_crbN;
        }
    }

    return cr;
}

/*-----------------------------------------------------------------------*/

/**
 * merge_fpscr:  Merge the FR/FI/FPRF alias into FPSCR and return an RTL
 * register containing the merged value.  Helper for guest_ppc_flush_fpscr()
 * and mffs.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     make_live: True to always leave FPSCR live in its alias, false to not
 *         call get_fpscr() if FPSCR is not live.
 * [Return value]
 *     RTL register containing merged value of FPSCR.
 */
static int merge_fpscr(GuestPPCContext *ctx, bool make_live)
{
    ASSERT(ctx->use_split_fields);

    RTLUnit * const unit = ctx->unit;

    int fpscr;
    if (make_live) {
        fpscr = get_fpscr(ctx);
    } else if (ctx->live.fpscr) {
        fpscr = ctx->live.fpscr;
    } else {
        fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
    }
    const int fr_fi_fprf = get_fr_fi_fprf(ctx);

    const int masked_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, masked_fpscr, fpscr, 0,
                 ~(FPSCR_FEX | FPSCR_VX | FPSCR_FR | FPSCR_FI | FPSCR_FPRF
                   | FPSCR_RESV20));
    const int shifted_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI,
                 shifted_fprf, fr_fi_fprf, 0, FPSCR_FPRF_SHIFT);

    const int merged_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, merged_fpscr, masked_fpscr, shifted_fprf, 0);

    return merged_fpscr;
}

/*-----------------------------------------------------------------------*/

/**
 * post_insn_callback:  Add RTL to call the post-instruction callback if
 * one has been set.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Instruction address to pass to the callback.
 */
static void post_insn_callback(GuestPPCContext *ctx, uint32_t address)
{
    if (ctx->handle->post_insn_callback) {
        flush_live_regs(ctx, false);
        RTLUnit * const unit = ctx->unit;
        const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, func, 0, 0,
                     (uintptr_t)ctx->handle->post_insn_callback);
        rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT,
                     0, func, ctx->psb_reg, rtl_imm32(unit, address));
    }
}

/*-----------------------------------------------------------------------*/

/**
 * check_snan:  Check whether the given floating-point RTL register is a
 * signaling NaN, and branch to the given label if so.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: Register to check (must be of scalar floating-point type).
 *     label: Label to branch to if the value is a SNaN.
 */
static void check_snan(GuestPPCContext *ctx, int reg, int label)
{
    RTLUnit * const unit = ctx->unit;

    RTLDataType bits_type;
    int type_size, snan_start, snan_count;
    if (unit->regs[reg].type == RTLTYPE_FLOAT32) {
        bits_type = RTLTYPE_INT32;
        type_size = 32;
        snan_start = 22;
        snan_count = 9;
    } else {
        bits_type = RTLTYPE_INT64;
        type_size = 64;
        snan_start = 51;
        snan_count = 12;
    }
    const uint32_t snan_value = (1 << snan_count) - 2;

    const int not_snan_label = rtl_alloc_label(unit);
    const int bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BITCAST, bits, reg, 0, 0);
    const int mantissa_test = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_SLLI,
                 mantissa_test, bits, 0, type_size - snan_start);
    const int snan_test = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BFEXT,
                 snan_test, bits, 0, snan_start | snan_count<<8);
    const int is_snan = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, is_snan, snan_test, 0, snan_value);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, is_snan, 0, not_snan_label);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, mantissa_test, 0, label);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, not_snan_label);
}

/*-----------------------------------------------------------------------*/

/**
 * flush_denormal:  Return a register containing the given input value,
 * or zero if that value is a denormal.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register (must be of type FLOAT32).
 * [Return value]
 *     RTL register containing result.
 */
static int flush_denormal(GuestPPCContext *ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;

    const int bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BITCAST, bits, reg, 0, 0);
    const int exp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, exp, bits, 0, 23 | 8<<8);
    const int sign = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, sign, bits, 0, 0x80000000);
    const int zero = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_BITCAST, zero, sign, 0, 0);
    const int flushed = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_SELECT, flushed, reg, zero, exp);
    return flushed;
}

/*-----------------------------------------------------------------------*/

/**
 * fma_negate:  Negate the given floating-point value, but only if it is
 * not a NaN.  Helper for fused multiply-add implementations.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register (must be of a scalar floating-point type).
 * [Return value]
 *     RTL register containing result.
 */
static int fma_negate(GuestPPCContext *ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;

    const RTLDataType type = unit->regs[reg].type;
    const int zero = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, zero, 0, 0, 0);
    const int is_nan = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, is_nan, reg, zero, RTLFCMP_UN);
    const int pos_value = reg;
    const int neg_value = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_FNEG, neg_value, pos_value, 0, 0);
    const int result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_SELECT, result, pos_value, neg_value, is_nan);

    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * fma_select_nan:  Select the appropriate NaN to return for a fused
 * multiply-add operation.  The returned value is unchanged if no operand
 * was a NaN.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     result: RTL register containing result (must be of a scalar
 *         floating-point type).
 *     frA: RTL register containing frA (of the same type as result).
 *     frB: RTL register containing frB (of the same type as result).
 *     frC: RTL register containing frC (of the same type as result).
 * [Return value]
 *     RTL register containing result.
 */
static int fma_select_nan(GuestPPCContext *ctx, int result,
                          int frA, int frB, int frC)
{
    RTLUnit * const unit = ctx->unit;

    const RTLDataType type = unit->regs[result].type;
    const bool use_float32 = (type == RTLTYPE_FLOAT32);

    /* We use a condition and SELECT instead of branches so we don't need
     * to mess around with temporary aliases.  This creates a fairly long
     * dependency chain even for the (much more common) case of not
     * changing the result, but if NATIVE_IEEE_NAN is disabled then speed
     * probably isn't important anyway. */
    const int zero = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, zero, 0, 0, 0);
    const int frA_not_nan = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, frA_not_nan, frA, zero, RTLFCMP_NUN);
    const int frC_is_nan = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, frC_is_nan, frC, zero, RTLFCMP_UN);
    const int frB_is_nan = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, frB_is_nan, frB, zero, RTLFCMP_UN);
    const int temp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, temp, frA_not_nan, frC_is_nan, 0);
    const int use_frB = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, use_frB, temp, frB_is_nan, 0);

    const RTLDataType bits_type = use_float32 ? RTLTYPE_INT32 : RTLTYPE_INT64;
    const int frB_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BITCAST, frB_bits, frB, 0, 0);
    const int quiet_bit = rtl_alloc_register(unit, bits_type);
    if (bits_type == RTLTYPE_INT32) {
        rtl_add_insn(unit, RTLOP_LOAD_IMM, quiet_bit, 0, 0, 0x00400000);
    } else {
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     quiet_bit, 0, 0, UINT64_C(0x0008000000000000));
    }
    const int quiet_frB_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_OR, quiet_frB_bits, frB_bits, quiet_bit, 0);
    const int quiet_frB = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_BITCAST, quiet_frB, quiet_frB_bits, 0, 0);

    const int orig_result = result;
    result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_SELECT, result, quiet_frB, orig_result, use_frB);
    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * ps_dequantize:  Convert an integer value to floating-point for a
 * paired-single load instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     intval: RTL register (of type INT32) containing the loaded value.
 *     scale: RTL register (of type FLOAT32) containing the scale factor
 *         (2^-gqr_scale), or 0 if the value does not need to be scaled.
 * [Return value]
 *     RTL register (of type FLOAT32) containing the converted value.
 */
static int ps_dequantize(GuestPPCContext *ctx, int intval, int scale)
{
    RTLUnit * const unit = ctx->unit;

    const int floatval = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_FSCAST, floatval, intval, 0, 0);
    if (!scale) {
        return floatval;
    }

    const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_FMUL, result, floatval, scale, 0);
    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * ps_quantize:  Convert a floating-point value to integer for a
 * paired-single store instruction.
 *
 * This function may raise host floating-point exceptions.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     floatval: RTL register (of type FLOAT32) containing the value to store.
 *     scale: RTL register (of type FLOAT32) containing the scale factor
 *         (2^gqr_scale), or 0 if the value does not need to be scaled.
 *     min_val: RTL register containing the low bound of the result value.
 *     max_val: RTL register containing the high bound of the result value.
 * [Return value]
 *     RTL register (of type INT32) containing the converted value.
 */
static int ps_quantize(GuestPPCContext *ctx, int floatval, int scale,
                       int min_val, int max_val)
{
    RTLUnit * const unit = ctx->unit;

    /* Scale the input value and convert to integer.  Also check (in
     * parallel, for hopefully simultaneous execution) for overflow. */
    int scaled_val;
    if (scale) {
        scaled_val = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_FMUL, scaled_val, floatval, scale, 0);
    } else {
        scaled_val = floatval;
    }
    const int bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BITCAST, bits, scaled_val, 0, 0);
    const int scaled_int = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTRUNCI, scaled_int, scaled_val, 0, 0);
    const int bits_sll1 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, bits_sll1, bits, 0, 1);
    const int is_sign = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, is_sign, bits, 0, 31);
    const int overflow_val = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SELECT, overflow_val, min_val, max_val, is_sign);
    const int is_overflow = rtl_alloc_register(unit, RTLTYPE_INT32);
    /* For the overflow boundary, use a value which is both large enough
     * not to clip any valid output values and small enough so we're not
     * affected by host-defined saturation behavior. */
    rtl_add_insn(unit, RTLOP_SGTUI, is_overflow, bits_sll1, 0,
                 (0x47800000<<1)-1);  // 0x47800000 == 65536.0f
    const int intval = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SELECT,
                 intval, overflow_val, scaled_int, is_overflow);

    /* Clamp the result to the given bounds and return the clamped value. */
    const int over_max = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTS, over_max, intval, max_val, 0);
    const int temp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SELECT, temp, max_val, intval, over_max);
    const int under_min = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTS, under_min, intval, min_val, 0);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SELECT, result, min_val, temp, under_min);
    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * set_fpscr_exceptions:  Set the given exception bits in FPSCR, along with
 * the FX bit if any exception bit was not already set.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     fpscr: RTL register containing current value of FPSCR, or 0 to
 *         fetch it from the alias.
 *     exceptions: Bitmask of exception bits to set.
 */
static void set_fpscr_exceptions(GuestPPCContext *ctx, int fpscr,
                                 uint32_t exceptions)
{
    RTLUnit * const unit = ctx->unit;

    if (!fpscr) {
        fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
    }

    const int unset_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_NOT, unset_bits, fpscr, 0, 0);
    const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0, exceptions);
    const int fx_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, fx_test, unset_bits, 0, exceptions);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_fpscr, 0, ctx->alias.fpscr);
    const int label = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, fx_test, 0, label);
    const int with_fx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, with_fx, new_fpscr, 0, FPSCR_FX);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, with_fx, 0, ctx->alias.fpscr);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label);
}

/*-----------------------------------------------------------------------*/

/**
 * gen_fprf:  Generate an FPRF value for the given floating-point value.
 * Helper function for set_fp_result().
 *
 * [Parameters]
 *     unit: RTLUnit to which to add code.
 *     value: RTL register containing value for which to generate FPRF.
 *     slot: Paired-slot index (0 or 1) to use if value is a vector.
 * [Return value]
 *     RTL register containing FPRF value.
 */
static int gen_fprf(RTLUnit *unit, int value, int slot)
{
    ASSERT(unit);

    if (rtl_register_is_vector(&unit->regs[value])) {
        const int slot_value = rtl_alloc_register(
            unit, rtl_vector_element_type(unit->regs[value].type));
        rtl_add_insn(unit, RTLOP_VEXTRACT, slot_value, value, 0, slot);
        value = slot_value;
    }

    const bool is64 = (unit->regs[value].type == RTLTYPE_FLOAT64);
    const RTLDataType bits_type = is64 ? RTLTYPE_INT64 : RTLTYPE_INT32;

    const int bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BITCAST, bits, value, 0, 0);
    const int bits_nonzero = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTUI, bits_nonzero, bits, 0, 0);

    const int sign = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int exponent_zero = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int exponent_max = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int mantissa_zero = rtl_alloc_register(unit, RTLTYPE_INT32);
    if (is64) {
        rtl_add_insn(unit, RTLOP_SLTSI, sign, bits, 0, 0);
        const int exponent = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BFEXT, exponent, bits, 0, 52 | 11<<8);
        rtl_add_insn(unit, RTLOP_SEQI, exponent_zero, exponent, 0, 0);
        rtl_add_insn(unit, RTLOP_SEQI, exponent_max, exponent, 0, 0x7FF);
        const int mantissa_test = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SLLI, mantissa_test, bits, 0, 12);
        rtl_add_insn(unit, RTLOP_SEQI, mantissa_zero, mantissa_test, 0, 0);
    } else {
        rtl_add_insn(unit, RTLOP_SRLI, sign, bits, 0, 31);
        const int exponent = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFEXT, exponent, bits, 0, 23 | 8<<8);
        rtl_add_insn(unit, RTLOP_SEQI, exponent_zero, exponent, 0, 0);
        rtl_add_insn(unit, RTLOP_SEQI, exponent_max, exponent, 0, 0xFF);
        const int mantissa_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, mantissa_test, bits, 0, 9);
        rtl_add_insn(unit, RTLOP_SEQI, mantissa_zero, mantissa_test, 0, 0);
    }

    /*
     * FPRF table:
     *    C < > E ?
     *    ---------
     *    1 0 0 0 1   NaN
     *    0 1 0 0 1   -inf
     *    0 1 0 0 0   -normal
     *    1 1 0 0 0   -denorm
     *    1 0 0 1 0   -zero
     *    0 0 0 1 0   +zero
     *    1 0 1 0 0   +denorm
     *    0 0 1 0 0   +norm
     *    0 0 1 0 1   +inf
     *
     * Bit formulas:
     *    nan = exponent_max & !mantissa_zero
     *    nzn = !(E | nan)  // "nonzero number"
     *    C = (exponent_zero & bits_nonzero) | nan
     *    < = sign & nzn
     *    > = !sign & nzn
     *    E = exponent_zero & mantissa_zero
     *    ? = exponent_max
     */
    const int un = exponent_max;
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, eq, exponent_zero, mantissa_zero, 0);
    const int mantissa_nonzero = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_XORI, mantissa_nonzero, mantissa_zero, 0, 1);
    const int nan = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, nan, exponent_max, mantissa_nonzero, 0);
    const int cls_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, cls_temp, exponent_zero, bits_nonzero, 0);
    const int cls = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, cls, cls_temp, nan, 0);
    const int not_nzn = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, not_nzn, eq, nan, 0);
    const int nzn = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_XORI, nzn, not_nzn, 0, 1);
    const int not_sign = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_XORI, not_sign, sign, 0, 1);
    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, lt, sign, nzn, 0);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, gt, not_sign, nzn, 0);

    const int shifted_cls = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_cls, cls, 0, 4);
    const int shifted_lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_lt, lt, 0, 3);
    const int shifted_gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_gt, gt, 0, 2);
    const int shifted_eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_eq, eq, 0, 1);
    const int cls_lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, cls_lt, shifted_cls, shifted_lt, 0);
    const int gt_eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, gt_eq, shifted_gt, shifted_eq, 0);
    const int cls_lt_un = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, cls_lt_un, cls_lt, un, 0);
    const int fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, fprf, cls_lt_un, gt_eq, 0);

    return fprf;
}

/*-----------------------------------------------------------------------*/

/**
 * round_fma_result_to_single:  Round the result of a double-precision
 * fused multiply-add operation to single precision, taking into account
 * the possibility of rounding error caused by a tiny addend.
 *
 * See the documentation of BINREC_OPT_G_PPC_FAST_FMADDS for the rationale
 * behind this function.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     result: Operation result (of type FLOAT64).
 *     rtlop: RTL opcode for the FMA operation.
 *     frA, frB, frC: Operands (of type FLOAT64).
 * [Return value]
 *     Rounded value (of type FLOAT32).
 */
static int round_fma_result_to_single(
    GuestPPCContext *ctx, int result, RTLOpcode rtlop,
    int frA, int frB, int frC)
{
    RTLUnit * const unit = ctx->unit;
    /* Don't ASSERT() over an error that already occurred. */
    ASSERT(!result || unit->regs[result].type == RTLTYPE_FLOAT64);
    ASSERT(!frA || unit->regs[frA].type == RTLTYPE_FLOAT64);
    ASSERT(!frB || unit->regs[frB].type == RTLTYPE_FLOAT64);
    ASSERT(!frC || unit->regs[frC].type == RTLTYPE_FLOAT64);

    const int result_alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT32);
    const int label_out = rtl_alloc_label(unit);

    /* Generate the rounded result now since it's what we'll use the vast
     * majority of the time.  This also ensures that appropriate exceptions
     * from the rounding operation are set. */
    const int result_rounded = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_FCVT, result_rounded, result, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, result_rounded, 0, result_alias);

    /* If FPSCR[RN] is not round-to-nearest, we don't have to do anything. */
    const int fpscr = get_fpscr(ctx);
    const int rn = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, rn, fpscr, 0, FPSCR_RN_SHIFT | 2<<8);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, rn, 0, label_out);

    /* If the result is out of single-precision range (or is a NaN), we
     * don't have to do anything. */
    const int result_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BITCAST, result_bits, result, 0, 0);
    const int exponent64 = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, exponent64, result_bits, 0, 52 | 11<<8);
    const int exponent = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ZCAST, exponent, exponent64, 0, 0);
    const int exponent_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ADDI, exponent_temp, exponent, result_bits, -874);
    const int exponent_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTUI, exponent_test, exponent_temp, 0, 1151-874);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, exponent_test, 0, label_out);

    /* If the result is not exactly between two single-precision values,
     * we don't have to do anything.  This is a bit tricky because we
     * have to take denormals into account as well. */
    const int mantissa = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, mantissa, result_bits, 0, 0 | 52<<8);
    const int mantissa_shift_denorm = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ADDI, mantissa_shift_denorm, exponent, 0, -862);
    const int is_denormal = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTUI, is_denormal, exponent, 0, 897);
    const int mantissa_shift_norm = rtl_imm32(unit, 35);
    const int mantissa_shift = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SELECT, mantissa_shift,
                 mantissa_shift_denorm, mantissa_shift_norm, is_denormal);
    const int tie_test = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_SLL, tie_test, mantissa, mantissa_shift, 0);
    const int tie_value = rtl_imm64(unit, UINT64_C(1)<<63);
    const int is_tie = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQ, is_tie, tie_test, tie_value, 0);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, is_tie, 0, label_out);

    /*
     * The result is a tie between two single-precision values, so we may
     * need to re-round it.  We do this by rerunning the operation in
     * round-toward-zero mode and checking the value and exactness of the
     * result.
     *
     * - If the result is exact, we don't need to do anything.
     *
     * - If the result has changed, then the exact result must be less than
     *   the double-precision representation, so we subtract 1 from the bit
     *   pattern and convert it to single precision.
     *
     * - If the result is unchanged but inexact, then the exact result must
     *   be greater than the double-precision representation, so we add 1
     *   to the bit pattern and convert it to single precision.
     */

    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
    const int clearexc = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FCLEAREXC, clearexc, fpstate, 0, 0);
    const int trunc_mode = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FSETROUND,
                 trunc_mode, clearexc, 0, RTLFROUND_TRUNC);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, trunc_mode, 0, 0);
    const int test_result = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, rtlop, test_result, frA, frC, frB);
    const int test_fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, test_fpstate, 0, 0, 0);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
    const int test_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BITCAST, test_bits, test_result, 0, 0);
    const int test_inexact = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC,
                 test_inexact, test_fpstate, 0, RTLFEXC_INEXACT);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, test_inexact, 0, label_out);

    const int test_mantissa = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, test_mantissa, test_bits, 0, 0 | 52<<8);
    const int unchanged = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQ, unchanged, test_mantissa, mantissa, 0);
    const int label_round_up = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, unchanged, 0, label_round_up);

    const int rounded_down_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ADDI, rounded_down_bits, result_bits, 0, -1);
    const int rounded_down = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST, rounded_down, rounded_down_bits, 0, 0);
    const int result_down = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_FCVT, result_down, rounded_down, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, result_down, 0, result_alias);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_round_up);
    const int rounded_up_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ADDI, rounded_up_bits, result_bits, 0, 1);
    const int rounded_up = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST, rounded_up, rounded_up_bits, 0, 0);
    const int result_up = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_FCVT, result_up, rounded_up, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, result_up, 0, result_alias);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
    const int result32 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, result32, 0, 0, result_alias);
    return result32;
}

/*-----------------------------------------------------------------------*/

/**
 * round_for_multiply:  Round a double-precision floating-point value
 * to be used as the frC operand to a single-precision multiply or
 * multiply-add operation.  Depending on the value of frC, the value of
 * frA may also be modified so that the multiply operation returns the
 * correct value.
 *
 * See the documentation of BINREC_OPT_G_PPC_FAST_FMULS for the rationale
 * behind this function.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     frA_ptr: Pointer to RTL register of FLOAT64 type holding the first
 *         multiplicand (frA); may be modified on return.
 *     frC_ptr: Pointer to RTL register of FLOAT64 type holding the second
 *         multiplicand (frC); may be modified on return.
 */
static void round_for_multiply(GuestPPCContext *ctx, int *frA_ptr,
                               int *frC_ptr)
{
    RTLUnit * const unit = ctx->unit;

    if (!ctx->alias_mulround_frA) {
        ctx->alias_mulround_frA =
            rtl_alloc_alias_register(unit, RTLTYPE_FLOAT64);
    }
    if (!ctx->alias_mulround_frC) {
        ctx->alias_mulround_frC =
            rtl_alloc_alias_register(unit, RTLTYPE_FLOAT64);
    }
    const int frA_alias = ctx->alias_mulround_frA;
    const int frC_alias = ctx->alias_mulround_frC;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, *frA_ptr, 0, frA_alias);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, *frC_ptr, 0, frC_alias);

    const int label_out = rtl_alloc_label(unit);

    /* If the mantissa is zero or either value is infinity/NaN, we don't
     * need to round anything. */
    const int frC_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BITCAST, frC_bits, *frC_ptr, 0, 0);
    const int frA_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BITCAST, frA_bits, *frA_ptr, 0, 0);
    const int frC_mantissa = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, frC_mantissa, frC_bits, 0, 0 | 52<<8);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, frC_mantissa, 0, label_out);
    const int frA_exponent = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, frA_exponent, frA_bits, 0, 52 | 11<<8);
    const int frC_exponent = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, frC_exponent, frC_bits, 0, 52 | 11<<8);
    const int frA_inf_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, frA_inf_test, frA_exponent, 0, 0x7FF);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, frA_inf_test, 0, label_out);
    const int frC_inf_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, frC_inf_test, frC_exponent, 0, 0x7FF);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, frC_inf_test, 0, label_out);

    /* If the second operand is a denormal, we normalize it before
     * rounding, adjusting the exponent of the other operand accordingly.
     * If the other operand becomes denormal, the product will round to
     * zero in any case, so we just abort and let the operation proceed
     * normally. */
    const int label_normalized = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, frC_exponent, 0, label_normalized);
    /* To normalize frC, we need to shift the mantissa left until we shift
     * out a 1 (which then moves to the exponent).  We could loop over a
     * single-bit shift, but it's much faster to count zero bits. */
    const int norm_temp = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_CLZ, norm_temp, frC_mantissa, 0, 0);
    const int norm_shift = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ADDI, norm_shift, norm_temp, 0, -11);
    const int frA_exp_new = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_SUB, frA_exp_new, frA_exponent, norm_shift, 0);
    const int frA_is_normal = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTSI, frA_is_normal, frA_exp_new, 0, 0);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, frA_is_normal, 0, label_out);
    /* Safe to normalize, so actually modify the values. */
    const int sign_bit = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, sign_bit, 0, 0, UINT64_C(1)<<63);
    const int frC_sign = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_AND, frC_sign, frC_bits, sign_bit, 0);
    const int frC_mantissa_shifted = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_SLL,
                 frC_mantissa_shifted, frC_mantissa, norm_shift, 0);
    const int frA_norm_adjusted = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFINS,
                 frA_norm_adjusted, frA_bits, frA_exp_new, 52 | 11<<8);
    const int frC_normalized = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_OR,
                 frC_normalized, frC_sign, frC_mantissa_shifted, 0);
    const int frA_norm_adjusted_fp = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST,
                 frA_norm_adjusted_fp, frA_norm_adjusted, 0, 0);
    const int frC_normalized_fp = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST, frC_normalized_fp, frC_normalized, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, frA_norm_adjusted_fp, 0, frA_alias);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, frC_normalized_fp, 0, frC_alias);

    /* Round the normalized value of frC.  Note that this rounding ignores
     * FPSCR[RN] and always rounds to nearest based on the bit in the
     * rounding position. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_normalized);
    const int frC_preround_fp = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, frC_preround_fp, 0, 0, frC_alias);
    const int frC_preround = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BITCAST, frC_preround, frC_preround_fp, 0, 0);
    const uint64_t round_bit = UINT64_C(1) << 27;
    const int frC_truncated = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ANDI, frC_truncated, frC_preround, 0, -round_bit);
    const int frC_round_bit = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ANDI, frC_round_bit, frC_preround, 0, round_bit);
    const int frC_rounded = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ADD,
                 frC_rounded, frC_truncated, frC_round_bit, 0);
    const int frC_rounded_fp = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST, frC_rounded_fp, frC_rounded, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, frC_rounded_fp, 0, frC_alias);

    /* If the rounding changed a large value into an infinity, subtract a
     * power of two from frC and add it to frA. */
    const int frC_rounded_exp = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT,
                 frC_rounded_exp, frC_rounded, 0, 52 | 11<<8);
    const int frC_rounded_inf_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI,
                 frC_rounded_inf_test, frC_rounded_exp, 0, 0x7FF);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, frC_rounded_inf_test, 0, label_out);
    const int exp_low_bit = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, exp_low_bit, 0, 0, UINT64_C(1)<<52);
    const int frC_halved = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_SUB, frC_halved, frC_rounded, exp_low_bit, 0);
    const int frC_halved_fp = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST, frC_halved_fp, frC_halved, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, frC_halved_fp, 0, frC_alias);

    /* frA might be denormal or huge, so we have to check the exponent. */
    const int frA_exponent_2 = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFEXT, frA_exponent_2, frA_bits, 0, 52 | 11<<8);
    const int label_frA_denorm = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                 0, frA_exponent_2, 0, label_frA_denorm);
    /* If doubling frA would turn it into an infinity, just leave it alone;
     * the multiply will overflow anyway. */
    const int frA_2_inf_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTUI, frA_2_inf_test, frA_exponent_2, 0, 0x7FD);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, frA_2_inf_test, 0, label_out);
    const int frA_doubled_norm = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ADD, frA_doubled_norm, frA_bits, exp_low_bit, 0);
    const int frA_doubled_norm_fp = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST,
                 frA_doubled_norm_fp, frA_doubled_norm, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, frA_doubled_norm_fp, 0, frA_alias);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_frA_denorm);
    const int frA_doubled_denorm_temp =
        rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_SLLI, frA_doubled_denorm_temp, frA_bits, 0, 1);
    const int frA_doubled_denorm = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_BFINS, frA_doubled_denorm,
                 frA_bits, frA_doubled_denorm_temp, 0 | 63<<8);
    const int frA_doubled_denorm_fp =
        rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST,
                 frA_doubled_denorm_fp, frA_doubled_denorm, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS,
                 0, frA_doubled_denorm_fp, 0, frA_alias);

    /* Return the possibly modified values. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
    const int new_frA = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, new_frA, 0, 0, frA_alias);
    const int new_frC = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, new_frC, 0, 0, frC_alias);
    *frA_ptr = new_frA;
    *frC_ptr = new_frC;
}

/*-----------------------------------------------------------------------*/

/**
 * set_fp_result:  Set FPR[index] and FPSCR based on the given
 * floating-point value and the current host exception state.  If an
 * invalid-operation exception has been raised and FPSCR[VE] is set, the
 * target FPR and FPSCR[FPRF] will not be written.
 *
 * This function will not work correctly for paired-single result values
 * if the operation has any non-SNaN invalid-operation exceptions and the
 * BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO (or BINREC_OPT_G_PPC_NO_FPSCR_STATE)
 * optimization is not enabled.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: Index of FPR to set.
 *     result: RTL register containing result value.
 *     fprf_slot: Paired-slot index (0 or 1) from which to set FPRF, if
 *         result is a paired-single value.
 *     frA, frB, frC: RTL registers containing operand values, or 0 for no
 *         operand.  Pass the second operand to fmul[s]/ps_mul in frB
 *         instead of frC.
 *     vxfoo_snan: FPSCR_VXFOO bitmask indicating the exception bit(s) to
 *         set for an invalid-operation exception involving SNaNs
 *         (FPSCR_VXSNAN is implicitly added to this bitmask).
 *     vxfoo_no_snan: FPSCR_VXFOO bitmask indicating which invalid
 *         exceptions other than VXSNAN can be raised.  Zero indicates
 *         that only SNaNs can trigger VX.
 *     check_vx: True to check for invalid-operation exceptions.
 *     check_zx: True to check for divide-by-zero exceptions.
 *     set_xx: True to set FPSCR[XX] when an inexact exception is raised;
 *         false to only set FPSCR[FI] (for fres).
 *     snan_safe: True if the result is known not to contain SNaNs, false
 *         for ps_sum[01].
 */
static void set_fp_result(GuestPPCContext *ctx, int index, int result,
                          int fprf_slot, int frA, int frB, int frC,
                          uint32_t vxfoo_snan, uint32_t vxfoo_no_snan,
                          bool check_vx, bool check_zx, bool set_xx,
                          bool snan_safe)
{
    if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE) {
        set_fpr(ctx, index, result);
        if (snan_safe) {
            ctx->fpr_is_safe |= 1 << index;
            if (ctx->unit->regs[result].type != RTLTYPE_FLOAT64) {
                ctx->ps1_is_safe |= 1 << index;
            }
        }
        return;
    }

    if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO) {
        vxfoo_no_snan = 0;
    }

    RTLUnit * const unit = ctx->unit;

    const int fpscr = get_fpscr(ctx);
    /* FPSCR is changed conditionally, so we can't save it. */
    ctx->live.fpscr = 0;
    ctx->last_set.fpscr = -1;
    ctx->live.fr_fi_fprf = 0;
    ctx->last_set.fr_fi_fprf = -1;
    /* Similarly for the output register. */
    flush_fpr(ctx, index, true);

    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
    const int clearexc = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FCLEAREXC, clearexc, fpstate, 0, 0);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, clearexc, 0, 0);

    int label_out = rtl_alloc_label(unit);
    int label_exception_abort = 0;

    if (check_vx) {
        const int invalid = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FTESTEXC,
                     invalid, fpstate, 0, RTLFEXC_INVALID);
        const int label_no_vx = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, invalid, 0, label_no_vx);

        int label_check_ve_snan = 0;

        if (vxfoo_no_snan) {
            const int label_snan = rtl_alloc_label(unit);
            int label_frB_snan = 0;
            if (frA) {
                check_snan(ctx, frA, label_snan);
            }
            if (frC) {
                check_snan(ctx, frC, label_snan);
            }
            if (vxfoo_no_snan == (FPSCR_VXIMZ | FPSCR_VXISI)) {
                /* Special handling for FMA instructions; see notes below. */
                label_frB_snan = rtl_alloc_label(unit);
                check_snan(ctx, frB, label_frB_snan);
            } else {
                check_snan(ctx, frB, label_snan);
            }

            /* If there are multiple exception types, we have to check for each
             * one separately. */
            int label_check_ve = 0;
            if (vxfoo_no_snan == (FPSCR_VXIDI | FPSCR_VXZDZ)) {  // fdiv, fdivs
                const int bits_type = (unit->regs[frA].type == RTLTYPE_FLOAT64
                                       ? RTLTYPE_INT64 : RTLTYPE_INT32);
                const int frA_bits = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_BITCAST, frA_bits, frA, 0, 0);
                const int frB_bits = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_BITCAST, frB_bits, frB, 0, 0);
                const int both_bits = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_OR, both_bits, frA_bits, frB_bits, 0);
                const int zero_test = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_SLLI, zero_test, both_bits, 0, 1);
                const int label_vxidi = rtl_alloc_label(unit);
                rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                             0, zero_test, 0, label_vxidi);
                set_fpscr_exceptions(ctx, fpscr, FPSCR_VXZDZ);
                if (ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN) {
                    label_check_ve_snan = rtl_alloc_label(unit);
                    rtl_add_insn(unit, RTLOP_GOTO,
                                 0, 0, 0, label_check_ve_snan);
                } else {
                    label_check_ve = rtl_alloc_label(unit);
                    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_check_ve);
                }
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_vxidi);
                set_fpscr_exceptions(ctx, fpscr, FPSCR_VXIDI);

            } else if (vxfoo_no_snan == (FPSCR_VXIMZ | FPSCR_VXISI)) {
                /* For an FMA operation to cause VX with no SNaNs, one of
                 * the following must be true:
                 *    - frA = +/-inf, frC = 0 (frB don't care)
                 *    - frA = 0, frC = +/-inf (frB don't care)
                 *    - frA = +/-inf, frC = nonzero, frB = -/+inf
                 *    - frA = nonzero, frC = +/-inf, frB = -/+inf
                 * In other words, at least one of frA and frC must be an
                 * infinity, so if we see that frA is not infinite, we can
                 * assume that frC is infinite without checking. */
                const int bits_type = (unit->regs[frA].type == RTLTYPE_FLOAT64
                                       ? RTLTYPE_INT64 : RTLTYPE_INT32);
                const int frA_bits = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_BITCAST, frA_bits, frA, 0, 0);
                const int frC_bits = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_BITCAST, frC_bits, frC, 0, 0);
                const int frA_zero_test = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_SLLI, frA_zero_test, frA_bits, 0, 1);
                const int frC_zero_test = rtl_alloc_register(unit, bits_type);
                rtl_add_insn(unit, RTLOP_SLLI, frC_zero_test, frC_bits, 0, 1);
                int frA_inf_test;
                if (bits_type == RTLTYPE_INT32) {
                    frA_inf_test = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_SEQI,
                                 frA_inf_test, frA_zero_test, 0, 0xFF000000);
                } else {
                    const int inf_sll1 =
                        rtl_alloc_register(unit, RTLTYPE_INT64);
                    rtl_add_insn(unit, RTLOP_LOAD_IMM,
                                 inf_sll1, 0, 0, UINT64_C(0xFFE0000000000000));
                    frA_inf_test = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_SEQ,
                                 frA_inf_test, frA_zero_test, inf_sll1, 0);
                }
                const int label_frC_inf = rtl_alloc_label(unit);
                rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                             0, frA_inf_test, 0, label_frC_inf);
                const int label_vximz = rtl_alloc_label(unit);
                rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                             0, frC_zero_test, 0, label_vximz);
                const int label_vxisi = rtl_alloc_label(unit);
                rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_vxisi);
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_frC_inf);
                rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                             0, frA_zero_test, 0, label_vxisi);
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_vximz);
                set_fpscr_exceptions(ctx, fpscr, FPSCR_VXIMZ);
                if (ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN) {
                    label_check_ve_snan = rtl_alloc_label(unit);
                    rtl_add_insn(unit, RTLOP_GOTO,
                                 0, 0, 0, label_check_ve_snan);
                } else {
                    label_check_ve = rtl_alloc_label(unit);
                    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_check_ve);
                }
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_vxisi);
                set_fpscr_exceptions(ctx, fpscr, FPSCR_VXISI);

            } else {
                /* Make sure only one VXFOO bit is set. */
                ASSERT((vxfoo_no_snan & (vxfoo_no_snan - 1)) == 0);
                set_fpscr_exceptions(ctx, fpscr, vxfoo_no_snan);
            }

            if (ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN) {
                if (!label_check_ve_snan) {
                    label_check_ve_snan = rtl_alloc_label(unit);
                }
                rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_check_ve_snan);
            } else {
                if (label_check_ve) {
                    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_check_ve);
                }
                const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr, 0, FPSCR_VE);
                int label_no_ve_default_nan = 0;
                label_no_ve_default_nan = rtl_alloc_label(unit);
                rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                             0, ve_test, 0, label_no_ve_default_nan);
                const int fr_fi_fprf = get_fr_fi_fprf(ctx);
                const int fr_fi_cleared =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI,
                             fr_fi_cleared, fr_fi_fprf, 0, 0x1F);
                set_fr_fi_fprf_and_flush(ctx, fr_fi_cleared);
                rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
                rtl_add_insn(unit, RTLOP_LABEL,
                             0, 0, 0, label_no_ve_default_nan);
                RTLDataType default_nan_type = unit->regs[result].type;
                const int default_nan =
                    rtl_alloc_register(unit, default_nan_type);
                rtl_add_insn(unit, RTLOP_LOAD_IMM, default_nan, 0, 0,
                             default_nan_type == RTLTYPE_FLOAT64
                             ? UINT64_C(0x7FF8000000000000) : 0x7FC00000);
                set_fpr_and_flush(ctx, index, default_nan, true);
                const int default_nan_fprf =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_LOAD_IMM,
                             default_nan_fprf, 0, 0, 0x11);
                set_fr_fi_fprf_and_flush(ctx, default_nan_fprf);
                rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
            }

            /* A fused multiply-add of the form inf*0+SNaN (or 0*inf-SNaN,
             * etc.) raises both VXIMZ and VXSNAN, so we need an extra
             * check if frB triggers VXSNAN. */
            if (vxfoo_no_snan == (FPSCR_VXIMZ | FPSCR_VXISI)) {
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_frB_snan);
                /* In the interest of not overly bloating the output code
                 * (possibly a lost cause at this point...) we just calculate
                 * frA*frC directly and see if that raises an exception.  We
                 * check frA and frC for SNaNs before frB, so if we get here,
                 * only inf*0 will trigger an invalid-operation exception.
                 * Note that exceptions are already cleared here, so we don't
                 * need an extra clear before the FMUL. */
                const int mul_test =
                    rtl_alloc_register(unit, unit->regs[frA].type);
                rtl_add_insn(unit, RTLOP_FMUL, mul_test, frA, frC, 0);
                const int mul_state =
                    rtl_alloc_register(unit, RTLTYPE_FPSTATE);
                rtl_add_insn(unit, RTLOP_FGETSTATE, mul_state, 0, 0, 0);
                rtl_add_insn(unit, RTLOP_FSETSTATE, 0, clearexc, 0, 0);
                const int mul_invalid =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_FTESTEXC,
                             mul_invalid, mul_state, 0, RTLFEXC_INVALID);
                rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                             0, mul_invalid, 0, label_snan);
                set_fpscr_exceptions(ctx, fpscr, FPSCR_VXSNAN | FPSCR_VXIMZ);
                if (!label_check_ve_snan) {
                    label_check_ve_snan = rtl_alloc_label(unit);
                }
                rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_check_ve_snan);
            }

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_snan);
        }  // if (vxfoo_no_snan)

        set_fpscr_exceptions(ctx, fpscr, FPSCR_VXSNAN | vxfoo_snan);
        if (label_check_ve_snan) {
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_check_ve_snan);
        }
        const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr, 0, FPSCR_VE);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, ve_test, 0, label_no_vx);

        if (check_zx) {
            /* We technically don't need this label if the result is a
             * paired-single value, but paired-single arithmetic operations
             * handle VX by falling back to scalars, so we'll never reach
             * this point with check_zx true and a paired-single value. */
            label_exception_abort = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_exception_abort);
        }
        const int fr_fi_fprf = get_fr_fi_fprf(ctx);
        const int fr_fi_cleared = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, fr_fi_cleared, fr_fi_fprf, 0, 0x1F);
        set_fr_fi_fprf_and_flush(ctx, fr_fi_cleared);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_vx);
    }

    if (check_zx) {
        const int zerodiv = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FTESTEXC,
                     zerodiv, fpstate, 0, RTLFEXC_ZERO_DIVIDE);
        const int label_no_zx = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, zerodiv, 0, label_no_zx);
        set_fpscr_exceptions(ctx, fpscr, FPSCR_ZX);
        const int ze_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ze_test, fpscr, 0, FPSCR_ZE);

        if (rtl_register_is_vector(&unit->regs[result])) {

            /* For ps_div, we still have to set FR/FI/FPRF before aborting,
             * along with any other exceptions if the other slot was not
             * also a zero-divide. */
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, ze_test, 0, label_no_zx);
            const RTLDataType scalar_type =
                rtl_vector_element_type(unit->regs[frA].type);
            const int frA_ps0 = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps0, frA, 0, 0);
            const int frA_ps1 = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps1, frA, 0, 1);
            const int frB_ps0 = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frB_ps0, frB, 0, 0);
            const int frB_ps1 = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frB_ps1, frB, 0, 1);

            int result_ps0 = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_FDIV, result_ps0, frA_ps0, frB_ps0, 0);
            if (scalar_type != RTLTYPE_FLOAT32) {
                const int result64 = result_ps0;
                result_ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_FCVT, result_ps0, result64, 0, 0);
            }
            const int fpstate_ps0 = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate_ps0, 0, 0, 0);

            int result_ps1 = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_FDIV, result_ps1, frA_ps1, frB_ps1, 0);
            /* Give the FDIV time to finish before we FCVT it. */
            const int zx_ps0 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_FTESTEXC,
                         zx_ps0, fpstate_ps0, 0, RTLFEXC_ZERO_DIVIDE);
            const int fprf_full_ps0 = gen_fprf(unit, result_ps0, 0);
            const int fprf_zero_ps0 = rtl_imm32(unit, 0);
            const int fprf_ps0 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SELECT,
                         fprf_ps0, fprf_zero_ps0, fprf_full_ps0, zx_ps0);
            if (scalar_type != RTLTYPE_FLOAT32) {
                const int result64 = result_ps1;
                result_ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_FCVT, result_ps1, result64, 0, 0);
            }
            const int fpstate_ps1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate_ps1, 0, 0, 0);
            rtl_add_insn(unit, RTLOP_FSETSTATE, 0, clearexc, 0, 0);

            const int zx_xx = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_FTESTEXC,
                         zx_xx, fpstate_ps1, 0, RTLFEXC_INEXACT);
            const int label_zx_no_xx = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, zx_xx, 0, label_zx_no_xx);
            set_fpscr_exceptions(ctx, 0, FPSCR_XX);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_zx_no_xx);

            const int zx_ox = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_FTESTEXC,
                         zx_ox, fpstate_ps1, 0, RTLFEXC_OVERFLOW);
            const int label_zx_no_ox = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, zx_ox, 0, label_zx_no_ox);
            set_fpscr_exceptions(ctx, 0, FPSCR_OX);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_zx_no_ox);

            const int zx_ux = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_FTESTEXC,
                         zx_ux, fpstate_ps1, 0, RTLFEXC_UNDERFLOW);
            const int label_zx_no_ux = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, zx_ux, 0, label_zx_no_ux);
            set_fpscr_exceptions(ctx, 0, FPSCR_UX);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_zx_no_ux);

            const int fi_sll5 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, fi_sll5, zx_xx, 0, 5);
            const int fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, fi_fprf, fi_sll5, fprf_ps0, 0);
            set_fr_fi_fprf_and_flush(ctx, fi_fprf);

            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

        } else {  // Not paired-single.

            /* The only cases in which we pass check_vx = false are for
             * paired-single results, so if we get here, we must have
             * already come through the VX check and thus
             * label_exception_abort will be set. */
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, ze_test, 0, label_exception_abort);

        }  // if (rtl_register_is_vector(&unit->regs[result]))

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_zx);
    }  // if (check_zx)

    set_fpr_and_flush(ctx, index, result, snan_safe);

    const int fprf = gen_fprf(unit, result, fprf_slot);

    const int inexact = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC, inexact, fpstate, 0, RTLFEXC_INEXACT);
    const int shifted_fi = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_fi, inexact, 0, 5);
    const int fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, fi_fprf, fprf, shifted_fi, 0);
    set_fr_fi_fprf_and_flush(ctx, fi_fprf);
    if (set_xx) {
        const int label_no_xx = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, inexact, 0, label_no_xx);
        set_fpscr_exceptions(ctx, 0, FPSCR_XX);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_xx);
    }

    const int overflow = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC, overflow, fpstate, 0, RTLFEXC_OVERFLOW);
    const int label_no_ox = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, overflow, 0, label_no_ox);
    set_fpscr_exceptions(ctx, 0, FPSCR_OX);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_ox);

    const int underflow = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC,
                 underflow, fpstate, 0, RTLFEXC_UNDERFLOW);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, underflow, 0, label_out);
    set_fpscr_exceptions(ctx, 0, FPSCR_UX);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
}

/*-----------------------------------------------------------------------*/

/**
 * set_ps_result:  Set FPR[index] and FPSCR based on the given
 * paired-single value and the current host exception state.  If an
 * invalid-operation exception has been raised and FPSCR[VE] is set, the
 * target FPR will not be written.
 *
 * Analog of set_fp_result() for paired-single instructions which can
 * generate invalid-operation exceptions.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: Index of FPR to set.
 *     result: RTL register containing result value.
 *     rtlop: RTL opcode for the operation to be performed, RTLOP_FDIV for
 *         ps_res, or RTLOP_FSQRT for ps_rsqrte.
 *     src1: First source operand (vector of 1.0f for ps_res and ps_rsqrte).
 *     src2: Second source operand.
 *     one: RTL register containing 1.0f for ps_res and ps_rsqrte; 0 for
 *         other operations.
 *     use_float32: True if source operands are of type FLOAT32, false if
 *         they are of type FLOAT64.
 *     vxfoo_no_snan: FPSCR_VXFOO bitmask indicating which invalid
 *         exceptions other than VXSNAN can be raised.  Zero indicates
 *         that only SNaNs can trigger VX.
 */
static void set_ps_result(GuestPPCContext *ctx, int index, int result,
                          RTLOpcode rtlop, int src1, int src2, int one,
                          bool use_float32, uint32_t vxfoo_no_snan)
{
    const bool is_res = (one && rtlop == RTLOP_FDIV);
    const bool is_rsqrte = (one && rtlop == RTLOP_FSQRT);
    ASSERT(is_res || is_rsqrte || !one);
    const RTLOpcode real_rtlop = is_rsqrte ? RTLOP_FDIV : rtlop;
    const RTLDataType type =
        use_float32 ? RTLTYPE_V2_FLOAT32 : RTLTYPE_V2_FLOAT64;

    RTLUnit * const unit = ctx->unit;

    /* If an invalid-operation exception occurred and either (1) we're
     * setting VXFOO exception flags or (2) we want to match guest NaN
     * behavior, we have to re-run the operation separately on each
     * paired-single slot, since each slot could raise a different VXFOO
     * exception or we might need to write a PowerPC default QNaN.
     * (FIXME: We should find some way to make these callable subroutines
     * instead of having to inline them at every guest instruction.) */
    int label_skip_set_result = 0;
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)
        && (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO)
            || !(ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN))) {
        const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
        const int has_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FTESTEXC,
                     has_vx, fpstate, 0, RTLFEXC_INVALID);
        const int label_do_set_result = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, has_vx, 0, label_do_set_result);

        /* Save the current value of the output register so we can restore
         * it later in case FPSCR[VE] is set. */
        int saved_frD;
        if (ctx->live.fpr[index]) {
            saved_frD = ctx->live.fpr[index];
            ctx->live.fpr[index] = 0;
            ctx->fpr_dirty &= ~(1 << index);
            ctx->fpr_is_safe &= ~(1 << index);
            ctx->ps1_is_safe &= ~(1 << index);
        } else {
            saved_frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         saved_frD, 0, 0, ctx->alias.fpr[index]);
        }

        const int fpstate_cleared = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FCLEAREXC, fpstate_cleared, fpstate, 0, 0);
        rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_cleared, 0, 0);

        /* Perform the operation and call set_fp_result() to set
         * appropriate exception flags (and possibly modify the result)
         * for each slot.  We use the frD alias as a temporary for
         * simplicity's sake. */
        const int scalar_type = rtl_vector_element_type(type);
        int result_ps[2], fi_fprf_ps[2];
        for (int slot = 0; slot < 2; slot++) {
            int op_src1;
            if (one) {
                op_src1 = one;
            } else {
                op_src1 = rtl_alloc_register(unit, scalar_type);
                rtl_add_insn(unit, RTLOP_VEXTRACT, op_src1, src1, 0, slot);
            }
            const int src2_ps = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, src2_ps, src2, 0, slot);
            int op_src2;
            if (is_rsqrte) {
                op_src2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_FSQRT, op_src2, src2_ps, 0, 0);
            } else {
                op_src2 = src2_ps;
            }
            result_ps[slot] = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, real_rtlop,
                         result_ps[slot], op_src1, op_src2, 0);
            if (!use_float32) {
                const int result64 = result_ps[slot];
                result_ps[slot] = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_FCVT,
                             result_ps[slot], result64, 0, 0);
            }
            set_fp_result(ctx, index, result_ps[slot], 0, op_src1, src2_ps, 0,
                          0, vxfoo_no_snan, true, real_rtlop == RTLOP_FDIV,
                          !(is_res || is_rsqrte), true);
            /* This will always be a direct alias access, so we might as
             * well avoid extra conversions to and from float32. */
            result_ps[slot] = get_fpr_as_type(ctx, index, RTLTYPE_FLOAT64);
            ctx->live.fpr[index] = 0;
            if (is_rsqrte && slot == 1) {
                fi_fprf_ps[slot] = 0;  // Not used.
            } else {
                fi_fprf_ps[slot] = get_fr_fi_fprf(ctx);
            }
        }

        /* FPRF is set from ps0 only; FI is set if either slot is inexact.
         * This update takes place regardless of whether FPSCR[VE] is set.
         * (That's probably a hardware bug, but if we've come this far we
         * might as well try to stay bug-compatible.) */
        int final_fi_fprf;
        if (is_rsqrte) {  // ps_rsqrte doesn't set FI.
            final_fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         final_fi_fprf, fi_fprf_ps[0], 0, 0x1F);
        } else {
            const int fi_ps1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, fi_ps1, fi_fprf_ps[1], 0, 0x20);
            final_fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR,
                         final_fi_fprf, fi_fprf_ps[0], fi_ps1, 0);
        }
        set_fr_fi_fprf_and_flush(ctx, final_fi_fprf);

        /* Merge the two outputs to frD, or restore the original value of
         * frD if FPSCR[VE] is set. */
        const int fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
        const int has_ve = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, has_ve, fpscr, 0, FPSCR_VE);
        const int label_no_ve = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, has_ve, 0, label_no_ve);
        set_fpr_and_flush(ctx, index, saved_frD, false);
        label_skip_set_result = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_skip_set_result);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_ve);
        const int new_result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_VBUILD2,
                     new_result, result_ps[0], result_ps[1], 0);
        set_fpr_and_flush(ctx, index, new_result, true);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_skip_set_result);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_do_set_result);
    }

    /* If no invalid-operation exception occurred, we can just store the
     * result directly. */
    set_fp_result(ctx, index, result, 0, src1, src2, 0,
                  0, 0, false, real_rtlop == RTLOP_FDIV, true, true);
    if (label_skip_set_result) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_set_result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_nia:  Set the NIA field of the processor state block to the value
 * of the given RTL register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register containing value to store.
 */
static inline void set_nia(GuestPPCContext *ctx, int reg)
{
    rtl_add_insn(ctx->unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.nia);
}

/*-----------------------------------------------------------------------*/

/**
 * set_nia_imm:  Set the NIA field of the processor state block to the
 * given literal value.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     nia: Address to store in the state block's NIA field.
 */
static inline void set_nia_imm(GuestPPCContext *ctx, uint32_t nia)
{
    set_nia(ctx, rtl_imm32(ctx->unit, nia));
}

/*-----------------------------------------------------------------------*/

/**
 * update_cr0:  Add RTL instructions to update the value of CR0 based on
 * the result of an integer operation.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     result: RTL register containing operation result.
 */
static void update_cr0(GuestPPCContext *ctx, int result)
{
    RTLUnit * const unit = ctx->unit;

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTSI, lt, result, 0, 0);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTSI, gt, result, 0, 0);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, eq, result, 0, 0);
    const int so = get_xer_so(ctx);
    set_crf(ctx, 0, lt, gt, eq, so);
}

/*-----------------------------------------------------------------------*/

/**
 * update_cr1:  Add RTL instructions to update the value of CR1 based on
 * the high 4 bits of FPSCR.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
static void update_cr1(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    const int fpscr = get_fpscr(ctx);
    int fex, vx;
    get_fpscr_fex_vx(ctx, fpscr, &fex, &vx);
    const int fx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, fx, fpscr, 0, 31 | 1<<8);
    const int ox = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, ox, fpscr, 0, 28 | 1<<8);
    set_crf(ctx, 1, fx, fex, vx, ox);
}

/*-----------------------------------------------------------------------*/

/**
 * update_rounding_mode:  Add RTL instructions to update the current
 * floating-point rounding mode based on the RN field of FPSCR.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
static void update_rounding_mode(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    const int label_n = rtl_alloc_label(unit);
    const int label_z = rtl_alloc_label(unit);
    const int label_p = rtl_alloc_label(unit);
    const int label_out = rtl_alloc_label(unit);

    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
    const int fpscr = get_fpscr(ctx);
    const int rn = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, rn, fpscr, 0, FPSCR_RN);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, rn, 0, label_n);
    const int test_1 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTUI, test_1, rn, 0, 2);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, test_1, 0, label_z);
    const int test_2 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, test_2, rn, 0, 2);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, test_2, 0, label_p);

    const int fpstate_m = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FSETROUND,
                 fpstate_m, fpstate, 0, RTLFROUND_FLOOR);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_m, 0, 0);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_p);
    const int fpstate_p = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FSETROUND,
                 fpstate_p, fpstate, 0, RTLFROUND_CEIL);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_p, 0, 0);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_z);
    const int fpstate_z = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FSETROUND,
                 fpstate_z, fpstate, 0, RTLFROUND_TRUNC);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_z, 0, 0);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_n);
    const int fpstate_n = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FSETROUND,
                 fpstate_n, fpstate, 0, RTLFROUND_NEAREST);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_n, 0, 0);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
}

/*-----------------------------------------------------------------------*/

/**
 * return_from_unit:  Add RTL instructions to return from the current
 * translation unit.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of the current instruction, or ~0 to suppress
 *         calling the post-instruction callback (if any).
 *     nia: RTL register containing the address of the next instruction to
 *         execute.
 *     need_flush: True if live registers should be flushed before returning.
 */
static void return_from_unit(GuestPPCContext *ctx, uint32_t address,
                             int nia, bool need_flush)
{
    RTLUnit * const unit = ctx->unit;

    if (need_flush) {
        flush_live_regs(ctx, true);
    }
    set_nia(ctx, nia);
    if (address != ~0u) {
        post_insn_callback(ctx, address);
    }

    if (ctx->handle->use_chaining
     && unit->regs[nia].source == RTLREG_CONSTANT) {
        guest_ppc_flush_cr(ctx, false);
        guest_ppc_flush_fpscr(ctx);
        const int chain_insn =
            rtl_add_chain_insn(unit, ctx->psb_reg, ctx->membase_reg);
        const int lookup_func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD, lookup_func, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_chain_lookup);
        const int lookup_result = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_CALL,
                     lookup_result, lookup_func, ctx->psb_reg, nia);
        rtl_add_insn(unit, RTLOP_CHAIN_RESOLVE,
                     0, lookup_result, 0, chain_insn);
        rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);
    } else {
        rtl_add_insn(unit, RTLOP_GOTO,
                     0, 0, 0, guest_ppc_get_epilogue_label(ctx));
    }
}

/*************************************************************************/
/*************************** Translation core ****************************/
/*************************************************************************/

/**
 * translate_illegal:  Translate an illegal instruction word.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 */
static inline void translate_illegal(GuestPPCContext *ctx, uint32_t insn)
{
    rtl_add_insn(ctx->unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_arith_imm:  Translate an integer register-immediate arithmetic
 * instruction.  For addi and addis, rA is assumed to be nonzero.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     shift_imm: True if the immediate value should be shifted left 16 bits.
 *     set_ca: True if XER[CA] should be set according to the result.
 *     set_cr0: True if CR0 should be set according to the result.
 */
static void translate_arith_imm(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool shift_imm,
    bool set_ca, bool set_cr0)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, insn_rA(insn));
    const int32_t imm = shift_imm ? insn_SIMM(insn)<<16 : insn_SIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rA, 0, imm);
    set_gpr(ctx, insn_rD(insn), result);

    if (set_ca) {
        ASSERT(rtlop == RTLOP_ADDI);
        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTUI, ca, result, 0, imm);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer, -1);
    }

    if (set_cr0) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_addsub_reg:  Translate an integer register-register add or
 * subtract instruction.
 *
 * This function implements addition and subtraction as a three-operand
 * addition operation following the PowerPC documentation, which is
 * relatively slow both to translate and to execute.  Simple operations
 * which set no flags should add appropriate RTL instructions directly
 * rather than calling this function.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     srcB_sel: Selector indicating the third operand to the addition:
 *         0 or -1 (constant), or 1 (use rB).
 *     srcC_sel: Selector indicating the third operand to the addition:
 *         0 or 1 (constant), or -1 to use XER[CA].
 *     invert_rA: True to invert (one's-complement) rA, for a subf operation.
 *     set_ca: True if XER[CA] should be set according to the result.
 */
static void translate_addsub_reg(
    GuestPPCContext *ctx, uint32_t insn, int srcB_sel, int srcC_sel,
    bool invert_rA, bool set_ca)
{
    RTLUnit * const unit = ctx->unit;

    int result = 0;

    int srcA = get_gpr(ctx, insn_rA(insn));
    if (invert_rA) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, srcA, 0, 0);
        srcA = inverted;
    }

    int srcB = 0;
    if (srcB_sel > 0) {
        srcB = get_gpr(ctx, insn_rB(insn));
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, srcA, srcB, 0);
    } else if (srcB_sel < 0) {
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, result, srcA, 0, -1);
    }

    if (srcC_sel > 0) {
        const int temp = result ? result : srcA;
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, result, temp, 0, 1);
    } else if (srcC_sel < 0) {
        const int temp = result ? result : srcA;
        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFEXT, ca, xer, 0, XER_CA_SHIFT | 1<<8);
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, temp, ca, 0);
    }

    set_gpr(ctx, insn_rD(insn), result);

    /* Extract high bits of values needed below so we don't have to do it
     * twice. */
    const int a = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, a, srcA, 0, 31);
    int b = 0, a_xor_b = 0;
    if (srcB_sel > 0) {
        b = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SRLI, b, srcB, 0, 31);
        a_xor_b = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XOR, a_xor_b, a, b, 0);
    }
    const int r = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, r, result, 0, 31);

    if (set_ca) {
        /* Carry calculation: XER[CA] = (a && b) || ((a != b) && !result)
         * where a, b, and result are the high bit of each value.
         * (Conceptually: carry always occurs if the high bit of both
         * inputs is set; when the high bit of exactly one input is set,
         * carry occurred if the high bit of the result is clear.)
         * We can also treat "a ^ b" as "a | b", since the latter gives
         * the same result, and we do so in the srcB_sel < 0 case. */
        int ca;
        const int nr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, nr, r, 0, 1);
        if (srcB_sel > 0) {
            const int a_and_b = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, a_and_b, a, b, 0);
            const int and_nr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, and_nr, a_xor_b, nr, 0);
            ca = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, ca, a_and_b, and_nr, 0);
        } else if (srcB_sel < 0) {  // b = 1 --> CA = a | !result
            ca = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, ca, a, nr, 0);
        } else {  // b = 0 --> CA = a & !result
            ca = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ca, a, nr, 0);
        }
        const int xer = get_xer(ctx);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer, -1);
    }

    if (insn_OE(insn)) {
        /* Overflow calculation: XER[OV] = (a == b) && (a != result)
         * (where a, b, and result are the high bit of each value)
         * which we implement as: !(a ^ b) & (a ^ result) */
        int ov;
        if (srcB_sel > 0) {
            const int n_a_xor_b = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, n_a_xor_b, a_xor_b, 0, 1);
            const int a_xor_r = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XOR, a_xor_r, a, r, 0);
            ov = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ov, n_a_xor_b, a_xor_r, 0);
        } else if (srcB_sel < 0) {  // b = 1 --> OV = a & !result
            const int nr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, nr, r, 0, 1);
            ov = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ov, a, nr, 0);
        } else {  // b = 0 --> OV = !a & result
            const int na = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, na, a, 0, 1);
            ov = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ov, na, r, 0);
        }
        const int xer = get_xer(ctx);
        const int masked_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, ~XER_OV);
        const int SO_OV = rtl_imm32(unit, XER_SO | XER_OV);
        const int bits_to_set = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, bits_to_set, SO_OV, ov, ov);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, new_xer, masked_xer, bits_to_set, 0);
        set_xer(ctx, new_xer, 0);
    }

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_bitmisc:  Translate the miscellaneous bit manipulation
 * instructions (cntlzw, extsh, extsb).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL instruction to perform the operation.
 */
static void translate_bitmisc(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, insn_rS(insn));
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, 0, 0);
    set_gpr(ctx, insn_rA(insn), result);

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_branch_label:  Translate a branch to another block within the
 * current unit.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     target: Branch target address.
 *     target_label: RTL label for the target block.
 */
static void translate_branch_label(
    GuestPPCContext *ctx, uint32_t address, int BO, int BI,
    uint32_t target, int target_label)
{
    const binrec_t * const handle = ctx->handle;
    RTLUnit * const unit = ctx->unit;

    const bool is_conditional = ((BO & 0x14) != 0x14);

    int skip_label;
    if ((BO & 0x14) == 0 || (is_conditional && handle->use_branch_exit_test)) {
        /* Need an extra label in case a non-final test fails. */
        skip_label = rtl_alloc_label(unit);
    } else {
        skip_label = 0;
    }

    RTLOpcode branch_op = RTLOP_GOTO;
    int test_reg = 0;

    uint32_t crb_store_branch = 0;
    uint32_t crb_store_next = 0;
    uint16_t crb_reg[32];
    RTLInsn crb_insn[32];  // Copy of the instruction that sets each value.
    if (ctx->trim_cr_stores) {
        guest_ppc_trim_cr_stores(ctx, BO, BI, &crb_store_branch,
                                 &crb_store_next, crb_reg, crb_insn);
        /* If there are bits to store on the branch-taken path for a
         * conditional branch, we need a label for skipping past the branch
         * even for a single condition (much like when the branch exit test
         * is enabled). */
        if (crb_store_branch && !skip_label) {
            skip_label = rtl_alloc_label(unit);
        }
    }

    if (!(BO & 0x04)) {
        const int ctr = get_ctr(ctx);
        const int new_ctr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, new_ctr, ctr, 0, -1);
        set_ctr(ctx, new_ctr);

        /* Flush here so any update to CTR is stored along with other
         * pending changes. */
        flush_live_regs(ctx, false);

        if (skip_label) {
            rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                         0, new_ctr, 0, skip_label);
        } else {
            branch_op = BO & 0x02 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ;
            test_reg = new_ctr;
        }
    } else {
        flush_live_regs(ctx, false);
    }
    /* All dirty registers have been flushed at this point. */

    if (!(BO & 0x10)) {
        const int test = test_crb(ctx, BI);
        if (handle->use_branch_exit_test || crb_store_branch) {
            rtl_add_insn(unit, BO & 0x08 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ,
                         0, test, 0, skip_label);
        } else {
            branch_op = BO & 0x08 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z;
            test_reg = test;
        }
    }

    while (crb_store_branch) {
        const int bit = ctz32(crb_store_branch);
        crb_store_branch ^= 1 << bit;
        if (crb_insn[bit].opcode) {
            rtl_add_insn_copy(unit, &crb_insn[bit]);
        }
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, crb_reg[bit], 0, ctx->alias.crb[bit]);
    }

    if (handle->post_insn_callback) {
        set_nia_imm(ctx, target);
    }
    post_insn_callback(ctx, address);
    if (handle->use_branch_exit_test) {
        ASSERT(branch_op == RTLOP_GOTO);
        const int flag = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD, flag, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_branch_exit_flag);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, flag, 0, target_label);
        set_nia_imm(ctx, target);
        rtl_add_insn(unit, RTLOP_GOTO,
                     0, 0, 0, guest_ppc_get_epilogue_label(ctx));
    } else {
        rtl_add_insn(unit, branch_op, 0, test_reg, 0, target_label);
    }

    if (skip_label) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);
    }

    while (crb_store_next) {
        const int bit = ctz32(crb_store_next);
        crb_store_next ^= 1 << bit;
        if (crb_insn[bit].opcode) {
            rtl_add_insn_copy(unit, &crb_insn[bit]);
        }
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, crb_reg[bit], 0, ctx->alias.crb[bit]);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_branch_terminal:  Translate a branch which terminates
 * execution of the current unit.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     LK: LK bit of instruction word.
 *     target: Target address, if an immediate branch (ignored for bclr/bcctr).
 *     target_lr: True if the instruction is bclr.
 *     target_ctr: True if the instruction is bcctr.
 */
static void translate_branch_terminal(
    GuestPPCContext *ctx, uint32_t address, int BO, int BI, int LK,
    uint32_t target, bool target_lr, bool target_ctr)
{
    RTLUnit * const unit = ctx->unit;

    int skip_label;
    if ((BO & 0x14) == 0x14) {
        skip_label = 0;  // Unconditional branch.
    } else {
        skip_label = rtl_alloc_label(unit);
    }

    /* For a terminal branch, we can only have dead stores if the branch
     * is conditional. */
    uint32_t crb_store_branch = 0;
    uint16_t crb_reg[32];
    RTLInsn crb_insn[32];
    if ((BO & 0x14) != 0x14 && ctx->trim_cr_stores) {
        uint32_t crb_store_next;
        guest_ppc_trim_cr_stores(ctx, BO, BI, &crb_store_branch,
                                 &crb_store_next, crb_reg, crb_insn);
        ASSERT(!crb_store_next);
    }

    if (!(BO & 0x04)) {
        const int ctr = get_ctr(ctx);
        const int new_ctr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, new_ctr, ctr, 0, -1);
        set_ctr(ctx, new_ctr);

        flush_live_regs(ctx, false);

        rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                     0, new_ctr, 0, skip_label);
    } else {
        flush_live_regs(ctx, false);
    }

    if (!(BO & 0x10)) {
        const int test = test_crb(ctx, BI);
        rtl_add_insn(unit, BO & 0x08 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ,
                     0, test, 0, skip_label);
    }

    int nia;
    if (target_lr || target_ctr) {
        const int nia_raw = target_lr ? get_lr(ctx) : get_ctr(ctx);
        nia = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, nia, nia_raw, 0, -4);
    } else {
        nia = rtl_imm32(unit, target);
    }

    while (crb_store_branch) {
        const int bit = ctz32(crb_store_branch);
        crb_store_branch ^= 1 << bit;
        if (crb_insn[bit].opcode) {
            rtl_add_insn_copy(unit, &crb_insn[bit]);
        }
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, crb_reg[bit], 0, ctx->alias.crb[bit]);
    }

    if (LK) {
        /* Write LR directly rather than going through set_lr() so we don't
         * leak the modified LR to the not-taken code path. */
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, rtl_imm32(unit, address+4), 0, ctx->alias.lr);
    }
    return_from_unit(ctx, address, nia, false);

    if (skip_label) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_branch:  Translate an immediate-displacement branch instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     disp: Displacement field of instruction word.
 *     AA: AA bit of instruction word.
 *     LK: LK bit of instruction word.
 */
static void translate_branch(
    GuestPPCContext *ctx, uint32_t address, int BO, int BI, int32_t disp,
    int AA, int LK)
{
    ASSERT((address & 3) == 0);
    ASSERT((disp & 3) == 0);

    uint32_t target;
    if (AA) {
        target = (uint32_t)disp;
    } else {
        target = address + disp;
    }

    int target_label = 0;
    if (!LK) {
        ASSERT(ctx->num_blocks > 0);
        const uint32_t unit_start = ctx->blocks[0].start;
        const uint32_t unit_end = (ctx->blocks[ctx->num_blocks-1].start
                                   + ctx->blocks[ctx->num_blocks-1].len - 1);
        if (target >= unit_start && target <= unit_end) {
            int lo = 0, hi = ctx->num_blocks - 1;
            /* We break blocks at all branch targets, so this search should
             * always terminate at a match. */
            while (!target_label) {
                ASSERT(lo <= hi);
                const int mid = (lo + hi + 1) / 2;
                const GuestPPCBlockInfo *mid_block = &ctx->blocks[mid];
                const uint32_t mid_start = mid_block->start;
                if (target == mid_start) {
                    target_label = mid_block->label;
                } else if (target < mid_start) {
                    hi = mid - 1;
                } else {
                    /* The target should never be in the middle of a block,
                     * since we split blocks at branch targets during
                     * scanning. */
                    ASSERT(target > mid_start + mid_block->len - 1);
                    lo = mid + 1;
                }
            }
        }
    }

    if (target_label) {
        translate_branch_label(ctx, address, BO, BI, target, target_label);
    } else {
        translate_branch_terminal(ctx, address, BO, BI, LK,
                                  target, false, false);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_compare:  Translate an integer compare instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_imm: True for a register-immediate compare (D-form instruction),
 *         false for a register-register compare (X-form instruction).
 *     is_signed: True for a signed compare, false for an unsigned compare.
 */
static void translate_compare(
    GuestPPCContext *ctx, uint32_t insn, bool is_imm, bool is_signed)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, insn_rA(insn));

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    if (is_imm) {
        const int32_t imm =
            is_signed ? insn_SIMM(insn) : (int32_t)insn_UIMM(insn);
        rtl_add_insn(unit, is_signed ? RTLOP_SLTSI : RTLOP_SLTUI,
                     lt, rA, 0, imm);
        rtl_add_insn(unit, is_signed ? RTLOP_SGTSI : RTLOP_SGTUI,
                     gt, rA, 0, imm);
        rtl_add_insn(unit, RTLOP_SEQI, eq, rA, 0, imm);
    } else {
        const int rB = get_gpr(ctx, insn_rB(insn));
        rtl_add_insn(unit, is_signed ? RTLOP_SLTS : RTLOP_SLTU, lt, rA, rB, 0);
        rtl_add_insn(unit, is_signed ? RTLOP_SGTS : RTLOP_SGTU, gt, rA, rB, 0);
        rtl_add_insn(unit, RTLOP_SEQ, eq, rA, rB, 0);
    }

    const int so = get_xer_so(ctx);

    set_crf(ctx, insn_crfD(insn), lt, gt, eq, so);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_compare_fp:  Translate a floating-point compare instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     ordered: True for an ordered comparison (invalid exception on QNaN),
 *         false for an unordered comparison.
 *     ps_index: Paired-single slot index to compare (0 or 1).
 */
static void translate_compare_fp(
    GuestPPCContext *ctx, uint32_t insn, bool ordered, int ps_index)
{
    RTLUnit * const unit = ctx->unit;

    const int obit = ordered ? RTLFCMP_ORDERED : 0;

    int frA, frB;
    if (ps_index == 1) {
        frA = get_ps1(ctx, insn_frA(insn), RTLTYPE_FLOAT32);
        frB = get_ps1(ctx, insn_frB(insn), RTLTYPE_FLOAT32);
    } else {
        frA = get_fpr_as_type(ctx, insn_frA(insn), RTLTYPE_FLOAT64);
        frB = get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_FLOAT64);
    }

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, lt, frA, frB, obit | RTLFCMP_LT);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, gt, frA, frB, obit | RTLFCMP_GT);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, eq, frA, frB, obit | RTLFCMP_EQ);
    const int un = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, un, frA, frB, obit | RTLFCMP_UN);

    set_crf(ctx, insn_crfD(insn), lt, gt, eq, un);

    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int fr_fi_fprf = get_fr_fi_fprf(ctx);
        const int masked = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, masked, fr_fi_fprf, 0, 0x70);
        const int shifted_lt = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, shifted_lt, lt, 0, 3);
        const int shifted_gt = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, shifted_gt, gt, 0, 2);
        const int shifted_eq = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, shifted_eq, eq, 0, 1);
        const int lt_gt = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, lt_gt, shifted_lt, shifted_gt, 0);
        const int eq_un = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, eq_un, shifted_eq, un, 0);
        const int fpcc = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, fpcc, lt_gt, eq_un, 0);
        const int merged = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, merged, masked, fpcc, 0);
        set_fr_fi_fprf(ctx, merged);

        const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
        const int invalid = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FTESTEXC,
                     invalid, fpstate, 0, RTLFEXC_INVALID);
        const int label_out = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, invalid, 0, label_out);
        const int clearexc = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FCLEAREXC, clearexc, fpstate, 0, 0);
        rtl_add_insn(unit, RTLOP_FSETSTATE, 0, clearexc, 0, 0);
        const int fpscr = get_fpscr(ctx);
        /* FPSCR is changed conditionally, so we can't save it. */
        ctx->live.fpscr = 0;
        ctx->last_set.fpscr = -1;
        if (ordered && !(ctx->handle->guest_opt
                         & BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO)) {
            /* If neither value is a SNaN, this is a VXVC exception from
             * ordered comparison of a QNaN. */
            const int label_snan = rtl_alloc_label(unit);
            check_snan(ctx, frA, label_snan);
            check_snan(ctx, frB, label_snan);
            set_fpscr_exceptions(ctx, fpscr, FPSCR_VXVC);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_snan);
            /* If this is a VXSNAN exception, we only set VXVC if VE is
             * clear. */
            const int label_no_vxvc = rtl_alloc_label(unit);
            const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr, 0, FPSCR_VE);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, ve_test, 0, label_no_vxvc);
            set_fpscr_exceptions(ctx, fpscr, FPSCR_VXSNAN | FPSCR_VXVC);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_vxvc);
        }
        set_fpscr_exceptions(ctx, fpscr, FPSCR_VXSNAN);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_dcbz:  Translate a dcbz or dcbz_l instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 */
static void translate_dcbz(GuestPPCContext *ctx, uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    int addr32;
    if (insn_rA(insn)) {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        addr32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, addr32, rA, rB, 0);
    } else {
        addr32 = get_gpr(ctx, insn_rB(insn));
    }
    const int addr32_aligned = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, addr32_aligned, addr32, 0, -32);
    const int addr_aligned = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ZCAST, addr_aligned, addr32_aligned, 0, 0);
    const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ADD,
                 host_address, ctx->membase_reg, addr_aligned, 0);

    const int zero_64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, zero_64, 0, 0, 0);
    const int zero_128 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
    rtl_add_insn(unit, RTLOP_VBROADCAST, zero_128, zero_64, 0, 0);
    rtl_add_insn(unit, RTLOP_STORE, 0, host_address, zero_128, 0);
    rtl_add_insn(unit, RTLOP_STORE, 0, host_address, zero_128, 16);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fctiw:  Translate an fctiw or fctiwz instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     convert_op: RTL opcode for the conversion (FROUNDI or FTRUNCI).
 */
static void translate_fctiw(GuestPPCContext *ctx, uint32_t insn,
                            RTLOpcode convert_op)
{
    RTLUnit * const unit = ctx->unit;

    /* There's no need to convert from single to double precision if the
     * register is currently in single precision; the result would be the
     * same either way. */
    const RTLDataType source_type = get_fpr_scalar_type(ctx, insn_frB(insn));
    const int frB = get_fpr_as_type(ctx, insn_frB(insn), source_type);

    /* Do the actual conversion, and handle positive overflow if necessary. */
    const int conv_result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, convert_op, conv_result, frB, 0, 0);
    const int imm_2_31 = rtl_alloc_register(unit, source_type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, imm_2_31, 0, 0,
                 (source_type == RTLTYPE_FLOAT64
                  ? UINT64_C(0x41DFFFFFFFC00000)  // 2147483647.0
                  : UINT64_C(0x4F000000)));       // 2147483648.0f
    const int overflow = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, overflow, frB, imm_2_31, RTLFCMP_GE);
    const int imm_intmax = rtl_imm32(unit, 0x7FFFFFFF);
    const int result32 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SELECT,
                 result32, imm_intmax, conv_result, overflow);

    /* Set the high word of the result appropriately, unless disabled by
     * optimization flags. */
    int result64 = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_ZCAST, result64, result32, 0, 0);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FCTIW)) {
        RTLDataType frB_bits_type;
        uint64_t negative_zero_val;
        if (unit->regs[frB].type == RTLTYPE_FLOAT32) {
            frB_bits_type = RTLTYPE_INT32;
            negative_zero_val = UINT64_C(1) << 31;
        } else {
            frB_bits_type = RTLTYPE_INT64;
            negative_zero_val = UINT64_C(1) << 63;
        }
        const int frB_bits = rtl_alloc_register(unit, frB_bits_type);
        rtl_add_insn(unit, RTLOP_BITCAST, frB_bits, frB, 0, 0);
        const int negative_zero = rtl_alloc_register(unit, frB_bits_type);
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     negative_zero, 0, 0, negative_zero_val);
        const int is_neg_zero = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SEQ, is_neg_zero, frB_bits, negative_zero, 0);
        const int inz_shifted = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SLLI, inz_shifted, is_neg_zero, 0, 32);
        const int high_word_base = rtl_imm64(unit, 0xFFF8000000000000);
        const int high_word = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_OR,
                     high_word, high_word_base, inz_shifted, 0);
        const int new_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_OR, new_bits, result64, high_word, 0);
        result64 = new_bits;
    }

    const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_BITCAST, result, result64, 0, 0);

    if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE) {
        set_fpr(ctx, insn_frD(insn), result);
        ctx->fpr_is_safe |= 1 << insn_frD(insn);
        if (insn_Rc(insn)) {
            update_cr1(ctx);
        }
        return;
    }

    /* Check for exceptions, like set_fp_result().  We can't call that
     * function directly because we don't return NaNs on exceptions.
     * We can also omit the overflow and underflow checks since fctiw[z]
     * doesn't raise those exceptions. */

    const int fpscr = get_fpscr(ctx);
    ctx->live.fpscr = 0;
    ctx->last_set.fpscr = -1;
    ctx->live.fr_fi_fprf = 0;
    ctx->last_set.fr_fi_fprf = -1;
    flush_fpr(ctx, insn_frD(insn), true);

    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
    const int clearexc = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FCLEAREXC, clearexc, fpstate, 0, 0);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, clearexc, 0, 0);

    int label_out = rtl_alloc_label(unit);

    const int invalid = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC, invalid, fpstate, 0, RTLFEXC_INVALID);
    const int label_no_vx = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, invalid, 0, label_no_vx);

    int label_check_ve_snan = 0;
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO)) {
        const int label_snan = rtl_alloc_label(unit);
        check_snan(ctx, frB, label_snan);
        set_fpscr_exceptions(ctx, fpscr, FPSCR_VXCVI);
        label_check_ve_snan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_check_ve_snan);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_snan);
    }
    set_fpscr_exceptions(ctx, fpscr, FPSCR_VXSNAN | FPSCR_VXCVI);
    if (label_check_ve_snan) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_check_ve_snan);
    }

    const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr, 0, FPSCR_VE);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, ve_test, 0, label_no_vx);
    const int fr_fi_fprf = get_fr_fi_fprf(ctx);
    const int fr_fi_cleared = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, fr_fi_cleared, fr_fi_fprf, 0, 0x1F);
    set_fr_fi_fprf_and_flush(ctx, fr_fi_cleared);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_vx);

    set_fpr_and_flush(ctx, insn_frD(insn), result, true);
    const int fprf = gen_fprf(unit, result, 0);

    const int inexact = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC, inexact, fpstate, 0, RTLFEXC_INEXACT);
    const int shifted_fi = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_fi, inexact, 0, 5);
    const int fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, fi_fprf, fprf, shifted_fi, 0);
    set_fr_fi_fprf_and_flush(ctx, fi_fprf);
    const int label_no_xx = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, inexact, 0, label_no_xx);
    set_fpscr_exceptions(ctx, 0, FPSCR_XX);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_xx);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * check_fp_underflow:  Check for, and raise if appropriate, an underflow
 * exception on a floating-point result whose magnitude is the minimum
 * normal value for its type.
 *
 * The PowerPC architecture defines underflow as "tiny before rounding",
 * while x86 (at least) defines it as "tiny after rounding".  If we're not
 * ignoring precise underflow behavior and the result is equal to the
 * smallest (in magnitude) normal number, we may need to manually raise an
 * underflow exception in order to match PowerPC behavior.  This function
 * checks for this case and, if it applies, runs the operation a second
 * time in round-toward-zero mode, which will raise an underflow exception
 * if appropriate regardless of whether the host detects underflow before
 * or after rounding.
 *
 * This function does nothing if underflow handling is disabled by
 * optimizations.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     result: Result of the operation.
 *     rtlop: RTL opcode to perform the operation.
 *     src1, src2, src3: RTL instruction operands.
 *     is_single: True if a single-precision (32-bit) operation, false if
 *         a double-precision (64-bit) operation.
 *     is_paired: True if a paired-single operation, false if not.
 *     use_float32: True if the input operands (src1/src2/src3) are of
 *         type FLOAT32 (V2_FLOAT32 for a paired-single operation), false
 *         if they are of type FLOAT64 (V2_FLOAT64 for paired-single).
 */
static void check_fp_underflow(
    GuestPPCContext *ctx, int result, RTLOpcode rtlop, int src1,
    int src2, int src3, bool is_single, bool is_paired, bool use_float32)
{
    if ((ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_UNDERFLOW)
     || (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        return;
    }

    RTLUnit * const unit = ctx->unit;

    const int bits_type = is_single ? RTLTYPE_INT32 : RTLTYPE_INT64;
    const int min_normal_x2 = rtl_alloc_register(unit, bits_type);
    if (is_single) {
        rtl_add_insn(unit, RTLOP_LOAD_IMM, min_normal_x2, 0, 0, 0x00800000<<1);
    } else {
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     min_normal_x2, 0, 0, UINT64_C(0x0010000000000000)<<1);
    }
    int label_check_tiny = 0;
    if (is_paired) {
        ASSERT(is_single);
        const int result_ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_VEXTRACT, result_ps1, result, 0, 1);
        const int result_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BITCAST, result_bits, result_ps1, 0, 0);
        const int result_bits_x2 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, result_bits_x2, result_bits, 0, 1);
        const int is_min_normal = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQ,
                     is_min_normal, result_bits_x2, min_normal_x2, 0);
        label_check_tiny = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                     0, is_min_normal, 0, label_check_tiny);
        const int result_ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_VEXTRACT, result_ps0, result, 0, 0);
        result = result_ps0;
    }
    const int result_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BITCAST, result_bits, result, 0, 0);
    /* Ignore the sign bit by shifting it out. */
    const int result_bits_x2 = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_SLLI, result_bits_x2, result_bits, 0, 1);
    const int is_min_normal = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQ,
                 is_min_normal, result_bits_x2, min_normal_x2, 0);
    const int label_skip_tiny = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, is_min_normal, 0, label_skip_tiny);

    /* Note that we don't actually need to save this result; we just need
     * to execute the operations so that the underflow exception is raised
     * if appropriate.  The floating-point side effect check will prevent
     * these technically dead stores from being eliminated (unless
     * BINREC_OPT_DSE_FP is enabled, in which case this entire check is
     * meaningless). */
    if (label_check_tiny) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_check_tiny);
    }
    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
    const int fpstate_trunc = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FSETROUND,
                 fpstate_trunc, fpstate, 0, RTLFROUND_TRUNC);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_trunc, 0, 0);
    const RTLDataType type = is_paired
        ? (use_float32 ? RTLTYPE_V2_FLOAT32 : RTLTYPE_V2_FLOAT64)
        : (use_float32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64);
    const int dummy_result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, rtlop, dummy_result, src1, src2, src3);
    if (is_single && !use_float32) {
        const RTLDataType type32 =
            is_paired ? RTLTYPE_V2_FLOAT32 : RTLTYPE_FLOAT32;
        const int dummy_result32 = rtl_alloc_register(unit, type32);
        rtl_add_insn(unit, is_paired ? RTLOP_VFCVT : RTLOP_FCVT,
                     dummy_result32, dummy_result, 0, 0);
    }
    const int fpstate2 = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate2, 0, 0, 0);
    const int fpstate2_oldround = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FCOPYROUND,
                 fpstate2_oldround, fpstate2, fpstate, 0);
    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate2_oldround, 0, 0);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_tiny);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fp_arith:  Translate a two-operand floating-point arithmetic
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL opcode to perform the operation.
 *     is_single: True if a single-precision (32-bit) operation, false if
 *         a double-precision (64-bit) operation.
 *     vxfoo_no_snan: FPSCR_VXFOO bitmask indicating which non-VXSNAN
 *         exception(s) can be raised by the instruction.
 */
static void translate_fp_arith(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool is_single,
    uint32_t vxfoo_no_snan)
{
    RTLUnit * const unit = ctx->unit;

    const int src1_fpr = insn_frA(insn);
    const int src2_fpr = (rtlop==RTLOP_FMUL ? insn_frC(insn) : insn_frB(insn));

    bool use_float32 = false;
    if (is_single) {
        if (get_fpr_scalar_type(ctx, src1_fpr) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, src2_fpr) == RTLTYPE_FLOAT32) {
            use_float32 = true;
        }
    }

    const RTLDataType type = use_float32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64;
    int src1 = get_fpr_as_type(ctx, src1_fpr, type);
    int src2 = get_fpr_as_type(ctx, src2_fpr, type);
    if (is_single && rtlop == RTLOP_FMUL && type == RTLTYPE_FLOAT64
     && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMULS)) {
        round_for_multiply(ctx, &src1, &src2);
    }
    int result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, rtlop, result, src1, src2, 0);
    if (is_single && !use_float32) {
        const int result64 = result;
        result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_FCVT, result, result64, 0, 0);
    }

    check_fp_underflow(ctx, result, rtlop, src1, src2, 0, is_single, false,
                       use_float32);

    set_fp_result(ctx, insn_frD(insn), result, 0, src1, src2, 0,
                  0, vxfoo_no_snan, true, rtlop == RTLOP_FDIV, true, true);
    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fp_fma:  Translate a floating-point multiply-add instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL opcode to perform the operation.
 *     is_single: True if a single-precision (32-bit) operation, false if a
 *         double-precision (64-bit) operation.
 *     negate: True to negate a non-NaN result.
 */
static void translate_fp_fma(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool is_single,
    bool negate)
{
    RTLUnit * const unit = ctx->unit;

    bool use_float32 = false;
    if (is_single) {
        if (get_fpr_scalar_type(ctx, insn_frA(insn)) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, insn_frB(insn)) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, insn_frC(insn)) == RTLTYPE_FLOAT32) {
            use_float32 = true;
        }
    }

    const RTLDataType type = use_float32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64;
    int frA = get_fpr_as_type(ctx, insn_frA(insn), type);
    int frC = get_fpr_as_type(ctx, insn_frC(insn), type);
    int frB = get_fpr_as_type(ctx, insn_frB(insn), type);
    if (is_single && type == RTLTYPE_FLOAT64
     && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMULS)) {
        round_for_multiply(ctx, &frA, &frC);
    }

    int result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, rtlop, result, frA, frC, frB);

    if (negate) {
        result = fma_negate(ctx, result);
    }

    /* If the result is a NaN copied from an input operand, we need to
     * ensure that the proper input operand is selected (unless the
     * NATIVE_IEEE_NAN optimization is enabled, in which case we don't
     * care).  The RTL FMADD-group instructions are defined to choose in
     * the order {src1, src2, src3}, which for us is {frA, frC, frB}, but
     * PowerPC gives frB precedence over frC, so we have to manually load
     * the frB NaN in that case. */
    if (!(ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN)) {
        result = fma_select_nan(ctx, result, frA, frB, frC);
    }

    if (is_single && !use_float32) {
        if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMADDS) {
            const int result64 = result;
            result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_FCVT, result, result64, 0, 0);
        } else {
            result = round_fma_result_to_single(
                ctx, result, rtlop, frA, frB, frC);
        }
    }

    check_fp_underflow(ctx, result, rtlop, frA, frC, frB, is_single, false,
                       use_float32);

    set_fp_result(ctx, insn_frD(insn), result, 0, frA, frB, frC,
                  0, FPSCR_VXIMZ | FPSCR_VXISI, true, false, true, true);

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fres_lookup:  Translate a reciprocal estimate operation using
 * lookup tables.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     input: RTL register containing the source value (type FLOAT32).
 *     output: RTL alias to which to store the result.
 *     label_abort: RTL label to which to jump if the operation is aborted
 *         due to an unmasked invalid-operation exception.  May be zero if
 *         the BINREC_OPT_G_PPC_NO_FPSCR_STATE optimization is enabled.
 */
static void translate_fres_lookup(GuestPPCContext *ctx, int input, int output,
                                  int label_abort)
{
    RTLUnit * const unit = ctx->unit;

    const int alias_exp = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
    const int alias_mant = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
    const int label_out = rtl_alloc_label(unit);
    int label_pre_abort = 0, alias_fi = 0, fpscr_in = 0;
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        label_pre_abort = rtl_alloc_label(unit);
        alias_fi = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        fpscr_in = get_fpscr(ctx);
        ctx->live.fpscr = 0;
        ctx->last_set.fpscr = -1;
        ctx->live.fr_fi_fprf = 0;
        ctx->last_set.fr_fi_fprf = -1;
    }

    /* Extract the sign bit, exponent, and mantissa, and check for special
     * cases. */
    const int input_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BITCAST, input_bits, input, 0, 0);
    const int sign = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, sign, input_bits, 0, 1<<31);
    const int mantissa = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, mantissa, input_bits, 0, 0 | 23<<8);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, mantissa, 0, alias_mant);
    const int exponent = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, exponent, input_bits, 0, 23 | 8<<8);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, exponent, 0, alias_exp);
    const int label_exp_0 = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, exponent, 0, label_exp_0);
    const int exp_is_255 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, exp_is_255, exponent, 0, 255);
    const int label_exp_255 = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, exp_is_255, 0, label_exp_255);
    const int label_continue = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_continue);

    /* Handle an infinite or NaN input. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_exp_255);
    const int label_nan = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, mantissa, 0, label_nan);
    const int zero_result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_BITCAST, zero_result, sign, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, zero_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int pzero = rtl_imm32(unit, 0x02);
        const int nzero = rtl_imm32(unit, 0x12);
        const int fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, fprf, nzero, pzero, sign);
        set_fr_fi_fprf_and_flush(ctx, fprf);
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_nan);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int quiet_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, quiet_test, mantissa, 0, 0x400000);
        const int label_snan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, quiet_test, 0, label_snan);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, input, 0, output);
        set_fr_fi_fprf_and_flush(ctx, rtl_imm32(unit,0x11));
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_snan);
        set_fpscr_exceptions(ctx, 0, FPSCR_VXSNAN);
        const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr_in, 0, FPSCR_VE);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, ve_test, 0, label_pre_abort);
    }
    const int quiet_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, quiet_bits, input_bits, 0, 0x400000);
    const int quiet_result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_BITCAST, quiet_result, quiet_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, quiet_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fr_fi_fprf_and_flush(ctx, rtl_imm32(unit,0x11));
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    /* Handle a zero or denormalized input. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_exp_0);
    const int label_not_zero = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, mantissa, 0, label_not_zero);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fpscr_exceptions(ctx, 0, FPSCR_ZX);
        const int ze_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ze_test, fpscr_in, 0, FPSCR_ZE);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, ze_test, 0, label_pre_abort);
    }
    const int inf_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, inf_bits, sign, 0, 0x7F800000);
    const int inf_result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_BITCAST, inf_result, inf_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, inf_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int pinf = rtl_imm32(unit, 0x05);
        const int ninf = rtl_imm32(unit, 0x09);
        const int fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, fprf, ninf, pinf, sign);
        set_fr_fi_fprf_and_flush(ctx, fprf);
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_zero);
    const int overflow_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTUI, overflow_test, mantissa, 0, 0x200000);
    const int label_not_overflow = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                 0, overflow_test, 0, label_not_overflow);
    const int huge_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, huge_bits, sign, 0, 0x7F7FFFFF);
    const int huge_result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_BITCAST, huge_result, huge_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, huge_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fpscr_exceptions(ctx, 0, FPSCR_OX);
        const int pnorm_fi = rtl_imm32(unit, 0x24);
        const int nnorm_fi = rtl_imm32(unit, 0x28);
        const int fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, fprf, nnorm_fi, pnorm_fi, sign);
        set_fr_fi_fprf_and_flush(ctx, fprf);
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_overflow);
    const int shifted_mant_1 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_mant_1, mantissa, 0, 1);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, shifted_mant_1, 0, alias_mant);
    const int normalized_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI,
                 normalized_test, shifted_mant_1, 0, 0x800000);
    const int label_normalized = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                 0, normalized_test, 0, label_normalized);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, rtl_imm32(unit,-1), 0, alias_exp);
    const int shifted_mant_2 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_mant_2, shifted_mant_1, 0, 1);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, shifted_mant_2, 0, alias_mant);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_normalized);
    const int normalized_mant = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, normalized_mant, 0, 0, alias_mant);
    const int cleared_800000 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI,
                 cleared_800000, normalized_mant, 0, 0x7FFFFF);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, cleared_800000, 0, alias_mant);

    /* Calculate the new exponent and mantissa. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_continue);
    const int lut = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD, lut, ctx->psb_reg, 0,
                 ctx->handle->setup.state_offset_fres_lut);
    const int mantissa_in = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, mantissa_in, 0, 0, alias_mant);
    const int index = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, index, mantissa_in, 0, 18);
    const int index4 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, index4, index, 0, 2);
    const int cst_253 = rtl_imm32(unit, 253);
    const int exponent_in = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, exponent_in, 0, 0, alias_exp);
    const int new_exp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SUB, new_exp, cst_253, exponent_in, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_exp, 0, alias_exp);
    const int index4_addr = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ZCAST, index4_addr, index4, 0, 0);
    const int entry_addr = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ADD, entry_addr, lut, index4_addr, 0);
    const int delta = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_U16, delta, entry_addr, 0, 2);
    const int base = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_U16, base, entry_addr, 0, 0);
    const int mult = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, mult, mantissa_in, 0, 8 | 10<<8);
    const int mult_delta = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_MUL, mult_delta, mult, delta, 0);
    const int shifted_base = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_base, base, 0, 10);
    const int lookup_result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SUB, lookup_result, shifted_base, mult_delta, 0);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int fi = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, fi, lookup_result, 0, 1);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, fi, 0, alias_fi);
    }
    const int lookup_mant = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, lookup_mant, lookup_result, 0, 1);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, lookup_mant, 0, alias_mant);

    /* Denormalize the result if the exponent is not positive. */
    const int label_denormalize_1 = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, new_exp, 0, label_denormalize_1);
    const int exp_neg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTSI, exp_neg, new_exp, 0, 0);
    const int label_denormalize_2 = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, exp_neg, 0, label_denormalize_2);

    /* Build and store the final value, and update FPSCR if appropriate. */
    const int label_finalize = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_finalize);
    const int final_mantissa = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, final_mantissa, 0, 0, alias_mant);
    const int sign_mant = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, sign_mant, final_mantissa, sign, 0);
    const int final_exponent = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, final_exponent, 0, 0, alias_exp);
    const int shifted_final_exp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_final_exp, final_exponent, 0, 23);
    const int final_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, final_bits, sign_mant, shifted_final_exp, 0);
    const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
    rtl_add_insn(unit, RTLOP_BITCAST, result, final_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int fprf = gen_fprf(unit, result, 0);
        const int fi = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fi, 0, 0, alias_fi);
        const int shifted_fi = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, shifted_fi, fi, 0, 5);
        const int label_no_ux = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, shifted_fi, 0, label_no_ux);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                     0, shifted_final_exp, 0, label_no_ux);
        set_fpscr_exceptions(ctx, 0, FPSCR_UX);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_ux);
        const int fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, fi_fprf, shifted_fi, fprf, 0);
        set_fr_fi_fprf_and_flush(ctx, fi_fprf);
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    /* Handle denormalizing a tiny result. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_denormalize_2);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, rtl_imm32(unit,0), 0, alias_exp);
    const int denorm2_mant_in = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, denorm2_mant_in, 0, 0, alias_mant);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int fi_in = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fi_in, 0, 0, alias_fi);
        const int mant_low_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, mant_low_bits, denorm2_mant_in, 0, 3);
        const int fi_add = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTUI, fi_add, mant_low_bits, 0, 0);
        const int new_fi = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, new_fi, fi_in, fi_add, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_fi, 0, alias_fi);
    }
    const int denorm2_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, denorm2_temp, denorm2_mant_in, 0, 0x800000);
    const int denorm2_mant_out = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, denorm2_mant_out, denorm2_temp, 0, 2);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, denorm2_mant_out, 0, alias_mant);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_finalize);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_denormalize_1);
    const int denorm1_mant_in = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, denorm1_mant_in, 0, 0, alias_mant);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int fi_in = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fi_in, 0, 0, alias_fi);
        const int fi_add = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, fi_add, denorm1_mant_in, 0, 1);
        const int new_fi = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, new_fi, fi_in, fi_add, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_fi, 0, alias_fi);
    }
    const int denorm1_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, denorm1_temp, denorm1_mant_in, 0, 0x800000);
    const int denorm1_mant_out = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, denorm1_mant_out, denorm1_temp, 0, 1);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, denorm1_mant_out, 0, alias_mant);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_finalize);

    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_pre_abort);
        const int fr_fi_fprf = get_fr_fi_fprf(ctx);
        const int fr_fi_cleared = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, fr_fi_cleared, fr_fi_fprf, 0, 0x1F);
        set_fr_fi_fprf_and_flush(ctx, fr_fi_fprf);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_abort);
    }

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_frsqrte_lookup:  Translate a reciprocal square root estimate
 * operation using lookup tables.
 *
 * It's worth noting that although the PowerPC 750CL manual states, in
 * relation to the lack of a single-precision version of frsqrte, that
 * "both frB and frD are representable in single-precision format" (which
 * itself is a nonsensical statement, since frB is an input rather than an
 * output operand), the actual implementation can generate values which
 * are _not_ representable in single precision, even on single-precision
 * inputs.  For example, the single-precision input 0x4080_0100 gives the
 * double-precision result 0x3FDF_FE61_7000_0000, which has 24 bits of
 * precision in the mantissa.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     input: RTL register containing the source value (type FLOAT32 or
 *         FLOAT64).
 *     output: RTL alias to which to store the result.
 *     label_abort: RTL label to which to jump if the operation is aborted
 *         due to an unmasked invalid-operation exception.  May be zero if
 *         the BINREC_OPT_G_PPC_NO_FPSCR_STATE optimization is enabled.
 */
static void translate_frsqrte_lookup(GuestPPCContext *ctx, int input,
                                     int output, int label_abort)
{
    RTLUnit * const unit = ctx->unit;

    const bool is32 = (unit->regs[input].type == RTLTYPE_FLOAT32);
    const RTLDataType type = is32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64;
    const RTLDataType bits_type = is32 ? RTLTYPE_INT32 : RTLTYPE_INT64;

    const int alias_exp = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
    const int alias_mant_hi = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
    const int label_out = rtl_alloc_label(unit);
    int label_pre_abort = 0, fpscr_in = 0;
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        label_pre_abort = rtl_alloc_label(unit);
        fpscr_in = get_fpscr(ctx);
        ctx->live.fpscr = 0;
        ctx->last_set.fpscr = -1;
        ctx->live.fr_fi_fprf = 0;
        ctx->last_set.fr_fi_fprf = -1;
    }

    /* Extract the sign bit, exponent, and mantissa, and check for special
     * cases. */
    const int input_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BITCAST, input_bits, input, 0, 0);
    int input_hibits;
    if (is32) {
        input_hibits = input_bits;
    } else {
        const int input_hibits_64 = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_SRLI, input_hibits_64, input_bits, 0, 32);
        input_hibits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, input_hibits, input_hibits_64, 0, 0);
    }
    const int sign_mask = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM,
                 sign_mask, 0, 0, is32 ? 1u<<31 : UINT64_C(1)<<63);
    const int sign = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_AND, sign, input_bits, sign_mask, 0);
    const int mantissa_hi = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, mantissa_hi, input_hibits, 0,
                 0 | (is32 ? 23 : 20) << 8);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, mantissa_hi, 0, alias_mant_hi);
    const int exponent = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, exponent, input_hibits, 0,
                 is32 ? 23 | 8<<8 : 20 | 11<<8);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, exponent, 0, alias_exp);
    const int label_exp_0 = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, exponent, 0, label_exp_0);
    const int exp_is_max = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, exp_is_max, exponent, 0, is32 ? 255 : 2047);
    const int label_exp_max = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, exp_is_max, 0, label_exp_max);
    const int label_vxsqrt = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, sign, 0, label_vxsqrt);
    const int label_continue = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_continue);

    /* Handle an infinite or NaN input. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_exp_max);
    const int expmax_mantissa = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BFEXT, expmax_mantissa, input_bits, 0,
                 0 | (is32 ? 23 : 52) << 8);
    const int label_nan = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, expmax_mantissa, 0, label_nan);
    /* Negative infinity turns into a NaN. */
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, sign, 0, label_vxsqrt);
    const int zero_result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, zero_result, 0, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, zero_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fr_fi_fprf_and_flush(ctx, rtl_imm32(unit,0x02));
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_nan);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int quiet_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, quiet_test, mantissa_hi, 0,
                     is32 ? 0x400000 : 0x80000);
        const int label_snan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, quiet_test, 0, label_snan);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, input, 0, output);
        set_fr_fi_fprf_and_flush(ctx, rtl_imm32(unit,0x11));
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_snan);
        set_fpscr_exceptions(ctx, 0, FPSCR_VXSNAN);
        const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr_in, 0, FPSCR_VE);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, ve_test, 0, label_pre_abort);
    }
    const int quiet_mask = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM,
                 quiet_mask, 0, 0, is32 ? 1<<22 : UINT64_C(1)<<51);
    const int quiet_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_OR, quiet_bits, input_bits, quiet_mask, 0);
    const int quiet_result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_BITCAST, quiet_result, quiet_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, quiet_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fr_fi_fprf_and_flush(ctx, rtl_imm32(unit,0x11));
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    /* Handle a zero or denormalized input. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_exp_0);
    const int exp0_mantissa = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BFEXT, exp0_mantissa, input_bits, 0,
                 0 | (is32 ? 23 : 52) << 8);
    const int label_not_zero = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, exp0_mantissa, 0, label_not_zero);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fpscr_exceptions(ctx, 0, FPSCR_ZX);
        const int ze_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ze_test, fpscr_in, 0, FPSCR_ZE);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, ze_test, 0, label_pre_abort);
    }
    const int pinf_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, pinf_bits, 0, 0,
                 is32 ? 0x7F800000 : UINT64_C(0x7FF0000000000000));
    const int inf_bits = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_OR, inf_bits, sign, pinf_bits, 0);
    const int inf_result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_BITCAST, inf_result, inf_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, inf_result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int pinf = rtl_imm32(unit, 0x05);
        const int ninf = rtl_imm32(unit, 0x09);
        const int fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, fprf, ninf, pinf, sign);
        set_fr_fi_fprf_and_flush(ctx, fprf);
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_not_zero);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, sign, 0, label_vxsqrt);
    const int norm_mant_clz = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_CLZ, norm_mant_clz, exp0_mantissa, 0, 0);
    const int norm_mant_shift = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ADDI,
                 norm_mant_shift, norm_mant_clz, 0, is32 ? -8 : -11);
    const int norm_neg_exp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ADDI,
                 norm_neg_exp, norm_mant_clz, 0, is32 ? -9 : -12);
    const int norm_shifted_mant64 = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_SLL,
                 norm_shifted_mant64, exp0_mantissa, norm_mant_shift, 0);
    const int norm_exp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_NEG, norm_exp, norm_neg_exp, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, norm_exp, 0, alias_exp);
    const int norm_mant64_hi = rtl_alloc_register(unit, bits_type);
    rtl_add_insn(unit, RTLOP_BFEXT, norm_mant64_hi, norm_shifted_mant64, 0,
                 is32 ? 0 | 23<<8 : 32 | 20<<8);
    const int norm_mant_hi = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ZCAST, norm_mant_hi, norm_mant64_hi, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, norm_mant_hi, 0, alias_mant_hi);

    /* Calculate the new exponent and mantissa. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_continue);
    const int lut = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD, lut, ctx->psb_reg, 0,
                 ctx->handle->setup.state_offset_frsqrte_lut);
    const int mantissa_in = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, mantissa_in, 0, 0, alias_mant_hi);
    const int index_mant = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, index_mant, mantissa_in, 0, is32 ? 19 : 16);
    const int index4_mant = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, index4_mant, index_mant, 0, 2);
    const int exponent_in = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS, exponent_in, 0, 0, alias_exp);
    const int exp_lowbit = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, exp_lowbit, exponent_in, 0, 1);
    const int exp_inv_lowbit = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_XORI, exp_inv_lowbit, exp_lowbit, 0, 1);
    const int exp_lowbit_sll6 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, exp_lowbit_sll6, exp_inv_lowbit, 0, 6);
    const int index4 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, index4, index4_mant, exp_lowbit_sll6, 0);
    const int cst_3068 = rtl_imm32(unit, is32 ? 380 : 3068);
    const int subbed_exp = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SUB, subbed_exp, cst_3068, exponent_in, 0);
    const int final_exponent = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, final_exponent, subbed_exp, 0, 1);
    const int index4_addr = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ZCAST, index4_addr, index4, 0, 0);
    const int entry_addr = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ADD, entry_addr, lut, index4_addr, 0);
    const int delta = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_U16, delta, entry_addr, 0, 2);
    const int base = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_U16, base, entry_addr, 0, 0);
    const int mult = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT,
                 mult, mantissa_in, 0, (is32 ? 8 : 5) | 11<<8);
    const int mult_delta = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_MUL, mult_delta, mult, delta, 0);
    const int shifted_base = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_base, base, 0, 11);
    const int lookup_result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SUB, lookup_result, shifted_base, mult_delta, 0);

    /* Build and store the final value, and update FPSCR if appropriate. */
    int final_bits;
    if (is32) {
        const int final_mant = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_SRLI, final_mant, lookup_result, 0, 3);
        const int shifted_exp = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_SLLI, shifted_exp, final_exponent, 0, 23);
        const int sign_mant = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_OR, sign_mant, final_mant, sign, 0);
        final_bits = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_OR, final_bits, sign_mant, shifted_exp, 0);
    } else {
        const int lookup_mant64 = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_ZCAST, lookup_mant64, lookup_result, 0, 0);
        const int final_exp64 = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_ZCAST, final_exp64, final_exponent, 0, 0);
        const int final_mant64 = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_SLLI, final_mant64, lookup_mant64, 0, 26);
        const int shifted_exp64 = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_SLLI, shifted_exp64, final_exp64, 0, 52);
        const int sign_mant64 = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_OR, sign_mant64, final_mant64, sign, 0);
        final_bits = rtl_alloc_register(unit, bits_type);
        rtl_add_insn(unit, RTLOP_OR, final_bits, sign_mant64, shifted_exp64, 0);
    }
    const int result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_BITCAST, result, final_bits, 0, 0);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, result, 0, output);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        const int fprf = gen_fprf(unit, result, 0);
        set_fr_fi_fprf_and_flush(ctx, fprf);
    }
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    /* Handle an invalid (negative) input. */
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_vxsqrt);
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        set_fpscr_exceptions(ctx, 0, FPSCR_VXSQRT);
        const int ve_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, ve_test, fpscr_in, 0, FPSCR_VE);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, ve_test, 0, label_pre_abort);
        set_fr_fi_fprf_and_flush(ctx, rtl_imm32(unit,0x11));
    }
    const int qnan_result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, qnan_result, 0, 0,
                 is32 ? 0x7FC00000 : UINT64_C(0x7FF8000000000000));
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, qnan_result, 0, output);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_pre_abort);
        const int fr_fi_fprf = get_fr_fi_fprf(ctx);
        const int fr_fi_cleared = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, fr_fi_cleared, fr_fi_fprf, 0, 0x1F);
        set_fr_fi_fprf_and_flush(ctx, fr_fi_cleared);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_abort);
    }

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fp_recip:  Translate an fres or frsqrte instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_rsqrte: True if the instruction is frsqrte, false if fres.
 */
static void translate_fp_recip(GuestPPCContext *ctx, uint32_t insn,
                               bool is_rsqrte)
{
    RTLUnit * const unit = ctx->unit;

    const RTLDataType type = is_rsqrte ? RTLTYPE_FLOAT64 : RTLTYPE_FLOAT32;
    const int frB = get_fpr_as_type(ctx, insn_frB(insn), type);

    if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NATIVE_RECIPROCAL) {
        int div_src;
        if (is_rsqrte) {
            div_src = rtl_alloc_register(unit, type);
            rtl_add_insn(unit, RTLOP_FSQRT, div_src, frB, 0, 0);
        } else {
            div_src = frB;
        }
        const int one = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, one, 0, 0,
                     type==RTLTYPE_FLOAT64 ? UINT64_C(0x3FF0000000000000)
                                           : 0x3F800000);
        const int result = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, RTLOP_FDIV, result, one, div_src, 0);
        set_fp_result(ctx, insn_frD(insn), result, 0, 0, frB, 0,
                      0, is_rsqrte ? FPSCR_VXSQRT : 0,
                      true, true, false, true);
    } else {
        const int alias = rtl_alloc_alias_register(unit, type);
        int label_skip_set = 0;
        if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
            flush_fpr(ctx, insn_frD(insn), true);
            label_skip_set = rtl_alloc_label(unit);
        }
        if (is_rsqrte) {
            translate_frsqrte_lookup(ctx, frB, alias, label_skip_set);
        } else {
            translate_fres_lookup(ctx, frB, alias, label_skip_set);
        }
        const int result = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, result, 0, 0, alias);
        if (label_skip_set) {
            set_fpr_and_flush(ctx, insn_frD(insn), result, true);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_set);
        } else {
            set_fpr(ctx, insn_frD(insn), result);
        }
    }

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_fpr:  Translate a floating-point load or store
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_single: True if a single-precision (32-bit) operation, false if a
 *         double-precision (64-bit) operation.
 *     is_store: True if the instruction is a store instruction.
 *     is_indexed: True if the access is an indexed access (like lwx or stwx).
 *     update: True if register rA should be updated with the final EA.
 */
static void translate_load_store_fpr(
    GuestPPCContext *ctx, uint32_t insn, bool is_single, bool is_store,
    bool is_indexed, bool update)
{
    RTLUnit * const unit = ctx->unit;

    const RTLDataType type = is_single ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64;
    const RTLOpcode rtlop = (ctx->handle->host_little_endian
                             ? (is_store ? RTLOP_STORE_BR : RTLOP_LOAD_BR)
                             : (is_store ? RTLOP_STORE : RTLOP_LOAD));

    int disp, ea;
    const int host_address =
        gen_load_store_address(ctx, insn, is_indexed, update, &disp, &ea);

    if (is_store) {
        if (is_single && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_STFS)
         && get_fpr_scalar_type(ctx, insn_frD(insn)) != RTLTYPE_FLOAT32) {
            /* stfs performs a bitwise conversion from double precision
             * rather than an arithmetic conversion. */
            const int value =
                get_fpr_as_type(ctx, insn_frD(insn), RTLTYPE_FLOAT64);
            const int bits = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_BITCAST, bits, value, 0, 0);
            const int high_word64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SRLI, high_word64, bits, 0, 32);
            const int high_word = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ZCAST, high_word, high_word64, 0, 0);
            const int range_test = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SLLI, range_test, bits, 0, 1);
            const int label_normal = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, range_test, 0, label_normal);
            const int normal_limit =
                rtl_imm64(unit, (UINT64_C(0x3810000000000000) << 1) - 1);
            const int normal_test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU,
                         normal_test, range_test, normal_limit, 0);
            const int label_denormal = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, normal_test, 0, label_denormal);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_normal);
            const int high_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         high_bits, high_word, 0, 0xC0000000);
            const int low_bits64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_BFEXT, low_bits64, bits, 0, 29 | 30<<8);
            const int low_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ZCAST, low_bits, low_bits64, 0, 0);
            const int normal_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, normal_bits, high_bits, low_bits, 0);
            rtl_add_insn(unit, rtlop, 0, host_address, normal_bits, disp);
            const int label_out = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_denormal);
            const int exponent = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SRLI, exponent, range_test, 0, 53);
            const int frac64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SRLI, frac64, range_test, 0, 31);
            const int frac_temp1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ZCAST, frac_temp1, frac64, 0, 0);
            const int cst_896 = rtl_imm64(unit, 896);
            const int frac_temp2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         frac_temp2, frac_temp1, 0, 0x7FFFFF);
            const int shift = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SUB, shift, cst_896, exponent, 0);
            const int frac = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ORI, frac, frac_temp2, 0, 1<<22);
            const int sign = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, sign, high_word, 0, 0x80000000);
            const int shifted_frac = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SRL, shifted_frac, frac, shift, 0);
            const int denormal_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, denormal_bits, sign, shifted_frac, 0);
            rtl_add_insn(unit, rtlop, 0, host_address, denormal_bits, disp);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
        } else {
            const int value = get_fpr_as_type(ctx, insn_frD(insn), type);
            rtl_add_insn(unit, rtlop, 0, host_address, value, disp);
        }
    } else {
        const int value = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, rtlop, value, host_address, 0, disp);
        set_fpr(ctx, insn_frD(insn), value);
    }

    if (update) {
        if (is_indexed || insn_d(insn) != 0) {
            set_gpr(ctx, insn_rA(insn), ea);
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_gpr:  Translate an integer load or store instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL instruction to perform the operation (assuming a
 *         same-endian host).
 *     is_store: True if the instruction is a store instruction.
 *     is_indexed: True if the access is an indexed access (like lwx or stwx).
 *     update: True if register rA should be updated with the final EA.
 */
static void translate_load_store_gpr(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool is_store,
    bool is_indexed, bool update)
{
    RTLUnit * const unit = ctx->unit;

    if (ctx->handle->host_little_endian) {
        ASSERT(rtlop != RTLOP_LOAD_S16_BR);  // No such PowerPC instruction.
        switch (rtlop) {
            case RTLOP_LOAD:         rtlop = RTLOP_LOAD_BR;      break;
            case RTLOP_LOAD_U16:     rtlop = RTLOP_LOAD_U16_BR;  break;
            case RTLOP_LOAD_S16:     rtlop = RTLOP_LOAD_S16_BR;  break;
            case RTLOP_STORE:        rtlop = RTLOP_STORE_BR;     break;
            case RTLOP_STORE_I16:    rtlop = RTLOP_STORE_I16_BR; break;
            case RTLOP_LOAD_BR:      rtlop = RTLOP_LOAD;         break;
            case RTLOP_LOAD_U16_BR:  rtlop = RTLOP_LOAD_U16;     break;
            case RTLOP_STORE_BR:     rtlop = RTLOP_STORE;        break;
            case RTLOP_STORE_I16_BR: rtlop = RTLOP_STORE_I16;    break;
            default: break;
        }
    }

    int disp, ea;
    const int host_address =
        gen_load_store_address(ctx, insn, is_indexed, update, &disp, &ea);

    if (is_store) {
        const int value = get_gpr(ctx, insn_rS(insn));
        rtl_add_insn(unit, rtlop, 0, host_address, value, disp);
    } else {
        const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, rtlop, value, host_address, 0, disp);
        set_gpr(ctx, insn_rD(insn), value);
    }

    if (update) {
        if (is_indexed || insn_d(insn) != 0) {
            set_gpr(ctx, insn_rA(insn), ea);
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_multiple:  Translate a load or store multiple
 * instruction (lmw/stmw).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_store: True if the instruction is a store instruction (stmw).
 */
static void translate_load_store_multiple(
    GuestPPCContext *ctx, uint32_t insn, bool is_store)
{
    RTLUnit * const unit = ctx->unit;

    RTLOpcode rtlop = is_store ? RTLOP_STORE : RTLOP_LOAD;
    if (ctx->handle->host_little_endian) {
        rtlop = (is_store ? RTLOP_STORE_BR : RTLOP_LOAD_BR);
    }

    int host_address = get_ea_base(ctx, insn);
    int disp = insn_d(insn);
    if (disp >= (int)(32768 - 4*(31-insn_rD(insn)))) {
        /* Advancing the offset will wrap around to negative values!
         * Add it into the base address as a workaround. */
        const int base = host_address;
        host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADDI, host_address, base, 0, disp);
        disp = 0;
    }

    int rD = insn_rD(insn);
    int reg[4];

    /* Copy data in batches of 4 registers to minimize load stalls. */

    if (is_store) {
        for (int i = rD; i & 3; i++) {
            reg[i & 3] = get_gpr(ctx, i);
        }
        for (; rD & 3; rD++, disp += 4) {
            rtl_add_insn(unit, rtlop, 0, host_address, reg[rD & 3], disp);
        }
    } else {
        for (int i = rD; i & 3; i++, disp += 4) {
            reg[i & 3] = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, rtlop, reg[i & 3], host_address, 0, disp);
        }
        for (; rD & 3; rD++) {
            set_gpr(ctx, rD, reg[rD & 3]);
        }
    }

    for (; rD < 32; rD += 4) {
        if (is_store) {
            for (int i = 0; i < 4; i++) {
                reg[i] = get_gpr(ctx, rD+i);
            }
            for (int i = 0; i < 4; i++, disp += 4) {
                rtl_add_insn(unit, rtlop, 0, host_address, reg[i], disp);
            }
        } else {
            for (int i = 0; i < 4; i++, disp += 4) {
                reg[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, rtlop, reg[i], host_address, 0, disp);
            }
            for (int i = 0; i < 4; i++) {
                set_gpr(ctx, rD+i, reg[i]);
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_ps:  Translate a paired-single load or store
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_store: True if the instruction is a store instruction.
 *     is_indexed: True if the access is an indexed access (like lwx or stwx).
 *     update: True if register rA should be updated with the final EA.
 */
static void translate_load_store_ps(
    GuestPPCContext *ctx, uint32_t insn, bool is_store, bool is_indexed,
    bool update)
{
    RTLUnit * const unit = ctx->unit;

    const int frD_index = insn_frD(insn);
    const int gqr_index = is_indexed ? insn_I_22(insn) : insn_I_17(insn);
    const bool use_both = is_indexed ? !insn_W_21(insn) : !insn_W_16(insn);
    const RTLOpcode rtlop_32 = (ctx->handle->host_little_endian
                                ? (is_store ? RTLOP_STORE_BR : RTLOP_LOAD_BR)
                                : (is_store ? RTLOP_STORE : RTLOP_LOAD));
    const RTLOpcode rtlop_u16 =
        (ctx->handle->host_little_endian
         ? (is_store ? RTLOP_STORE_I16_BR : RTLOP_LOAD_U16_BR)
         : (is_store ? RTLOP_STORE_I16 : RTLOP_LOAD_U16));
    const RTLOpcode rtlop_s16 =
        (ctx->handle->host_little_endian
         ? (is_store ? RTLOP_STORE_I16_BR : RTLOP_LOAD_S16_BR)
         : (is_store ? RTLOP_STORE_I16 : RTLOP_LOAD_S16));

    const bool have_constant_gqr =
        (ctx->handle->guest_opt & BINREC_OPT_G_PPC_CONSTANT_GQRS)
        && ctx->handle->opt_state != NULL;
    uint32_t cgqr_value_raw;
    if (have_constant_gqr) {
        uint32_t *state_gqr_ptr =
            (uint32_t *)((uintptr_t)ctx->handle->opt_state
                         + ctx->handle->setup.state_offset_gqr);
        cgqr_value_raw = state_gqr_ptr[gqr_index];
    } else {
        cgqr_value_raw = 0;
    }
    const unsigned int cgqr_value =
        is_store ? cgqr_value_raw & 0xFFFF : cgqr_value_raw >> 16;
    const int cgqr_type = cgqr_value & 7;
    const int cgqr_scale = (int16_t)(cgqr_value << 2) >> 10;

    /* For store operations, if not using constant GQR mode, make sure the
     * alias is loaded here so it's not initialized on a conditional path. */
    if (is_store && !have_constant_gqr && !ctx->live.fpr[frD_index]) {
        (void) get_fpr(ctx, frD_index);
    }

    int disp, ea;
    const int host_address =
        gen_load_store_address(ctx, insn, is_indexed, update, &disp, &ea);

    /* Load and test the GQR value. */
    int gqr = 0, gqr_type = 0, label_int = 0, label_out = 0;
    if (!have_constant_gqr) {
        label_out = rtl_alloc_label(unit);
        gqr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD, gqr, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_gqr + gqr_index*4);
        gqr_type = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFEXT,
                     gqr_type, gqr, 0, (is_store ? 0 : 16) | 3<<8);
        const int int_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, int_test, gqr_type, 0, 4);
        label_int = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, int_test, 0, label_int);
    }

    /* Floating-point loads and stores. */
    if (!have_constant_gqr || !(cgqr_type & 4)) {
        if (is_store) {
            int ps = 0, ps0;
            if (use_both) {
                ps = get_fpr_as_type(ctx, frD_index, RTLTYPE_V2_FLOAT32);
                ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_VEXTRACT, ps0, ps, 0, 0);
            } else {
                ps0 = get_fpr_as_type(ctx, frD_index, RTLTYPE_FLOAT32);
            }
            if (!(ctx->handle->guest_opt
                  & BINREC_OPT_G_PPC_PS_STORE_DENORMALS)) {
                ps0 = flush_denormal(ctx, ps0);
            }
            rtl_add_insn(unit, rtlop_32, 0, host_address, ps0, disp);
            if (use_both) {
                int ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_VEXTRACT, ps1, ps, 0, 1);
                if (!(ctx->handle->guest_opt
                      & BINREC_OPT_G_PPC_PS_STORE_DENORMALS)) {
                    ps1 = flush_denormal(ctx, ps1);
                }
                rtl_add_insn(unit, rtlop_32, 0, host_address, ps1, disp+4);
            }
        } else {  // !is_store
            const int ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, rtlop_32, ps0, host_address, 0, disp);
            ctx->fpr_is_safe &= ~(1 << frD_index);
            const int ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            if (use_both) {
                rtl_add_insn(unit, rtlop_32, ps1, host_address, 0, disp+4);
                ctx->ps1_is_safe &= ~(1 << frD_index);
            } else {
                rtl_add_insn(unit, RTLOP_LOAD_IMM, ps1, 0, 0, 0x3F800000);
                ctx->ps1_is_safe |= 1 << frD_index;
            }
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, ps0, ps1, 0);
            set_fpr(ctx, frD_index, frD);
            if (!have_constant_gqr) {
                flush_fpr(ctx, frD_index, true);
            }
        }
        if (!have_constant_gqr) {
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
        }
    }

    /* Integer loads and stores.  Set up the quantization scale factor,
     * if necessary. */
    int gqr_scale;
    if (!have_constant_gqr) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_int);
        const int scale_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, scale_temp, gqr, 0, is_store ? 18 : 2);
        const int scale_temp2 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SRAI, scale_temp2, scale_temp, 0, 26);
        const int scale_exp = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, scale_exp, scale_temp2, 0, 23);
        int gqr_scale_bits;
        if (is_store) {
            gqr_scale_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ADDI,
                         gqr_scale_bits, scale_exp, 0, 0x3F800000);
        } else {
            const int one_bits = rtl_imm32(unit, 0x3F800000);
            gqr_scale_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SUB,
                         gqr_scale_bits, one_bits, scale_exp, 0);
        }
        gqr_scale = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_BITCAST, gqr_scale, gqr_scale_bits, 0, 0);
    } else if ((cgqr_type & 4) && cgqr_scale != 0) {
        gqr_scale = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, gqr_scale, 0, 0,
                     is_store ? 0x3F800000 + (cgqr_scale<<23)
                              : 0x3F800000 - (cgqr_scale<<23));
    } else {
        gqr_scale = 0;  // No scaling needed.
    }

    /* If storing, extract and quantize the values first. */
    int ps0_int = 0, ps1_int = 0;
    if ((!have_constant_gqr || (cgqr_type & 4)) && is_store) {
        /* Determine the saturation bounds. */
        int min_val, max_val;
        if (have_constant_gqr) {
            min_val = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_IMM, min_val, 0, 0,
                         cgqr_type & 1 ? (cgqr_type & 2 ? -0x8000 : 0)
                                       : (cgqr_type & 2 ? -0x80 : 0));
            max_val = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_IMM, max_val, 0, 0,
                         cgqr_type & 1 ? (cgqr_type & 2 ? 0x7FFF : 0xFFFF)
                                       : (cgqr_type & 2 ? 0x7F : 0xFF));
        } else {
            const int signed_test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, signed_test, gqr_type, 0, 2);
            const int sign_bit = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, sign_bit, signed_test, 0, 14);
            const int int16_test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, int16_test, gqr_type, 0, 1);
            const int shift_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, shift_temp, int16_test, 0, 1);
            const int minmax_shift = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, minmax_shift, shift_temp, 0, 3);
            const int min_base = rtl_imm32(unit, 0);
            const int max_base = rtl_imm32(unit, 0xFFFF);
            const int min_signed = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SUB, min_signed, min_base, sign_bit, 0);
            const int max_signed = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SUB, max_signed, max_base, sign_bit, 0);
            min_val = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SRA,
                         min_val, min_signed, minmax_shift, 0);
            max_val = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SRA,
                         max_val, max_signed, minmax_shift, 0);
        }
        /* Quantize the values.  For this case, we don't need to check
         * for SNaNs since both SNaNs and QNaNs convert to the same
         * output value.  (Note that we could in theory pass FLOAT64s
         * straight to FROUNDI in ps_quantize(), but typically psq_st
         * instructions should be located at the end of a sequence of
         * calculations which will leave the register in V2_FLOAT32 mode,
         * so it's probably not worth worrying about the FLOAT64 case.) */
        const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
        int ps = 0, ps0 = 0;
        if (ctx->live.fpr[frD_index]
         && unit->regs[ctx->live.fpr[frD_index]].type == RTLTYPE_V2_FLOAT32) {
            ps = ctx->live.fpr[frD_index];
            ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_VEXTRACT,
                         ps0, ctx->live.fpr[frD_index], 0, 0);
        } else {
            const int ps_64 =
                get_fpr_as_type(ctx, frD_index, RTLTYPE_V2_FLOAT64);
            if (use_both) {
                ps = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
                rtl_add_insn(unit, RTLOP_VFCVT, ps, ps_64, 0, 0);
                ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_VEXTRACT, ps0, ps, 0, 0);
            } else {
                const int ps0_64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
                rtl_add_insn(unit, RTLOP_VEXTRACT, ps0_64, ps_64, 0, 0);
                ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_FCVT, ps0, ps0_64, 0, 0);
            }
        }
        ps0_int = ps_quantize(ctx, ps0, gqr_scale, min_val, max_val);
        if (use_both) {
            const int ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_VEXTRACT, ps1, ps, 0, 1);
            ps1_int = ps_quantize(ctx, ps1, gqr_scale, min_val, max_val);
        }
        /* Clear any exceptions raised by the quantization. */
        rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
    }

    /* Check the access type. */
    int label_int16 = 0, label_sint8 = 0;
    if (!have_constant_gqr) {
        const int int16_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, int16_test, gqr_type, 0, 1);
        label_int16 = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, int16_test, 0, label_int16);
        if (!is_store) {
            const int sint8_test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, sint8_test, gqr_type, 0, 2);
            label_sint8 = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, sint8_test, 0, label_sint8);
        }
    }

    /* 8-bit store or unsigned 8-bit load. */
    if (!have_constant_gqr || cgqr_type == 4 || (is_store && cgqr_type == 6)) {
        if (is_store) {
            rtl_add_insn(unit, RTLOP_STORE_I8, 0, host_address, ps0_int, disp);
            if (use_both) {
                rtl_add_insn(unit, RTLOP_STORE_I8,
                             0, host_address, ps1_int, disp+1);
            }
        } else {
            const int int0 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_U8, int0, host_address, 0, disp);
            int int1 = 0;
            if (use_both) {
                int1 = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_LOAD_U8,
                             int1, host_address, 0, disp+1);
            }
            const int ps0 = ps_dequantize(ctx, int0, gqr_scale);
            int ps1;
            if (use_both) {
                ps1 = ps_dequantize(ctx, int1, gqr_scale);
            } else {
                ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_LOAD_IMM, ps1, 0, 0, 0x3F800000);
            }
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, ps0, ps1, 0);
            if (have_constant_gqr) {
                set_fpr(ctx, frD_index, frD);
                ctx->fpr_is_safe |= 1 << frD_index;
                ctx->ps1_is_safe |= 1 << frD_index;
            } else {
                set_fpr_and_flush(ctx, frD_index, frD, true);
            }
        }
        if (!have_constant_gqr) {
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
        }
    }

    /* Signed 8-bit load. */
    if (!is_store) {
        if (!have_constant_gqr) {
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_sint8);
        }
        if (!have_constant_gqr || cgqr_type == 6) {
            const int int0 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_S8, int0, host_address, 0, disp);
            int int1 = 0;
            if (use_both) {
                int1 = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_LOAD_S8,
                             int1, host_address, 0, disp+1);
            }
            const int ps0 = ps_dequantize(ctx, int0, gqr_scale);
            int ps1;
            if (use_both) {
                ps1 = ps_dequantize(ctx, int1, gqr_scale);
            } else {
                ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_LOAD_IMM, ps1, 0, 0, 0x3F800000);
            }
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, ps0, ps1, 0);
            if (have_constant_gqr) {
                set_fpr(ctx, frD_index, frD);
                ctx->fpr_is_safe |= 1 << frD_index;
                ctx->ps1_is_safe |= 1 << frD_index;
            } else {
                set_fpr_and_flush(ctx, frD_index, frD, true);
            }
        }
        if (!have_constant_gqr) {
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
        }
    }

    /* 16-bit signedness check (if loading data). */
    int label_sint16 = 0;
    if (!have_constant_gqr) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_int16);
        if (!is_store) {
            const int sint16_test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, sint16_test, gqr_type, 0, 2);
            label_sint16 = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, sint16_test, 0, label_sint16);
        }
    }

    /* 16-bit store or unsigned 16-bit load. */
    if (!have_constant_gqr || cgqr_type == 5 || (is_store && cgqr_type == 7)) {
        if (is_store) {
            rtl_add_insn(unit, rtlop_u16, 0, host_address, ps0_int, disp);
            if (use_both) {
                rtl_add_insn(unit, rtlop_u16,
                             0, host_address, ps1_int, disp+2);
            }
        } else {
            const int int0 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, rtlop_u16, int0, host_address, 0, disp);
            int int1 = 0;
            if (use_both) {
                int1 = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, rtlop_u16, int1, host_address, 0, disp+2);
            }
            const int ps0 = ps_dequantize(ctx, int0, gqr_scale);
            int ps1;
            if (use_both) {
                ps1 = ps_dequantize(ctx, int1, gqr_scale);
            } else {
                ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_LOAD_IMM, ps1, 0, 0, 0x3F800000);
            }
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, ps0, ps1, 0);
            if (have_constant_gqr) {
                set_fpr(ctx, frD_index, frD);
                ctx->fpr_is_safe |= 1 << frD_index;
                ctx->ps1_is_safe |= 1 << frD_index;
            } else {
                set_fpr_and_flush(ctx, frD_index, frD, true);
            }
        }
        if (!have_constant_gqr && !is_store) {
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);
        }
    }

    /* Signed 16-bit load. */
    if (!is_store) {
        if (!have_constant_gqr) {
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_sint16);
        }
        if (!have_constant_gqr || cgqr_type == 7) {
            const int int0 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, rtlop_s16, int0, host_address, 0, disp);
            int int1 = 0;
            if (use_both) {
                int1 = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, rtlop_s16, int1, host_address, 0, disp+2);
            }
            const int ps0 = ps_dequantize(ctx, int0, gqr_scale);
            int ps1;
            if (use_both) {
                ps1 = ps_dequantize(ctx, int1, gqr_scale);
            } else {
                ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_LOAD_IMM, ps1, 0, 0, 0x3F800000);
            }
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, ps0, ps1, 0);
            if (have_constant_gqr) {
                set_fpr(ctx, frD_index, frD);
                ctx->fpr_is_safe |= 1 << frD_index;
                ctx->ps1_is_safe |= 1 << frD_index;
            } else {
                set_fpr_and_flush(ctx, frD_index, frD, true);
            }
        }
    }

    if (!have_constant_gqr) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
    }
    if (update) {
        if (is_indexed || insn_d12(insn) != 0) {
            set_gpr(ctx, insn_rA(insn), ea);
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_string:  Translate a string load or store
 * instruction (lswi/lswx/stswi/stswx).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_store: True if the instruction is a store instruction (stswi/stswx).
 *     is_imm: True if the instruction has an immediate count (lswi/stswi).
 */
static void translate_load_store_string(
    GuestPPCContext *ctx, uint32_t insn, bool is_store, bool is_imm)
{
    RTLUnit * const unit = ctx->unit;

    /* We implement the string move instructions by loading or storing
     * directly to/from the PSB, so make sure it's up to date, and clear
     * the live register set so no future code tries to use stale values. */
    flush_live_regs(ctx, true);

    int base_address, host_address;
    if (insn_rA(insn)) {
        const int rA = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     rA, 0, 0, ctx->alias.gpr[insn_rA(insn)]);
        const int rA_zcast = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, rA_zcast, rA, 0, 0);
        base_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     base_address, ctx->membase_reg, rA_zcast, 0);
    } else {
        base_address = ctx->membase_reg;
    }
    if (is_imm) {
        host_address = base_address;
    } else {
        const int rB = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     rB, 0, 0, ctx->alias.gpr[insn_rB(insn)]);
        const int rB_zcast = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, rB_zcast, rB, 0, 0);
        host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     host_address, base_address, rB_zcast, 0);
    }

    const int psb_reg = ctx->psb_reg;
    const int gpr_base = ctx->handle->setup.state_offset_gpr;
    const int endian_flip = ctx->handle->host_little_endian ? 3 : 0;

    if (is_imm) {

        const int n = insn_NB(insn) ? insn_NB(insn) : 32;

        /* Unroll into units of 4 bytes, both to try and hide load latency
         * and since each GPR is 4 bytes wide anyway. */
        int rD = insn_rD(insn);
        for (int i = 0; i+4 <= n; i += 4, rD = (rD + 1) & 31) {
            int byte_reg[4];
            for (int byte = 0; byte < 4; byte++) {
                byte_reg[byte] = rtl_alloc_register(unit, RTLTYPE_INT32);
            }
            const int gpr_offset = gpr_base + 4*rD;
            if (is_store) {
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], psb_reg, 0,
                                 gpr_offset + (byte ^ endian_flip));
                }
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, host_address, byte_reg[byte], i + byte);
                }
            } else {
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], host_address, 0, i + byte);
                }
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, psb_reg, byte_reg[byte],
                                 gpr_offset + (byte ^ endian_flip));
                }
            }
        }

        if ((n & 3) != 0) {
            const int i = n & ~3;
            int byte_reg[4];
            for (int byte = 0; byte < (n & 3); byte++) {
                byte_reg[byte] = rtl_alloc_register(unit, RTLTYPE_INT32);
            }
            const int gpr_offset = gpr_base + 4*rD;
            if (is_store) {
                for (int byte = 0; byte < (n & 3); byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], psb_reg, 0,
                                 gpr_offset + (byte ^ endian_flip));
                }
                for (int byte = 0; byte < (n & 3); byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, host_address, byte_reg[byte], i + byte);
                }
            } else {
                for (int byte = 0; byte < (n & 3); byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], host_address, 0, i + byte);
                }
                const int zero = rtl_imm32(unit, 0);
                for (int byte = (n & 3); byte < 4; byte++) {
                    byte_reg[byte] = zero;
                }
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, psb_reg, byte_reg[byte],
                                 gpr_offset + (byte ^ endian_flip));
                }
            }
        }

    } else {  // !is_imm

        /* We don't even attempt to optimize this because it's way too
         * complicated already.  Hopefully nobody actually uses lswx/stswx
         * anymore. */

        const int xer = get_xer(ctx);
        const int xer_count = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, xer_count, xer, 0, 127);

        /* We need to zero out the unused bytes of the last register if
         * not loading a multiple of four bytes.  Do this by clearing
         * the entire register ahead of time for simplicity. */
        if (!is_store) {
            const int count_mod_4 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, count_mod_4, xer_count, 0, 3);
            const int start_label = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, count_mod_4, 0, start_label);

            const int last_gpr_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ADDI, last_gpr_temp, xer_count, 0,
                         4 * insn_rD(insn));
            const int last_gpr_offset =
                rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         last_gpr_offset, last_gpr_temp, 0, 31<<2);
            const int last_gpr_offset_zcast =
                rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ZCAST,
                         last_gpr_offset_zcast, last_gpr_offset, 0, 0);
            const int last_gpr_address =
                rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ADD,
                         last_gpr_address, psb_reg, last_gpr_offset_zcast, 0);
            rtl_add_insn(unit, RTLOP_STORE,
                         0, last_gpr_address, rtl_imm32(unit, 0), gpr_base);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, start_label);
        }

        /* We use ADDRESS type for the count and GPR offset aliases so we
         * can add them directly to the base addresses without an
         * intermediate ZCAST. */
        const int count = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, count, xer_count, 0, 0);
        const int init_i = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, init_i, 0, 0, 0);
        const int alias_i = rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, init_i, 0, alias_i);
        const int init_gpr_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     init_gpr_offset, 0, 0, 4 * insn_rD(insn));
        const int alias_gpr_offset =
            rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, init_gpr_offset, 0, alias_gpr_offset);

        const int loop_label = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, loop_label);
        const int i = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, i, 0, 0, alias_i);
        const int i_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTU, i_test, i, count, 0);
        const int end_label = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, i_test, 0, end_label);
        const int gpr_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     gpr_offset, 0, 0, alias_gpr_offset);

        const int mem_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD, mem_address, host_address, i, 0);

        int real_offset;
        if (endian_flip) {
            real_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_XORI,
                         real_offset, gpr_offset, 0, endian_flip);
        } else {
            real_offset = gpr_offset;
        }
        int gpr_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD, gpr_address, psb_reg, real_offset, 0);

        const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
        if (is_store) {
            rtl_add_insn(unit, RTLOP_LOAD_U8, value, gpr_address, 0, gpr_base);
            rtl_add_insn(unit, RTLOP_STORE_I8, 0, mem_address, value, 0);
        } else {
            rtl_add_insn(unit, RTLOP_LOAD_U8, value, mem_address, 0, 0);
            rtl_add_insn(unit, RTLOP_STORE_I8, 0, gpr_address, value, gpr_base);
        }

        int new_i = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADDI, new_i, i, 0, 1);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_i, 0, alias_i);
        const int gpr_offset_temp = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADDI, gpr_offset_temp, gpr_offset, 0, 1);
        int new_gpr_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ANDI,
                     new_gpr_offset, gpr_offset_temp, 0, 127);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, new_gpr_offset, 0, alias_gpr_offset);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, loop_label);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, end_label);

    }  // if (is_imm)
}

/*-----------------------------------------------------------------------*/

/**
 * translate_logic_crb:  Translate a logic instruction operating on CR bits.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-register instruction to perform the operation.
 *     invert_crbB: True if the value of crbB should be inverted.
 *     invert_result: True if the result should be inverted.
 */
static void translate_logic_crb(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool invert_crbB,
    bool invert_result)
{
    RTLUnit * const unit = ctx->unit;

    const int crbA = get_crb(ctx, insn_crbA(insn));

    int crbB = get_crb(ctx, insn_crbB(insn));
    if (invert_crbB) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, inverted, crbB, 0, 1);
        crbB = inverted;
    }

    int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, crbA, crbB, 0);
    if (invert_result) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, inverted, result, 0, 1);
        result = inverted;
    }
    set_crb(ctx, insn_crbD(insn), result);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_logic_imm:  Translate an integer register-immediate logic
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     shift_imm: True if the immediate value should be shifted left 16 bits.
 *     set_cr0: True if CR0 should be set according to the result.
 */
static void translate_logic_imm(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool shift_imm,
    bool set_cr0)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, insn_rS(insn));
    const uint32_t imm = shift_imm ? insn_UIMM(insn)<<16 : insn_UIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, 0, imm);
    set_gpr(ctx, insn_rA(insn), result);

    if (set_cr0) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_logic_reg:  Translate an integer register-register logic
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-register instruction to perform the operation.
 *     invert_rB: True if the value of rB should be inverted.
 *     invert_result: True if the result should be inverted.
 */
static void translate_logic_reg(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool invert_rB,
    bool invert_result)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, insn_rS(insn));

    int rB = get_gpr(ctx, insn_rB(insn));
    if (invert_rB) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, rB, 0, 0);
        rB = inverted;
    }

    int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, rB, 0);
    if (invert_result) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, result, 0, 0);
        result = inverted;
    }
    set_gpr(ctx, insn_rA(insn), result);

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_lwarx:  Translate a lwarx instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static void translate_lwarx(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn)
{
    const binrec_t *handle = ctx->handle;
    RTLUnit * const unit = ctx->unit;
    const int psb_reg = ctx->psb_reg;

    const int host_address = get_ea_indexed(ctx, insn, NULL);

    /*
     * If enabled, we optimize the common case of a loop containing an
     * lwarx followed by a stwcx (with no intervening branches or branch
     * targets) by saving the loaded value in an RTL register instead of
     * storing it back to the PSB, then using that value in the
     * accompanying stwcx. translation instead of reloading it from the
     * PSB.  Knowledge of the pairing also lets us omit the reserve_flag
     * check from stwcx., since the translator design ensures that code
     * flow cannot be interrupted between the two instructions.
     *
     * If the optimization is not enabled, paired_lwarx will always be
     * set to ~0, so it can never test equal to the instruction address.
     */
    const int value_be = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD, value_be, host_address, 0, 0);
    int value;
    if (handle->host_little_endian) {
        value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BSWAP, value, value_be, 0, 0);
    } else {
        value = value_be;
    }
    if (address == ctx->blocks[ctx->current_block].paired_lwarx) {
        ctx->paired_lwarx_data_be = value_be;
    } else {
        rtl_add_insn(unit, RTLOP_STORE_I8, 0, psb_reg, rtl_imm32(unit, 1),
                     handle->setup.state_offset_reserve_flag);
        rtl_add_insn(unit, RTLOP_STORE, 0, psb_reg, value_be,
                     handle->setup.state_offset_reserve_state);
    }
    set_gpr(ctx, insn_rD(insn), value);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_move_fpr:  Translate an fmr/fneg/fabs/fnabs or
 * ps_mr/ps_neg/ps_abs/ps_nabs instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     opcode: RTL opcode for the operation.
 *     is_paired: True if a paired-single operation, false if not.
 */
static void translate_move_fpr(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode opcode, bool is_paired)
{
    const RTLDataType type = is_paired ? RTLTYPE_V2_FLOAT32 : RTLTYPE_FLOAT64;
    const int frB = get_fpr_as_type(ctx, insn_frB(insn), type);
    if (opcode == RTLOP_MOVE) {
        set_fpr(ctx, insn_frD(insn), frB);
    } else {
        RTLUnit * const unit = ctx->unit;
        const int result = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, opcode, result, frB, 0, 0);
        set_fpr(ctx, insn_frD(insn), result);
    }
    if (ctx->fpr_is_safe & (1 << insn_frB(insn))) {
        ctx->fpr_is_safe |= 1 << insn_frD(insn);
    } else {
        ctx->fpr_is_safe &= ~(1 << insn_frD(insn));
    }
    if (is_paired) {
        if (ctx->ps1_is_safe & (1 << insn_frB(insn))) {
            ctx->ps1_is_safe |= 1 << insn_frD(insn);
        } else {
            ctx->ps1_is_safe &= ~(1 << insn_frD(insn));
        }
    }

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_move_spr:  Translate an mfspr or mtspr instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 *     to_spr: True for mtspr, false for mfspr.
 */
static void translate_move_spr(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn, bool to_spr)
{
    RTLUnit * const unit = ctx->unit;

    const int spr = insn_spr(insn);

    switch (spr) {
      case SPR_XER:
        if (to_spr) {
            set_xer(ctx, get_gpr(ctx, insn_rS(insn)), 0);
        } else {
            set_gpr(ctx, insn_rD(insn), get_xer(ctx));
        }
        break;

      case SPR_LR:
        if (to_spr) {
            set_lr(ctx, get_gpr(ctx, insn_rS(insn)));
        } else {
            set_gpr(ctx, insn_rD(insn), get_lr(ctx));
        }
        break;

      case SPR_CTR:
        if (to_spr) {
            set_ctr(ctx, get_gpr(ctx, insn_rS(insn)));
        } else {
            set_gpr(ctx, insn_rD(insn), get_ctr(ctx));
        }
        break;

      case SPR_TBL:
      case SPR_TBU:
        if (to_spr) {
            log_warning(ctx->handle, "0x%X: Invalid attempt to write TB%c",
                        address, spr==SPR_TBU ? 'U' : 'L');
            rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        } else {
            /* We have to flush rD since we set it differently depending on
             * whether a timebase handler function is present. */
            const int rD = insn_rD(insn);
            ctx->last_set.gpr[rD] = -1;
            ctx->live.gpr[rD] = 0;

            const int label_no_handler = rtl_alloc_label(unit);
            const int label_end = rtl_alloc_label(unit);
            const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_LOAD, func, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_timebase_handler);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, func, 0, label_no_handler);

            const int result64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_CALL, result64, func, ctx->psb_reg, 0);
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            if (spr == SPR_TBL) {
                rtl_add_insn(unit, RTLOP_ZCAST, result, result64, 0, 0);
            } else {
                const int shifted = rtl_alloc_register(unit, RTLTYPE_INT64);
                rtl_add_insn(unit, RTLOP_SRLI, shifted, result64, 0, 32);
                rtl_add_insn(unit, RTLOP_ZCAST, result, shifted, 0, 0);
            }
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, result, 0, ctx->alias.gpr[rD]);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_end);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_handler);
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, rtl_imm32(unit,0), 0, ctx->alias.gpr[rD]);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_end);
        }
        break;

      case SPR_UGQR(0):
      case SPR_UGQR(1):
      case SPR_UGQR(2):
      case SPR_UGQR(3):
      case SPR_UGQR(4):
      case SPR_UGQR(5):
      case SPR_UGQR(6):
      case SPR_UGQR(7):
        if (to_spr) {
            const int rS = get_gpr(ctx, insn_rS(insn));
            rtl_add_insn(unit, RTLOP_STORE, 0, ctx->psb_reg, rS,
                         ctx->handle->setup.state_offset_gqr + 4 * (spr & 7));
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_CONSTANT_GQRS) {
                return_from_unit(ctx, address, rtl_imm32(unit, address+4),
                                 true);
            }
        } else {
            const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD, value, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_gqr + 4 * (spr & 7));
            set_gpr(ctx, insn_rD(insn), value);
        }
        break;

      default:
        log_warning(ctx->handle, "0x%X: %s on invalid SPR %d", address,
                    to_spr ? "mtspr" : "mfspr/mftb", spr);
        rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        break;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_muldiv_reg:  Translate an integer register-register multiply
 * or divide instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     do_overflow: True to update XER[SO:OV] based on overflow state.
 */
static void translate_muldiv_reg(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool do_overflow)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, insn_rA(insn));
    const int rB = get_gpr(ctx, insn_rB(insn));

    /* For division, we might skip over the actual division operation, so
     * store the target register now.  We handle XER (when OE is set)
     * separately, since we have to set SO|OV anyway on the overflow path. */
    const bool is_divide = (rtlop == RTLOP_DIVU || rtlop == RTLOP_DIVS);
    if (is_divide) {
        const int rD = insn_rD(insn);
        ctx->last_set.gpr[rD] = -1;
        ctx->live.gpr[rD] = 0;
    }

    int div_skip_label = 0;
    int xer = 0;
    if (is_divide) {
        if (do_overflow) {
            xer = get_xer(ctx);
        }
        div_skip_label = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, rB, 0, div_skip_label);
        if (rtlop == RTLOP_DIVS) {
            int noskip_label = rtl_alloc_label(unit);
            const int rA_is_80000000 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI,
                         rA_is_80000000, rA, 0, UINT64_C(-0x80000000));
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, rA_is_80000000, 0, noskip_label);
            const int rB_is_FFFFFFFF = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI, rB_is_FFFFFFFF, rB, 0, -1);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, rB_is_FFFFFFFF, 0, div_skip_label);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, noskip_label);
        }
    }

    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rA, rB, 0);
    if (is_divide) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, result, 0, ctx->alias.gpr[insn_rD(insn)]);
    } else {
        set_gpr(ctx, insn_rD(insn), result);
    }

    if (do_overflow) {
        if (!xer) {
            xer = get_xer(ctx);
        }
        const int masked_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, ~XER_OV);
        if (rtlop == RTLOP_MUL) {
            /* mullwo's overflow check is for signed integers, so we can't
             * just check for the high word being nonzero. */
            const int result_hi = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_MULHS, result_hi, rA, rB, 0);
            const int lo_sign = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SRAI, lo_sign, result, 0, 31);
            const int overflow = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XOR, overflow, result_hi, lo_sign, 0);
            const int SO_OV = rtl_imm32(unit, XER_SO | XER_OV);
            const int bits_to_set = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SELECT,
                         bits_to_set, SO_OV, overflow, overflow);
            const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, new_xer, masked_xer, bits_to_set, 0);
            set_xer(ctx, new_xer, 0);
        } else {
            ASSERT(rtlop == RTLOP_DIVU || rtlop == RTLOP_DIVS);
            set_xer(ctx, masked_xer, 0);
        }
    }

    if (div_skip_label) {
        int div_continue_label = 0;
        if (do_overflow) {
            ctx->last_set.xer = -1;
            ctx->last_set.xer_so = -1;
            ctx->live.xer = 0;
            ctx->live.xer_so = 0;
            div_continue_label = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, div_continue_label);
        }

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, div_skip_label);

        if (do_overflow) {
            const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ORI, new_xer, xer, 0, XER_SO | XER_OV);
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_xer, 0, ctx->alias.xer);
            if (ctx->alias.xer_so) {
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, rtl_imm32(unit,1), 0, ctx->alias.xer_so);
            }
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, div_continue_label);
        }
    }

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_ps_arith:  Translate a two-operand paired-single arithmetic
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL opcode to perform the operation.
 *     frC_slot: frC slot to use as the multiplier for ps_muls[01], else -1.
 *     vxfoo_no_snan: FPSCR_VXFOO bitmask indicating which non-VXSNAN
 *         exception(s) can be raised by the instruction.
 */
static void translate_ps_arith(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, int frC_slot,
    uint32_t vxfoo_no_snan)
{
    RTLUnit * const unit = ctx->unit;

    const int src1_fpr = insn_frA(insn);
    const int src2_fpr = (rtlop==RTLOP_FMUL ? insn_frC(insn) : insn_frB(insn));
    const bool use_float32 =
        (get_fpr_scalar_type(ctx, src1_fpr) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, src2_fpr) == RTLTYPE_FLOAT32);
    const RTLDataType type =
        use_float32 ? RTLTYPE_V2_FLOAT32 : RTLTYPE_V2_FLOAT64;

    int src1 = get_fpr_as_type(ctx, src1_fpr, type);
    int src2 = get_fpr_as_type(ctx, src2_fpr, type);
    if (frC_slot >= 0) {
        const int multiplier = rtl_alloc_register(
            unit, use_float32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, multiplier, src2, 0, frC_slot);
        src2 = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, RTLOP_VBROADCAST, src2, multiplier, 0, 0);
    }
    if (rtlop == RTLOP_FMUL && type == RTLTYPE_V2_FLOAT64
     && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMULS)) {
        int src1_ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, src1_ps0, src1, 0, 0);
        int src2_ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, src2_ps0, src2, 0, 0);
        round_for_multiply(ctx, &src1_ps0, &src2_ps0);
        const int new_src1 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_VINSERT, new_src1, src1, src1_ps0, 0);
        const int new_src2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_VINSERT, new_src2, src2, src2_ps0, 0);
        src1 = new_src1;
        src2 = new_src2;
    }

    int result = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, rtlop, result, src1, src2, 0);
    if (!use_float32) {
        const int result64 = result;
        result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_VFCVT, result, result64, 0, 0);
    }

    check_fp_underflow(ctx, result, rtlop, src1, src2, 0, true, true,
                       use_float32);
    set_ps_result(ctx, insn_frD(insn), result, rtlop, src1, src2, 0,
                  use_float32, vxfoo_no_snan);

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_ps_fma:  Translate a paired-single multiply-add instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL opcode to perform the operation.
 *     frC_slot: frC slot to use as the multiplier for ps_madds[01], else -1.
 *     negate: True to negate non-NaN result values.
 */
static void translate_ps_fma(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, int frC_slot,
    bool negate)
{
    RTLUnit * const unit = ctx->unit;

    const bool use_float32 =
        (get_fpr_scalar_type(ctx, insn_frA(insn)) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, insn_frB(insn)) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, insn_frC(insn)) == RTLTYPE_FLOAT32);
    const RTLDataType type =
        use_float32 ? RTLTYPE_V2_FLOAT32 : RTLTYPE_V2_FLOAT64;
    const RTLDataType scalar_type = rtl_vector_element_type(type);
    int frA = get_fpr_as_type(ctx, insn_frA(insn), type);
    int frC = get_fpr_as_type(ctx, insn_frC(insn), type);
    int frB = get_fpr_as_type(ctx, insn_frB(insn), type);

    /* We can only use SIMD instructions if there are no special cases to
     * worry about; otherwise, the complexity of dealing with edge cases
     * in each of the paired-single slots becomes prohibitive. */
    if (!negate
     && (ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN)
     && (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMADDS)
     && (ctx->handle->guest_opt & (BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO
                                   | BINREC_OPT_G_PPC_NO_FPSCR_STATE))) {

        if (frC_slot >= 0) {
            const int multiplier = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, multiplier, frC, 0, frC_slot);
            frC = multiplier;
        }

        if (type == RTLTYPE_V2_FLOAT64
         && frC_slot != 1  // Slot 1 is already in single precision.
         && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMULS)) {
            int frA_ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps0, frA, 0, 0);
            int frC_ps0;
            if (frC_slot == 0) {
                frC_ps0 = frC;
            } else {
                frC_ps0 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
                rtl_add_insn(unit, RTLOP_VEXTRACT, frC_ps0, frC, 0, 0);
            }
            round_for_multiply(ctx, &frA_ps0, &frC_ps0);
            if (frC_slot == 0) {
                int frA_ps1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
                rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps1, frA, 0, 1);
                int frC_ps1 = frC;
                round_for_multiply(ctx, &frA_ps1, &frC_ps1);
                frA = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VBUILD2, frA, frA_ps0, frA_ps1, 0);
                frC = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VBUILD2, frC, frC_ps0, frC_ps1, 0);
            } else {
                const int new_frA =
                    rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VINSERT, new_frA, frA, frA_ps0, 0);
                const int new_frC =
                    rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VINSERT, new_frC, frC, frC_ps0, 0);
                frA = new_frA;
                frC = new_frC;
            }
        } else if (frC_slot >= 0) {
            const int multiplier = frC;
            frC = rtl_alloc_register(unit, type);
            rtl_add_insn(unit, RTLOP_VBROADCAST, frC, multiplier, 0, 0);
        }

        int result = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, rtlop, result, frA, frC, frB);
        if (!use_float32) {
            const int result64 = result;
            result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VFCVT, result, result64, 0, 0);
        }
        check_fp_underflow(ctx, result, rtlop, frA, frC, frB, true, true,
                           use_float32);
        set_fp_result(ctx, insn_frD(insn), result, 0, frA, frB, frC,
                      0, 0, true, false, true, true);

    } else {  // SIMD instructions not usable

        int frA_ps[2], frB_ps[2], frC_ps[2];
        frA_ps[0] = rtl_alloc_register(unit, scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps[0], frA, 0, 0);
        frA_ps[1] = rtl_alloc_register(unit, scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps[1], frA, 0, 1);
        frC_ps[0] = rtl_alloc_register(unit, scalar_type);
        if (frC_slot >= 0) {
            rtl_add_insn(unit, RTLOP_VEXTRACT, frC_ps[0], frC, 0, frC_slot);
            frC_ps[1] = frC_ps[0];
        } else {
            rtl_add_insn(unit, RTLOP_VEXTRACT, frC_ps[0], frC, 0, 0);
            frC_ps[1] = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frC_ps[1], frC, 0, 1);
        }
        frB_ps[0] = rtl_alloc_register(unit, scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frB_ps[0], frB, 0, 0);
        frB_ps[1] = rtl_alloc_register(unit, scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frB_ps[1], frB, 0, 1);

        if (type == RTLTYPE_V2_FLOAT64 && frC_slot != 1
            && !(ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMULS)) {
            round_for_multiply(ctx, &frA_ps[0], &frC_ps[0]);
            if (frC_slot == 0) {
                round_for_multiply(ctx, &frA_ps[1], &frC_ps[1]);
            }
        }

        int frD_ps[2], fi_fprf[2], invalid[2];
        int saved_frD = 0;
        if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
            flush_fpr(ctx, insn_frD(insn), false);
            saved_frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         saved_frD, 0, 0, ctx->alias.fpr[insn_frD(insn)]);
        }

        /* We have to process each slot sequentially so set_fp_result() sees
         * the set of exceptions for that slot (and not the other one). */
        for (int slot = 0; slot < 2; slot++) {
            int result = rtl_alloc_register(unit, scalar_type);
            rtl_add_insn(unit, rtlop,
                         result, frA_ps[slot], frC_ps[slot], frB_ps[slot]);

            invalid[slot] = 0;
            if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
                const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
                rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
                invalid[slot] = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_FTESTEXC,
                             invalid[slot], fpstate, 0, RTLFEXC_INVALID);
            }

            if (negate) {
                result = fma_negate(ctx, result);
            }
            if (!(ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN)) {
                result = fma_select_nan(
                    ctx, result, frA_ps[slot], frB_ps[slot], frC_ps[slot]);
            }
            if (!use_float32) {
                if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FAST_FMADDS) {
                    const int result64 = result;
                    result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                    rtl_add_insn(unit, RTLOP_FCVT, result, result64, 0, 0);
                } else {
                    result = round_fma_result_to_single(
                        ctx, result, rtlop,
                        frA_ps[slot], frB_ps[slot], frC_ps[slot]);
                }
            }
            check_fp_underflow(ctx, result, rtlop,
                               frA_ps[slot], frC_ps[slot], frB_ps[slot],
                               true, false, use_float32);

            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE) {
                frD_ps[slot] = result;
                fi_fprf[slot] = 0;
            } else {
                set_fp_result(ctx, insn_frD(insn), result, 0,
                              frA_ps[slot], frB_ps[slot], frC_ps[slot],
                              0, FPSCR_VXIMZ | FPSCR_VXISI,
                              true, false, true, true);
                frD_ps[slot] =
                    get_fpr_as_type(ctx, insn_frD(insn), RTLTYPE_FLOAT64);
                fi_fprf[slot] = get_fr_fi_fprf(ctx);
            }
        }

        if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE) {
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, frD_ps[0], frD_ps[1], 0);
            set_fpr(ctx, insn_frD(insn), frD);
            ctx->fpr_is_safe |= 1 << insn_frD(insn);
            ctx->ps1_is_safe |= 1 << insn_frD(insn);
        } else {
            const int fi_ps1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, fi_ps1, fi_fprf[1], 0, 0x20);
            const int final_fi_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, final_fi_fprf, fi_fprf[0], fi_ps1, 0);
            set_fr_fi_fprf_and_flush(ctx, final_fi_fprf);
            const int invalid_any = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR,
                         invalid_any, invalid[0], invalid[1], 0);
            const int label_do_result = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, invalid_any, 0, label_do_result);
            const int fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
            const int has_ve = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, has_ve, fpscr, 0, FPSCR_VE);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, has_ve, 0, label_do_result);
            set_fpr_and_flush(ctx, insn_frD(insn), saved_frD, false);
            const int label_skip_result = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_skip_result);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_do_result);
            const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_VBUILD2, frD, frD_ps[0], frD_ps[1], 0);
            set_fpr_and_flush(ctx, insn_frD(insn), frD, true);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_result);
       }
    }

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_ps_merge:  Translate a ps_merge* instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     frA_index: Paired-single slot index for frA (copied to frD[ps0]).
 *     frB_index: Paired-single slot index for frB (copied to frD[ps1]).
 */
static void translate_ps_merge(GuestPPCContext *ctx, uint32_t insn,
                               int frA_index, int frB_index)
{
    RTLUnit * const unit = ctx->unit;

    int frA;
    if (frA_index == 0) {
        frA = get_fpr_as_type(ctx, insn_frA(insn), RTLTYPE_FLOAT32);
    } else {
        frA = get_ps1(ctx, insn_frA(insn), RTLTYPE_FLOAT32);
    }

    int frB;
    if (frB_index == 0) {
        if (get_fpr_scalar_type(ctx, insn_frB(insn)) == RTLTYPE_FLOAT32) {
            frB = get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_FLOAT32);
        } else {
            /* When moving a double-precision value into the ps1 slot,
             * the value is always truncated rather than being rounded
             * based on FPSCR[RN]. */
            const int frB_64 =
                get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_FLOAT64);
            const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
            const int fpstate_trunc =
                rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FSETROUND,
                         fpstate_trunc, fpstate, 0, RTLFROUND_TRUNC);
            rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate_trunc, 0, 0);
            const bool snan_safe =
                (ctx->fpr_is_safe & (1 << insn_frB(insn))) != 0;
            frB = fcast_64to32(unit, frB_64, !snan_safe);
            rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
        }
    } else {
        frB = get_ps1(ctx, insn_frB(insn), RTLTYPE_FLOAT32);
    }

    const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
    rtl_add_insn(unit, RTLOP_VBUILD2, frD, frA, frB, 0);
    set_fpr(ctx, insn_frD(insn), frD);
    if ((frA_index ? ctx->ps1_is_safe : ctx->fpr_is_safe) & (1 << insn_frA(insn))) {
        ctx->fpr_is_safe |= 1 << insn_frD(insn);
    } else {
        ctx->fpr_is_safe &= ~(1 << insn_frD(insn));
    }
    if ((frB_index ? ctx->ps1_is_safe : ctx->fpr_is_safe) & (1 << insn_frB(insn))) {
        ctx->ps1_is_safe |= 1 << insn_frD(insn);
    } else {
        ctx->ps1_is_safe &= ~(1 << insn_frD(insn));
    }

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_ps_recip:  Translate a ps_res or ps_rsqrte instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_rsqrte: True if the instruction is ps_rsqrte, false if ps_res.
 */
static void translate_ps_recip(GuestPPCContext *ctx, uint32_t insn,
                               bool is_rsqrte)
{
    RTLUnit * const unit = ctx->unit;

    const int frB = get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_V2_FLOAT32);

    if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NATIVE_RECIPROCAL) {

        const uint32_t vxfoo_no_snan = is_rsqrte ? FPSCR_VXSQRT : 0;

        int div_src;
        if (is_rsqrte) {
            const int sqrt_frB = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_FSQRT, sqrt_frB, frB, 0, 0);
            div_src = sqrt_frB;
        } else {
            div_src = frB;
        }
        const int one = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, one, 0, 0, 0x3F800000);
        const int ones = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_VBROADCAST, ones, one, 0, 0);
        const int result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_FDIV, result, ones, div_src, 0);
        set_ps_result(ctx, insn_frD(insn), result,
                      is_rsqrte ? RTLOP_FSQRT : RTLOP_FDIV, ones, frB, one,
                      true, vxfoo_no_snan);

    } else {  // !NATIVE_RECIPROCAL

        int alias_skip_set = 0;
        if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
            flush_fpr(ctx, insn_frD(insn), true);
            alias_skip_set = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, rtl_imm32(unit,0), 0, alias_skip_set);
        }

        int frD_ps[2], fi_fprf = 0;
        for (int slot = 0; slot < 2; slot++) {
            const int alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT32);
            int label_skip_set = 0;
            if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
                label_skip_set = rtl_alloc_label(unit);
            }
            const int frB_ps = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_VEXTRACT, frB_ps, frB, 0, slot);
            if (is_rsqrte) {
                translate_frsqrte_lookup(ctx, frB_ps, alias, label_skip_set);
            } else {
                translate_fres_lookup(ctx, frB_ps, alias, label_skip_set);
            }
            if (label_skip_set) {
                const int label_do_set = rtl_alloc_label(unit);
                rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_do_set);
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_set);
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, rtl_imm32(unit,1), 0, alias_skip_set);
                rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_do_set);
            }
            if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
                if (slot == 0) {
                    fi_fprf = get_fr_fi_fprf(ctx);
                } else if (is_rsqrte) {
                    set_fr_fi_fprf_and_flush(ctx, fi_fprf);
                } else {
                    const int fi_fprf_ps1 = get_fr_fi_fprf(ctx);
                    const int fi_ps1 = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 fi_ps1, fi_fprf_ps1, 0, 0x20);
                    const int final_fi_fprf =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_OR,
                                 final_fi_fprf, fi_fprf, fi_ps1, 0);
                    set_fr_fi_fprf_and_flush(ctx, final_fi_fprf);
                }
            }
            frD_ps[slot] = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, frD_ps[slot], 0, 0, alias);
        }

        int label_skip_set = 0;
        if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)) {
            const int test_skip_set = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         test_skip_set, 0, 0, alias_skip_set);
            label_skip_set = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, test_skip_set, 0, label_skip_set);
        }
        const int frD = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_VBUILD2, frD, frD_ps[0], frD_ps[1], 0);
        if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE) {
            set_fpr(ctx, insn_frD(insn), frD);
            ctx->fpr_is_safe |= 1 << insn_frD(insn);
            ctx->ps1_is_safe |= 1 << insn_frD(insn);
        } else {
            set_fpr_and_flush(ctx, insn_frD(insn), frD, true);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_set);
        }

    }

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_ps_sel:  Translate a ps_sel instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 */
static void translate_ps_sel(GuestPPCContext *ctx, uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);

    const RTLDataType frA_scalar_type =
        get_fpr_scalar_type(ctx, insn_frA(insn));
    const RTLDataType frA_type = (frA_scalar_type == RTLTYPE_FLOAT32
                                  ? RTLTYPE_V2_FLOAT32 : RTLTYPE_V2_FLOAT64);
    const bool frBC_float32 =
        (get_fpr_scalar_type(ctx, insn_frB(insn)) == RTLTYPE_FLOAT32
         && get_fpr_scalar_type(ctx, insn_frC(insn)) == RTLTYPE_FLOAT32);
    const RTLDataType frBC_scalar_type =
        (frBC_float32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64);
    const RTLDataType frBC_type =
        (frBC_float32 ? RTLTYPE_V2_FLOAT32 : RTLTYPE_V2_FLOAT64);
    const int frA = get_fpr_as_type(ctx, insn_frA(insn), frA_type);
    const int frC = get_fpr_as_type(ctx, insn_frC(insn), frBC_type);
    const int frB = get_fpr_as_type(ctx, insn_frB(insn), frBC_type);
    const int zero = rtl_alloc_register(unit, frA_scalar_type);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, zero, 0, 0, 0);

    int frD_ps[2];
    for (int slot = 0; slot < 2; slot++) {
        const int frA_ps = rtl_alloc_register(unit, frA_scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frA_ps, frA, 0, slot);
        const int frC_ps = rtl_alloc_register(unit, frBC_scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frC_ps, frC, 0, slot);
        const int frB_ps = rtl_alloc_register(unit, frBC_scalar_type);
        rtl_add_insn(unit, RTLOP_VEXTRACT, frB_ps, frB, 0, slot);
        const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FCMP, test, frA_ps, zero, RTLFCMP_GE);
        frD_ps[slot] = rtl_alloc_register(unit, frBC_scalar_type);
        rtl_add_insn(unit, RTLOP_SELECT, frD_ps[slot], frC_ps, frB_ps, test);
    }

    const int frD = rtl_alloc_register(unit, frBC_type);
    rtl_add_insn(unit, RTLOP_VBUILD2, frD, frD_ps[0], frD_ps[1], 0);
    set_fpr(ctx, insn_frD(insn), frD);

    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_ps_sum:  Translate a ps_sum* instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     index: Slot (0 or 1) which receives frA[ps0] + frB[ps1].
 */
static void translate_ps_sum(GuestPPCContext *ctx, uint32_t insn, int index)
{
    RTLUnit * const unit = ctx->unit;

    const bool use_float32 =
        (get_fpr_scalar_type(ctx, insn_frA(insn)) == RTLTYPE_FLOAT32);
    const RTLDataType type = use_float32 ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64;

    const int frA = get_fpr_as_type(ctx, insn_frA(insn), type);
    const int frB = get_ps1(ctx, insn_frB(insn), type);
    int frC;
    if (index == 0) {
        frC = get_ps1(ctx, insn_frC(insn), RTLTYPE_FLOAT32);
    } else {
        frC = get_fpr_as_type(ctx, insn_frC(insn), RTLTYPE_FLOAT32);
    }

    int sum = rtl_alloc_register(unit, type);
    rtl_add_insn(unit, RTLOP_FADD, sum, frA, frB, 0);
    if (!use_float32) {
        const int sum64 = sum;
        sum = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
        rtl_add_insn(unit, RTLOP_FCVT, sum, sum64, 0, 0);
    }
    check_fp_underflow(ctx, sum, RTLOP_FADD, frA, frB, 0, true, false,
                       use_float32);

    /* If an invalid-operation exception occurred and native NaNs are
     * not enabled, we have to process the result as a scalar in order
     * to get the generated NaN (if any) in the proper place. */
    if (!(ctx->handle->guest_opt & BINREC_OPT_G_PPC_NO_FPSCR_STATE)
     && !(ctx->handle->common_opt & BINREC_OPT_NATIVE_IEEE_NAN)) {
        const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
        const int invalid = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_FTESTEXC,
                     invalid, fpstate, 0, RTLFEXC_INVALID);
        const int label_do_result = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, invalid, 0, label_do_result);

        set_fp_result(ctx, insn_frD(insn), sum, index, frA, frB, 0,
                      0, FPSCR_VXISI, true, false, true, true);
        const int fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
        const int has_ve = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, has_ve, fpscr, 0, FPSCR_VE);
        const int label_skip_result = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, has_ve, 0, label_skip_result);

        /* Suppress exceptions from the cast operation. */
        const int fcast_fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
        rtl_add_insn(unit, RTLOP_FGETSTATE, fcast_fpstate, 0, 0, 0);
        const int frD_ps = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     frD_ps, 0, 0, ctx->alias.fpr[insn_frD(insn)]);
        const int sum_64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
        rtl_add_insn(unit, RTLOP_VEXTRACT, sum_64, frD_ps, 0, 0);
        const bool frC_snan_safe =
            (ctx->handle->guest_opt & BINREC_OPT_G_PPC_ASSUME_NO_SNAN) != 0;
        const int frC_64 = fcast_32to64(unit, frC, !frC_snan_safe);
        rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fcast_fpstate, 0, 0);
        const int nan_result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        rtl_add_insn(unit, RTLOP_VBUILD2, nan_result,
                     index==0 ? sum_64 : frC_64,
                     index==0 ? frC_64 : sum_64, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, nan_result, 0, ctx->alias.fpr[insn_frD(insn)]);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_skip_result);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_do_result);
        const int result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_VBUILD2,
                     result, index==0 ? sum : frC, index==0 ? frC : sum, 0);
        /* Don't pass VXISI here because it'll confuse the default NaN
         * generator.  This is only reached if VX was not raised, so we
         * don't need to specify any exceptions anyway. */
        set_fp_result(ctx, insn_frD(insn), result, index, frA, frB, 0,
                      0, 0, false, false, true, false);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_skip_result);
    } else {
        /* If we don't have to worry about default NaNs, we can just set
         * the result directly. */
        const int result = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
        rtl_add_insn(unit, RTLOP_VBUILD2,
                     result, index==0 ? sum : frC, index==0 ? frC : sum, 0);
        set_fp_result(ctx, insn_frD(insn), result, index, frA, frB, 0,
                      0, FPSCR_VXISI, true, false, true, false);
    }

    if (insn_Rc(insn)) {
        update_cr1(ctx);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_rotate_mask:  Translate a rlwinm, rlwnm, or rlwimi instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_imm: True if the shift count is an immediate value, false if rB.
 *     insert: True for a rlwimi instruction.
 */
static void translate_rotate_mask(
    GuestPPCContext *ctx, uint32_t insn, bool is_imm, bool insert)
{
    RTLUnit * const unit = ctx->unit;

    const int SH = insn_SH(insn);
    const int MB = insn_MB(insn);
    const int ME = insn_ME(insn);

    const int rS = get_gpr(ctx, insn_rS(insn));
    int result;

    if (MB == ((ME + 1) & 31)) {  // rotlw/rotlwi
        if (is_imm) {
            if (SH == 0) {
                result = rS;
            } else {
                result = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI, result, rS, 0, 32-SH);
            }
        } else {
            const int rB = get_gpr(ctx, insn_rB(insn));
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ROL, result, rS, rB, 0);
        }

    } else if (is_imm && !insert && MB == 0 && ME == 31-SH) {  // slwi
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, result, rS, 0, SH);

    } else if (is_imm && !insert && MB == 32-SH && ME == 31) {  // srwi
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SRLI, result, rS, 0, 32-SH);

    } else if (insert) {
        ASSERT(is_imm);
        const int rA = get_gpr(ctx, insn_rA(insn));
        if (MB <= ME) {
            const int start = 31 - ME;
            const int count = ME - MB + 1;
            const int base = rA;
            int value;
            if (SH == start) {
                value = rS;
            } else {
                value = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI,
                             value, rS, 0, ((32-SH) + start) & 31);
            }
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFINS,
                         result, base, value, start | count<<8);
        } else {
            const int start = 32 - MB;
            const int count = MB - ME - 1;
            ASSERT(count > 0);  // Or else it would be rotlwi.
            int rS_rotated;
            if (SH == 0) {
                rS_rotated = rS;
            } else {
                rS_rotated = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI, rS_rotated, rS, 0, 32-SH);
            }
            const uint32_t mask = ((1 << count) - 1) << start;
            const int rS_masked = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, rS_masked, rS_rotated, 0, ~mask);
            const int rA_masked = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, rA_masked, rA, 0, mask);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, rS_masked, rA_masked, 0);
        }

    } else {
        /* MASK function for little-endian bit numbering.  Assumes mb >= me. */
        #define MASK_LE(mb, me) \
            ((uint32_t)((UINT64_C(1) << ((mb)-(me)+1)) - 1) << (me))
        const int mb = 31 - MB;
        const int me = 31 - ME;
        const uint32_t mask =
            (mb < me ? ~MASK_LE((me-1) & 31, (mb+1) & 31) : MASK_LE(mb, me));
        #undef MASK_LE

        int rotated;
        if (is_imm) {
            if (SH == 0) {
                rotated = rS;
            } else {
                rotated = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI, rotated, rS, 0, 32-SH);
            }
        } else {
            const int rB = get_gpr(ctx, insn_rB(insn));
            rotated = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ROL, rotated, rS, rB, 0);
        }
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, result, rotated, 0, mask);
    }

    set_gpr(ctx, insn_rA(insn), result);

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_shift:  Translate a bit-shift instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL instruction to perform the operation.
 *     is_imm: True if the shift count is an immediate value, false if rB.
 *     is_sra: True if the shift is an arithmetic right shift (sets XER[CA]).
 */
static void translate_shift(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool is_imm,
    bool is_sra)
{
    RTLUnit * const unit = ctx->unit;

    int rS = get_gpr(ctx, insn_rS(insn));
    int count, result;
    if (is_imm) {
        count = 0;  // Not used, but avoid a "may be uninitialized" warning.
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, rtlop, result, rS, 0, insn_SH(insn));
    } else {
        const int rB = get_gpr(ctx, insn_rB(insn));
        count = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, count, rB, 0, 63);
        const int rS_64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        if (is_sra) {
            rtl_add_insn(unit, RTLOP_SCAST, rS_64, rS, 0, 0);
        } else {
            rtl_add_insn(unit, RTLOP_ZCAST, rS_64, rS, 0, 0);
        }
        rS = rS_64;
        const int result_64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, rtlop, result_64, rS, count, 0);
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, result, result_64, 0, 0);
    }
    set_gpr(ctx, insn_rA(insn), result);

    if (is_sra) {
        int test;
        if (is_imm) {
            const uint32_t mask = (1 << insn_SH(insn)) - 1;
            test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, test, rS, 0, mask);
        } else {
            const int one = rtl_imm64(unit, 1);
            const int shifted_one = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SLL, shifted_one, one, count, 0);
            const int mask = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_ADDI, mask, shifted_one, 0, -1);
            test = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_AND, test, rS, mask, 0);
        }
        const int has_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTUI, has_bits, test, 0, 0);
        const int is_neg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTSI, is_neg, rS, 0, 0);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_AND, ca, has_bits, is_neg, 0);
        const int xer = get_xer(ctx);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer, -1);
    }

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_stwcx:  Translate a stwcx. instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static void translate_stwcx(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn)
{
    const binrec_t *handle = ctx->handle;
    RTLUnit * const unit = ctx->unit;
    const int psb_reg = ctx->psb_reg;

    const bool is_paired =
        (address == ctx->blocks[ctx->current_block].paired_stwcx);

    const int skip_label = rtl_alloc_label(unit);

    int flag;
    if (is_paired) {
        /* If this instruction is part of an optimized pair, we don't need
         * to test reserve_flag.  We still clear it, though, just in case
         * it was set on entry to the block. */
        flag = 0;
    } else {
        flag = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_U8, flag, psb_reg, 0,
                     handle->setup.state_offset_reserve_flag);
    }
    const int zero = rtl_imm32(unit, 0);
    const int so = get_xer_so(ctx);
    if (ctx->use_split_fields) {
        set_crf(ctx, 0, zero, zero, zero, so);
        /* Flush CR0.eq because of the conditional branches. */
        ctx->live.crb[2] = 0;
        ctx->last_set.crb[2] = -1;
        ctx->crb_dirty |= 1 << 2;
    } else {
        /* Optimize non-split-field set_crf() since we know the high
         * three bits are zero. */
        const int old_cr = get_cr(ctx);
        const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_cr, old_cr, so, 28 | 4<<8);
        set_cr(ctx, new_cr);
        /* Flush CR, as above. */
        ctx->live.cr = 0;
        ctx->last_set.cr = -1;
    }
    if (!is_paired) {
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, flag, 0, skip_label);
    }
    rtl_add_insn(unit, RTLOP_STORE_I8, 0, psb_reg, zero,
                 handle->setup.state_offset_reserve_flag);

    int old_value;
    if (is_paired) {
        old_value = ctx->paired_lwarx_data_be;
    } else {
        old_value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD, old_value, psb_reg, 0,
                     handle->setup.state_offset_reserve_state);
    }

    int new_value = get_gpr(ctx, insn_rS(insn));
    if (handle->host_little_endian) {
        const int temp = new_value;
        new_value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BSWAP, new_value, temp, 0, 0);
    }
    const int host_address = get_ea_indexed(ctx, insn, NULL);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_CMPXCHG,
                 result, host_address, old_value, new_value);
    const int success = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQ, success, result, old_value, 0);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, success, 0, skip_label);

    if (ctx->use_split_fields) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, rtl_imm32(unit, 1), 0, ctx->alias.crb[2]);
    } else {
        const int old_cr = get_cr(ctx);
        const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ORI, new_cr, old_cr, 0, 1<<29);
        set_cr(ctx, new_cr);
        ctx->live.cr = 0;
        ctx->last_set.cr = -1;
    }

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);

    /* If split fields are in use and the post-instruction callback is
     * active, flush the store success bit back to the CR word in the PSB,
     * so the callback knows whether the store succeeded.  This deviates
     * from the ideal of not changing behavior in the presence of pre/post
     * instruction callbacks, but it is necessary when the callbacks are
     * used to validate the behavior of generated code against a hardware
     * implementation or interpreter so the validator knows whether to
     * simulate the store (since stwcx. is not repeatable). */
    if (ctx->use_split_fields && ctx->handle->post_insn_callback) {
        const int cr0_eq = get_crb(ctx, 2);
        const int old_cr = get_cr(ctx);
        const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_cr, old_cr, cr0_eq, 29 | 1<<8);
        set_cr(ctx, new_cr);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_trap:  Translate a tw or twi instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 *     rB: RTL register containing second comparison value (rB or immediate).
 */
static void translate_trap(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn, int rB)
{
    RTLUnit * const unit = ctx->unit;

    const int TO = insn_TO(insn);
    if (!TO) {
        return;  // Effectively a NOP.
    }

    const int label = rtl_alloc_label(unit);
    const int rA = get_gpr(ctx, insn_rA(insn));
    int result;
    RTLOpcode skip_op;

    switch (TO) {
      case TO_GTU:                              // 0x01
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_LTU:                              // 0x02
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_LTU | TO_GTU:                     // 0x03
      case TO_GTS | TO_LTU | TO_GTU:            // 0x0B
      case TO_LTS | TO_LTU | TO_GTU:            // 0x13
      case TO_LTS | TO_GTS:                     // 0x18
      case TO_LTS | TO_GTS | TO_GTU:            // 0x19
      case TO_LTS | TO_GTS | TO_LTU:            // 0x1A
      case TO_LTS | TO_GTS | TO_LTU | TO_GTU:   // 0x1B
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQ, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_NZ;
        break;

      case TO_EQ:                               // 0x04
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQ, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_GTU | TO_EQ:                      // 0x05
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_NZ;
        break;

      case TO_LTU | TO_EQ:                      // 0x06
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_NZ;
        break;

      case TO_GTS:                              // 0x08
      case TO_GTS | TO_GTU:                     // 0x09
      case TO_GTS | TO_LTU:                     // 0x0A
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        } else if (TO & TO_LTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        }
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_GTS | TO_EQ:                      // 0x0C
      case TO_GTS | TO_EQ | TO_GTU:             // 0x0D
      case TO_GTS | TO_EQ | TO_LTU:             // 0x0E
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else if (TO & TO_LTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else {
            skip_op = RTLOP_GOTO_IF_NZ;
        }
        break;

      case TO_LTS:                              // 0x10
      case TO_LTS | TO_GTU:                     // 0x11
      case TO_LTS | TO_LTU:                     // 0x12
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        } else if (TO & TO_LTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        }
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_LTS | TO_EQ:                      // 0x14
      case TO_LTS | TO_EQ | TO_GTU:             // 0x15
      case TO_LTS | TO_EQ | TO_LTU:             // 0x16
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else if (TO & TO_LTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else {
            skip_op = RTLOP_GOTO_IF_NZ;
        }
        break;

      default:
        ASSERT((TO & (TO_LTS|TO_GTS|TO_EQ)) == (TO_LTS|TO_GTS|TO_EQ)
            || (TO & (TO_LTU|TO_GTU|TO_EQ)) == (TO_LTU|TO_GTU|TO_EQ));
        result = 0;
        skip_op = RTLOP_NOP;
        break;
    }

    if (result) {
        rtl_add_insn(unit, skip_op, 0, result, 0, label);
    }

    flush_live_regs(ctx, false);
    guest_ppc_flush_cr(ctx, false);
    guest_ppc_flush_fpscr(ctx);
    set_nia_imm(ctx, address);
    post_insn_callback(ctx, address);
    const int trap_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD, trap_handler, ctx->psb_reg, 0,
                 ctx->handle->setup.state_offset_trap_handler);
    rtl_add_insn(unit, RTLOP_CALL, 0, trap_handler, ctx->psb_reg, 0);
    rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);

    if (result) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_unimplemented_insn:  Handle translation for an instruction
 * not supported by the translator.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static void translate_unimplemented_insn(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn)
{
    log_warning(ctx->handle, "Unsupported instruction %08X at address 0x%X,"
                " treating as invalid", insn, address);
    rtl_add_insn(ctx->unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_x1F:  Translate the given opcode-0x1F instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static inline void translate_x1F(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    switch ((PPCExtendedOpcode1F)insn_XO_10(insn)) {

      /* XO_5 = 0x00 */
      case XO_CMP:
        translate_compare(ctx, insn, false, true);
        return;
      case XO_CMPL:
        translate_compare(ctx, insn, false, false);
        return;
      case XO_MCRXR: {
        const int xer = get_xer(ctx);
        int crb[4];
        for (int bit = 0; bit < 4; bit++) {
            crb[bit] = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT,
                         crb[bit], xer, 0, (31-bit) | (1<<8));
        }
        set_crf(ctx, insn_crfD(insn), crb[0], crb[1], crb[2], crb[3]);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, new_xer, xer, 0, 0x0FFFFFFF);
        set_xer(ctx, new_xer, rtl_imm32(unit,0));
        return;
      }  // case XO_MCRXR

      /* XO_5 = 0x04 */
      case XO_TW:
        translate_trap(ctx, address, insn, get_gpr(ctx, insn_rB(insn)));
        return;

      /* XO_5 = 0x08 */
      case XO_SUBFC:
      case XO_SUBFCO:
        translate_addsub_reg(ctx, insn, 1, 1, true, true);
        return;
      case XO_SUBF: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, rB, rA, 0);
        set_gpr(ctx, insn_rD(insn), result);
        if (insn_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_SUBF
      case XO_SUBFO:
        translate_addsub_reg(ctx, insn, 1, 1, true, false);
        return;
      case XO_NEG: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NEG, result, rA, 0, 0);
        set_gpr(ctx, insn_rD(insn), result);
        if (insn_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_NEG
      case XO_NEGO:
        translate_addsub_reg(ctx, insn, 0, 1, true, false);
        return;
      case XO_SUBFE:
      case XO_SUBFEO:
        translate_addsub_reg(ctx, insn, 1, -1, true, true);
        return;
      case XO_SUBFZE:
      case XO_SUBFZEO:
        translate_addsub_reg(ctx, insn, 0, -1, true, true);
        return;
      case XO_SUBFME:
      case XO_SUBFMEO:
        translate_addsub_reg(ctx, insn, -1, -1, true, true);
        return;

      /* XO_5 = 0x0A */
      case XO_ADDC:
      case XO_ADDCO:
        translate_addsub_reg(ctx, insn, 1, 0, false, true);
        return;
      case XO_ADDE:
      case XO_ADDEO:
        translate_addsub_reg(ctx, insn, 1, -1, false, true);
        return;
      case XO_ADDZE:
      case XO_ADDZEO:
        translate_addsub_reg(ctx, insn, 0, -1, false, true);
        return;
      case XO_ADDME:
      case XO_ADDMEO:
        translate_addsub_reg(ctx, insn, -1, -1, false, true);
        return;
      case XO_ADD: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, rA, rB, 0);
        set_gpr(ctx, insn_rD(insn), result);
        if (insn_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_ADD
      case XO_ADDO:
        translate_addsub_reg(ctx, insn, 1, 0, false, false);
        return;

      /* XO_5 = 0x0B */
      case XO_MULHWU:
      case XO_UNDOCUMENTED_MULHWUO:  // OE is ignored.
        translate_muldiv_reg(ctx, insn, RTLOP_MULHU, false);
        return;
      case XO_MULHW:
      case XO_UNDOCUMENTED_MULHWO:  // OE is ignored.
        translate_muldiv_reg(ctx, insn, RTLOP_MULHS, false);
        return;
      case XO_MULLW:
        translate_muldiv_reg(ctx, insn, RTLOP_MUL, false);
        return;
      case XO_MULLWO:
        translate_muldiv_reg(ctx, insn, RTLOP_MUL, true);
        return;
      case XO_DIVWU:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVU, false);
        return;
      case XO_DIVWUO:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVU, true);
        return;
      case XO_DIVW:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVS, false);
        return;
      case XO_DIVWO:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVS, true);
        return;

      /* XO_5 = 0x10 */
      case XO_MTCRF: {
        const int rS = get_gpr(ctx, insn_rS(insn));
        if (insn_CRM(insn) == 0xFF) {
            set_cr(ctx, rS);
        }
        if (ctx->use_split_fields) {
            for (int i = 0; i < 8; i++) {
                if (insn_CRM(insn) & (0x80 >> i)) {
                    int crb[4];
                    for (int j = 0; j < 4; j++) {
                        const int bit = i*4+j;
                        if (ctx->alias.crb[bit]) {
                            crb[j] = rtl_alloc_register(unit, RTLTYPE_INT32);
                            rtl_add_insn(unit, RTLOP_BFEXT,
                                         crb[j], rS, 0, (31-bit) | (1<<8));
                        } else {
                            crb[j] = 0;
                        }
                    }
                    for (int j = 0; j < 4; j++) {
                        if (crb[j]) {
                            const int bit = i*4+j;
                            set_crb(ctx, bit, crb[j]);
                        }
                    }
                }
            }
        } else {  // !ctx->use_split_fields
            if (insn_CRM(insn) != 0xFF) {
                const uint32_t mask = crm_to_mask(insn_CRM(insn));
                const int old_cr = get_cr(ctx);
                const int masked_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, masked_cr, old_cr, 0, ~mask);
                const int masked_rS = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, masked_rS, rS, 0, mask);
                const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_OR, new_cr, masked_cr, masked_rS, 0);
                set_cr(ctx, new_cr);
            }
        }
        return;
      }  // case XO_MTCRF

      /* XO_5 = 0x12 */
      case XO_MTMSR:
      case XO_MTSR:
      case XO_MTSRIN:
      case XO_TLBIE:
        translate_unimplemented_insn(ctx, address, insn);
        return;

      /* XO_5 = 0x13 */
      case XO_MFCR: {
        /* Optimize an mfcr+rlwinm pair which extracts a single bit from CR.
         * We can only trivially skip the mfcr if the rlwinm reads and
         * writes the same register. */
        const bool can_skip_insn = (!ctx->handle->pre_insn_callback
                                    && !ctx->handle->post_insn_callback);
        const uint32_t next_insn =
            can_skip_insn ? guest_ppc_get_insn_at(ctx, block, address+4) : 0;
        const uint32_t extract_bit_insn =
            OPCD_RLWINM<<26 | insn_rD(insn)<<21 | 31<<6 | 31<<1;
        const uint32_t extract_bit_same_reg_insn =
            extract_bit_insn | insn_rD(insn)<<16;
        if ((next_insn & 0xFFFF07FE) != extract_bit_same_reg_insn) {
            guest_ppc_flush_cr(ctx, true);
            set_gpr(ctx, insn_rD(insn), get_cr(ctx));
        }
        if ((next_insn & 0xFFE007FE) == extract_bit_insn) {
            const int bit_index = (insn_SH(next_insn) - 1) & 31;
            set_gpr(ctx, insn_rA(next_insn), get_crb(ctx, bit_index));
            ctx->skip_next_insn = true;
        }
        return;
      }  // case XO_MFCR
      case XO_MFTB:
      case XO_MFSPR:
        translate_move_spr(ctx, address, insn, false);
        return;
      case XO_MTSPR:
        translate_move_spr(ctx, address, insn, true);
        return;
      case XO_MFMSR:
      case XO_MFSR:
      case XO_MFSRIN:
        translate_unimplemented_insn(ctx, address, insn);
        return;

      /* XO_5 = 0x14 */
      case XO_LWARX:
        translate_lwarx(ctx, address, insn);
        return;

      /* XO_5 = 0x15 */
      case XO_LSWX:
        translate_load_store_string(ctx, insn, false, false);
        return;
      case XO_LSWI:
        translate_load_store_string(ctx, insn, false, true);
        return;
      case XO_STSWX:
        translate_load_store_string(ctx, insn, true, false);
        return;
      case XO_STSWI:
        translate_load_store_string(ctx, insn, true, true);
        return;

      /* XO_5 = 0x16 */
      case XO_DCBST:
      case XO_DCBF:
      case XO_DCBTST:
      case XO_DCBT:
      case XO_DCBI:
        // FIXME: We currently act as if there is no data cache.
        return;
      case XO_STWCX_:
        translate_stwcx(ctx, address, insn);
        return;
      case XO_ECIWX:
      case XO_ECOWX:
      case XO_TLBSYNC:
        translate_unimplemented_insn(ctx, address, insn);
        return;
      case XO_LWBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_BR, false, true, false);
        return;
      case XO_SYNC:
      case XO_EIEIO:
        // FIXME: We currently act as if all loads and stores are sequential.
        return;
      case XO_STWBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_BR, true, true, false);
        return;
      case XO_LHBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16_BR,
                                 false, true, false);
        return;
      case XO_STHBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16_BR,
                                 true, true, false);
        return;
      case XO_ICBI:
        /* icbi implies that already-translated code may have changed, so
         * unconditionally return from this unit.  We currently don't
         * bother checking the invalidation address. */
        return_from_unit(ctx, address, rtl_imm32(unit, address+4), true);
        return;
      case XO_DCBZ:
        translate_dcbz(ctx, insn);
        return;

      /* XO_5 = 0x17 */
      case XO_LWZX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, true, false);
        return;
      case XO_LWZUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, true, true);
        return;
      case XO_LBZX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8, false, true, false);
        return;
      case XO_LBZUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8, false, true, true);
        return;
      case XO_STWX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, true, false);
        return;
      case XO_STWUX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, true, true);
        return;
      case XO_STBX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8, true, true, false);
        return;
      case XO_STBUX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8, true, true, true);
        return;
      case XO_LHZX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16, false, true, false);
        return;
      case XO_LHZUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16, false, true, true);
        return;
      case XO_LHAX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16, false, true, false);
        return;
      case XO_LHAUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16, false, true, true);
        return;
      case XO_STHX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16, true, true, false);
        return;
      case XO_STHUX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16, true, true, true);
        return;
      case XO_LFSX:
        translate_load_store_fpr(ctx, insn, true, false, true, false);
        return;
      case XO_LFSUX:
        translate_load_store_fpr(ctx, insn, true, false, true, true);
        return;
      case XO_LFDX:
        translate_load_store_fpr(ctx, insn, false, false, true, false);
        return;
      case XO_LFDUX:
        translate_load_store_fpr(ctx, insn, false, false, true, true);
        return;
      case XO_STFSX:
        translate_load_store_fpr(ctx, insn, true, true, true, false);
        return;
      case XO_STFSUX:
        translate_load_store_fpr(ctx, insn, true, true, true, true);
        return;
      case XO_STFDX:
        translate_load_store_fpr(ctx, insn, false, true, true, false);
        return;
      case XO_STFDUX:
        translate_load_store_fpr(ctx, insn, false, true, true, true);
        return;
      case XO_STFIWX: {
        const RTLOpcode rtlop =
            ctx->handle->host_little_endian ? RTLOP_STORE_BR : RTLOP_STORE;
        const int host_address = get_ea_indexed(ctx, insn, NULL);
        const int f64 = get_fpr_as_type(ctx, insn_frD(insn), RTLTYPE_FLOAT64);
        const int i64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, i64, f64, 0, 0);
        const int i32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, i32, i64, 0, 0);
        rtl_add_insn(unit, rtlop, 0, host_address, i32, 0);
        return;
      }  // case XO_STFIWX

      /* XO_5 = 0x18 */
      case XO_SLW:
        translate_shift(ctx, insn, RTLOP_SLL, false, false);
        return;
      case XO_SRW:
        translate_shift(ctx, insn, RTLOP_SRL, false, false);
        return;
      case XO_SRAW:
        translate_shift(ctx, insn, RTLOP_SRA, false, true);
        return;
      case XO_SRAWI:
        translate_shift(ctx, insn, RTLOP_SRAI, true, true);
        return;

      /* XO_5 = 0x1A */
      case XO_CNTLZW:
        if (!ctx->handle->pre_insn_callback
         && !ctx->handle->post_insn_callback
         && !insn_Rc(insn)
         && ((guest_ppc_get_insn_at(ctx, block, address+4) & 0xFFE0FFFE)
             == (OPCD_RLWINM<<26 | insn_rA(insn)<<21 | 27<<11 | 5<<6 | 31<<1)))
        {
            /* "cntlzw temp,rX; srwi rY,temp,5" is a common PowerPC idiom
             * for comparing a value to zero and getting the result as an
             * integer rather than a condition flag.  If the temporary is
             * different from the output registers, we leave the cntlzw in
             * place in case its result happens to also be used elsewhere;
             * dead store elimination will remove it if not. */
            const uint32_t next_insn =
                guest_ppc_get_insn_at(ctx, block, address+4);
            const int cntlzw_rS = insn_rS(insn);
            const int rlwinm_rA = insn_rA(next_insn);
            const int value = get_gpr(ctx, cntlzw_rS);
            /* Don't append the CLZ until after we retrieve the input
             * operand value, to correctly handle the case of cntlzw rN,rN
             * (overwriting the input operand). */
            if ((int)insn_rA(insn) != rlwinm_rA) {
                translate_bitmisc(ctx, insn, RTLOP_CLZ);
            }
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI, result, value, 0, 0);
            set_gpr(ctx, rlwinm_rA, result);
            if (insn_Rc(next_insn)) {
                const int lt = rtl_imm32(unit, 0);
                const int gt = result;
                const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_XORI, eq, result, 0, 1);
                const int so = get_xer_so(ctx);
                set_crf(ctx, 0, lt, gt, eq, so);
            }
            ctx->skip_next_insn = true;
            return;
        }
        translate_bitmisc(ctx, insn, RTLOP_CLZ);
        return;
      case XO_EXTSH:
        translate_bitmisc(ctx, insn, RTLOP_SEXT16);
        return;
      case XO_EXTSB:
        translate_bitmisc(ctx, insn, RTLOP_SEXT8);
        return;

      /* XO_5 = 0x1C */
      case XO_AND:
        translate_logic_reg(ctx, insn, RTLOP_AND, false, false);
        return;
      case XO_ANDC:
        translate_logic_reg(ctx, insn, RTLOP_AND, true, false);
        return;
      case XO_NOR:
        translate_logic_reg(ctx, insn, RTLOP_OR, false, true);
        return;
      case XO_EQV:
        /* The PowerPC spec describes this as ~(rS ^ rB), but we implement
         * it as (rS ^ ~rB) since that allows the NOT operation to be
         * scheduled earlier if rB is already loaded. */
        translate_logic_reg(ctx, insn, RTLOP_XOR, true, false);
        return;
      case XO_XOR:
        translate_logic_reg(ctx, insn, RTLOP_XOR, false, false);
        return;
      case XO_ORC:
        translate_logic_reg(ctx, insn, RTLOP_OR, true, false);
        return;
      case XO_OR:
        if (insn_rB(insn) == insn_rS(insn)) {  // mr rA,rS
            const int rS = get_gpr(ctx, insn_rS(insn));
            set_gpr(ctx, insn_rA(insn), rS);
            if (insn_Rc(insn)) {
                update_cr0(ctx, rS);
            }
        } else {
            translate_logic_reg(ctx, insn, RTLOP_OR, false, false);
        }
        return;
      case XO_NAND:
        translate_logic_reg(ctx, insn, RTLOP_AND, false, true);
        return;
    }

    translate_illegal(ctx, insn);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_x3F:  Translate the given opcode-0x3F instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 */
static inline void translate_x3F(
    GuestPPCContext * const ctx, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (insn_XO_5(insn) & 0x10) {

        switch ((PPCExtendedOpcode3F_5)insn_XO_5(insn)) {
          case XO_FDIV:
            translate_fp_arith(ctx, insn, RTLOP_FDIV, false,
                               FPSCR_VXIDI | FPSCR_VXZDZ);
            return;

          case XO_FSUB:
            translate_fp_arith(ctx, insn, RTLOP_FSUB, false, FPSCR_VXISI);
            return;

          case XO_FADD:
            translate_fp_arith(ctx, insn, RTLOP_FADD, false, FPSCR_VXISI);
            return;

          case XO_FSEL: {
            /* fsel does not raise any exceptions, so make sure we don't
             * affect host exception state with FCMP. */
            const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
            rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
            /* There's no need to convert frA to float64 if it's currently
             * float32, since all we do is test its value. */
            const RTLDataType frA_type =
                get_fpr_scalar_type(ctx, insn_frA(insn));
            const int frA = get_fpr_as_type(ctx, insn_frA(insn), frA_type);
            const int zero = rtl_alloc_register(unit, frA_type);
            rtl_add_insn(unit, RTLOP_LOAD_IMM, zero, 0, 0, 0);
            const int frC =
                get_fpr_as_type(ctx, insn_frC(insn), RTLTYPE_FLOAT64);
            const int frB =
                get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_FLOAT64);
            const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_FCMP, test, frA, zero, RTLFCMP_GE);
            const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_SELECT, result, frC, frB, test);
            set_fpr(ctx, insn_frD(insn), result);
            rtl_add_insn(unit, RTLOP_FSETSTATE, 0, fpstate, 0, 0);
            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_FSEL

          case XO_FMUL:
            translate_fp_arith(ctx, insn, RTLOP_FMUL, false, FPSCR_VXIMZ);
            return;

          case XO_FRSQRTE:
            translate_fp_recip(ctx, insn, true);
            return;

          case XO_FMSUB:
            translate_fp_fma(ctx, insn, RTLOP_FMSUB, false, false);
            return;

          case XO_FMADD:
            translate_fp_fma(ctx, insn, RTLOP_FMADD, false, false);
            return;

          case XO_FNMSUB:
            /* The PowerPC fnmsub instruction negates the final result
             * rather than just the intermediate product.  We could
             * potentially use RTLOP_FNMADD instead of RTLOP_FNMSUB,
             * but that gives the wrong sign of zero, so (unless the
             * relevant optimization is enabled) we have to use
             * RTLOP_FMSUB and manually negate the result. */
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FNMADD_ZERO_SIGN) {
                translate_fp_fma(ctx, insn, RTLOP_FNMADD, false, false);
            } else {
                translate_fp_fma(ctx, insn, RTLOP_FMSUB, false, true);
            }
            return;

          case XO_FNMADD:
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FNMADD_ZERO_SIGN) {
                translate_fp_fma(ctx, insn, RTLOP_FNMSUB, false, false);
            } else {
                translate_fp_fma(ctx, insn, RTLOP_FMADD, false, true);
            }
            return;
        }

    } else {  // !(insn_XO_5(insn) & 0x10)

        switch ((PPCExtendedOpcode3F_10)insn_XO_10(insn)) {
          case XO_FCMPU:
            translate_compare_fp(ctx, insn, false, 0);
            return;

          case XO_FCMPO:
            translate_compare_fp(ctx, insn, true, 0);
            return;

          case XO_MCRFS: {
            int crb[4];

            if (insn_crfS(insn) == 4) {
                const int fprf = get_fr_fi_fprf(ctx);
                for (int i = 0; i < 4; i++) {
                    crb[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[i], fprf, 0, (3-i) | 1<<8);
                }
                set_crf(ctx, insn_crfD(insn), crb[0], crb[1], crb[2], crb[3]);

            } else {  // crfS != 4
                const int crfS_bit = insn_crfS(insn) * 4;
                const int fpscr = get_fpscr(ctx);
                if (insn_crfS(insn) == 0) {
                    get_fpscr_fex_vx(ctx, fpscr, &crb[1], &crb[2]);
                    crb[0] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[0], fpscr, 0, 31 | 1<<8);
                    crb[3] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[3], fpscr, 0, 28 | 1<<8);
                } else if (insn_crfS(insn) == 3) {
                    const int fprf = get_fr_fi_fprf(ctx);
                    crb[0] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[0], fpscr, 0, 19 | 1<<8);
                    for (int i = 1; i < 4; i++) {
                        crb[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_BFEXT,
                                     crb[i], fprf, 0, (7-i) | 1<<8);
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        crb[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_BFEXT, crb[i], fpscr, 0,
                                     (31-(crfS_bit+i)) | 1<<8);
                    }
                }
                set_crf(ctx, insn_crfD(insn), crb[0], crb[1], crb[2], crb[3]);
                uint32_t mask = ((FPSCR_FX | FPSCR_ALL_EXCEPTIONS)
                                 & (0xF0000000 >> crfS_bit));
                if (mask) {
                    const int new_fpscr =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI, new_fpscr, fpscr, 0, ~mask);
                    set_fpscr(ctx, new_fpscr);
                }
            }

            return;
          }  // case XO_MCRFS

          case XO_MTFSB1:
          case XO_MTFSB0: {
            const uint32_t crbD_mask = 1 << (31 - insn_crbD(insn));

            if ((FPSCR_FR | FPSCR_FI | FPSCR_FPRF) & crbD_mask) {
                const int fprf = get_fr_fi_fprf(ctx);
                const int new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                const uint32_t mask = 1 << (19 - insn_crbD(insn));
                if (insn_XO_10(insn) == XO_MTFSB1) {
                    rtl_add_insn(unit, RTLOP_ORI, new_fprf, fprf, 0, mask);
                } else {
                    rtl_add_insn(unit, RTLOP_ANDI, new_fprf, fprf, 0, ~mask);
                }
                set_fr_fi_fprf(ctx, new_fprf);

            } else if ((FPSCR_FEX | FPSCR_VX | FPSCR_RESV20) & crbD_mask) {
                /* Do nothing -- these bits can't be written. */

            } else {
                const int fpscr = get_fpscr(ctx);
                const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                if (insn_XO_10(insn) == XO_MTFSB1) {
                    uint32_t mask = crbD_mask;
                    if (FPSCR_ALL_EXCEPTIONS & crbD_mask) {
                        mask |= FPSCR_FX;
                    }
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0, mask);
                } else {  // mtfsb0
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 new_fpscr, fpscr, 0, ~crbD_mask);
                }
                set_fpscr(ctx, new_fpscr);
                if (FPSCR_RN & crbD_mask) {
                    update_rounding_mode(ctx);
                }
            }

            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_MTFSB1, XO_MTFSB0

          case XO_MTFSFI: {
            const int crfD = insn_crfD(insn);

            if (crfD == 0) {
                const int fpscr = get_fpscr(ctx);
                const int masked_fpscr =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI,
                             masked_fpscr, fpscr, 0, 0x0FFFFFFF);
                int new_fpscr;
                if (insn_IMM(insn) & 9) {
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, masked_fpscr, 0,
                                 (insn_IMM(insn) & 9) << 28);
                } else {
                    new_fpscr = masked_fpscr;
                }
                set_fpscr(ctx, new_fpscr);

            } else if (ctx->use_split_fields && crfD == 3) {
                const int fpscr = get_fpscr(ctx);
                const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                if (insn_IMM(insn) & 8) {
                    /* The omission of FPSCR_FX here is deliberate, since
                     * mtfsfi does not set FPSCR[FX] for nonzero crfD. */
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0, 1<<19);
                } else {
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 new_fpscr, fpscr, 0, ~(1<<19));
                }
                set_fpscr(ctx, new_fpscr);
                const int fprf = get_fr_fi_fprf(ctx);
                const int masked_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, masked_fprf, fprf, 0, 0x0F);
                int new_fprf;
                if (insn_IMM(insn) & 7) {
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI, new_fprf, masked_fprf, 0,
                                 (insn_IMM(insn) & 7) << 4);
                } else {
                    new_fprf = masked_fprf;
                }
                set_fr_fi_fprf(ctx, new_fprf);

            } else if (ctx->use_split_fields && crfD == 4) {
                const int fprf = get_fr_fi_fprf(ctx);
                const int masked_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, masked_fprf, fprf, 0, 0x70);
                int new_fprf;
                if (insn_IMM(insn)) {
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI,
                                 new_fprf, masked_fprf, 0, insn_IMM(insn));
                } else {
                    new_fprf = masked_fprf;
                }
                set_fr_fi_fprf(ctx, new_fprf);

            } else {  // crfD not 0 (or 3/4 if use_split_fields)
                const int fpscr = get_fpscr(ctx);
                const int masked_fpscr =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                uint32_t mask = 0xF0000000 >> (insn_crfD(insn) * 4);
                rtl_add_insn(unit, RTLOP_ANDI, masked_fpscr, fpscr, 0, ~mask);
                int imm = insn_IMM(insn);
                if (insn_crfD(insn) == 5) {
                    imm &= 7;
                }
                int new_fpscr;
                if (insn_IMM(insn)) {
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, masked_fpscr, 0,
                                 imm << (28 - insn_crfD(insn)*4));
                } else {
                    new_fpscr = masked_fpscr;
                }
                set_fpscr(ctx, new_fpscr);
                if (insn_crfD(insn) == 7) {
                    static const uint8_t rounding_mode[4] = {
                        [FPSCR_RN_N] = RTLFROUND_NEAREST,
                        [FPSCR_RN_Z] = RTLFROUND_TRUNC,
                        [FPSCR_RN_P] = RTLFROUND_CEIL,
                        [FPSCR_RN_M] = RTLFROUND_FLOOR,
                    };
                    const int old_state =
                        rtl_alloc_register(unit, RTLTYPE_FPSTATE);
                    rtl_add_insn(unit, RTLOP_FGETSTATE, old_state, 0, 0, 0);
                    const int new_state =
                        rtl_alloc_register(unit, RTLTYPE_FPSTATE);
                    rtl_add_insn(unit, RTLOP_FSETROUND, new_state,
                                 old_state, 0, rounding_mode[imm & 3]);
                    rtl_add_insn(unit, RTLOP_FSETSTATE, 0, new_state, 0, 0);
                }
            }  // if (crfD == ...)

            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_MTFSFI

          case XO_MFFS: {
            int fpscr = get_fpscr(ctx);
            int fex, vx;
            get_fpscr_fex_vx(ctx, fpscr, &fex, &vx);
            if (ctx->use_split_fields) {
                fpscr = merge_fpscr(ctx, true);
            }
            const int shifted_fex = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI,
                         shifted_fex, fex, 0, FPSCR_FEX_SHIFT);
            const int shifted_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, shifted_vx, vx, 0, FPSCR_VX_SHIFT);
            const int fex_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, fex_vx, shifted_fex, shifted_vx, 0);
            const int final_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, final_fpscr, fpscr, fex_vx, 0);
            const int fpscr64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_ZCAST, fpscr64, final_fpscr, 0, 0);
            const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_BITCAST, result, fpscr64, 0, 0);
            set_fpr(ctx, insn_frD(insn), result);
            /* The value generated by mffs will never be a NaN, so we can
             * call it SNaN-safe even though there should never be a reason
             * to convert it to single precision. */
            ctx->fpr_is_safe |= 1 << insn_frD(insn);
            if (insn_Rc(insn)) {
                /* We already have FEX/VX, so avoid recomputing them. */
                const int fx = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT, fx, fpscr, 0, 31 | 1<<8);
                const int ox = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT, ox, fpscr, 0, 28 | 1<<8);
                set_crf(ctx, 1, fx, fex, vx, ox);
            }
            return;
          }  // case XO_MFFS

          case XO_MTFSF: {
            const int frB =
                get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_FLOAT64);
            const int bits64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_BITCAST, bits64, frB, 0, 0);
            const int bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ZCAST, bits, bits64, 0, 0);

            const int FM_fpscr_all = ctx->use_split_fields ? 0xF7 : 0xFF;
            const int FM_fpscr = insn_FM(insn) & FM_fpscr_all;
            const int FM_fprf =
                ctx->use_split_fields ? insn_FM(insn) & 0x18 : 0;

            if (FM_fpscr) {
                uint32_t fpscr_mask_off = FPSCR_FEX | FPSCR_VX | FPSCR_RESV20;
                if (ctx->use_split_fields) {
                    fpscr_mask_off |= FPSCR_FR | FPSCR_FI | FPSCR_FPRF;
                }
                const uint32_t fpscr_mask = ~fpscr_mask_off;
                int new_fpscr;
                if (FM_fpscr == FM_fpscr_all) {
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 new_fpscr, bits, 0, fpscr_mask);
                } else {
                    const uint32_t mask = crm_to_mask(FM_fpscr) & fpscr_mask;
                    const int fpscr = get_fpscr(ctx);
                    const int masked_bits =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI, masked_bits, bits, 0, mask);
                    const int masked_fpscr =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 masked_fpscr, fpscr, 0, ~mask);
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_OR,
                                 new_fpscr, masked_fpscr, masked_bits, 0);
                }
                set_fpscr(ctx, new_fpscr);
                if (FM_fpscr & 0x01) {
                    update_rounding_mode(ctx);
                }
            }

            if (FM_fprf) {
                int new_fprf;
                if (FM_fprf == 0x18) {
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 new_fprf, bits, 0, FPSCR_FPRF_SHIFT | 7<<8);
                } else {
                    const int fprf = get_fr_fi_fprf(ctx);
                    const int shifted_bits =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_SRLI,
                                 shifted_bits, bits, 0, FPSCR_FPRF_SHIFT);
                    const int masked_bits =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    const int masked_fprf =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    if (FM_fprf == 0x10) {
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_bits, shifted_bits, 0, 0x70);
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_fprf, fprf, 0, 0x0F);
                    } else {
                        ASSERT(FM_fprf == 0x08);
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_bits, shifted_bits, 0, 0x0F);
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_fprf, fprf, 0, 0x70);
                    }
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_OR,
                                 new_fprf, masked_fprf, masked_bits, 0);
                }
                set_fr_fi_fprf(ctx, new_fprf);
            }

            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_MTFSF

          case XO_FNEG:
            translate_move_fpr(ctx, insn, RTLOP_FNEG, false);
            return;

          case XO_FMR:
            translate_move_fpr(ctx, insn, RTLOP_MOVE, false);
            return;

          case XO_FNABS:
            translate_move_fpr(ctx, insn, RTLOP_FNABS, false);
            return;

          case XO_FABS:
            translate_move_fpr(ctx, insn, RTLOP_FABS, false);
            return;

          case XO_FRSP: {
            const int frB =
                get_fpr_as_type(ctx, insn_frB(insn), RTLTYPE_FLOAT64);
            const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_FCVT, result, frB, 0, 0);
            set_fp_result(ctx, insn_frD(insn), result, 0, 0, frB, 0,
                          0, 0, true, false, true, true);
            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_FRSP

          case XO_FCTIW:
            translate_fctiw(ctx, insn, RTLOP_FROUNDI);
            return;

          case XO_FCTIWZ:
            translate_fctiw(ctx, insn, RTLOP_FTRUNCI);
            return;
        }

    }  // if (insn_XO_5(insn) & 0x10)

    translate_illegal(ctx, insn);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_insn:  Translate the given instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static inline void translate_insn(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    /* Skip instructions which were translated as part of an optimized
     * instruction pair. */
    if (ctx->skip_next_insn) {
        ctx->skip_next_insn = false;
        return;
    }

    switch (insn_OPCD(insn)) {
      case OPCD_TWI:
        translate_trap(ctx, address, insn, rtl_imm32(unit, insn_SIMM(insn)));
        return;

      case OPCD_x04:
        switch ((PPCExtendedOpcode04_750CL_5)insn_XO_5(insn)) {
          case XO_PS_CMP:
          case XO_PS_MOVE:
          case XO_PS_MERGE:
          case XO_PS_MISC:
            switch ((PPCExtendedOpcode04_750CL_10)insn_XO_10(insn)) {
              case XO_PS_CMPU0:
                translate_compare_fp(ctx, insn, false, 0);
                return;
              case XO_PS_CMPO0:
                translate_compare_fp(ctx, insn, true, 0);
                return;
              case XO_PS_CMPU1:
                translate_compare_fp(ctx, insn, false, 1);
                return;
              case XO_PS_CMPO1:
                translate_compare_fp(ctx, insn, true, 1);
                return;
              case XO_PS_NEG:
                translate_move_fpr(ctx, insn, RTLOP_FNEG, true);
                return;
              case XO_PS_MR:
                translate_move_fpr(ctx, insn, RTLOP_MOVE, true);
                return;
              case XO_PS_NABS:
                translate_move_fpr(ctx, insn, RTLOP_FNABS, true);
                return;
              case XO_PS_ABS:
                translate_move_fpr(ctx, insn, RTLOP_FABS, true);
                return;
              case XO_PS_MERGE00:
                translate_ps_merge(ctx, insn, 0, 0);
                return;
              case XO_PS_MERGE01:
                translate_ps_merge(ctx, insn, 0, 1);
                return;
              case XO_PS_MERGE10:
                translate_ps_merge(ctx, insn, 1, 0);
                return;
              case XO_PS_MERGE11:
                translate_ps_merge(ctx, insn, 1, 1);
                return;
              case XO_DCBZ_L:
                /* We treat "locked" cache identically to normal cache. */
                translate_dcbz(ctx, insn);
                return;
            }
            translate_illegal(ctx, insn);
            return;

          case XO_PSQ_LX:
            translate_load_store_ps(ctx, insn, false, true,
                                    (insn_XO_10(insn) & 0x20) != 0);
            return;
          case XO_PSQ_STX:
            translate_load_store_ps(ctx, insn, true, true,
                                    (insn_XO_10(insn) & 0x20) != 0);
            return;
          case XO_PS_SUM0:
            translate_ps_sum(ctx, insn, 0);
            return;
          case XO_PS_SUM1:
            translate_ps_sum(ctx, insn, 1);
            return;
          case XO_PS_MULS0:
            translate_ps_arith(ctx, insn, RTLOP_FMUL, 0, FPSCR_VXIMZ);
            return;
          case XO_PS_MULS1:
            translate_ps_arith(ctx, insn, RTLOP_FMUL, 1, FPSCR_VXIMZ);
            return;
          case XO_PS_MADDS0:
            translate_ps_fma(ctx, insn, RTLOP_FMADD, 0, false);
            return;
          case XO_PS_MADDS1:
            translate_ps_fma(ctx, insn, RTLOP_FMADD, 1, false);
            return;
          case XO_PS_DIV:
            translate_ps_arith(ctx, insn, RTLOP_FDIV, -1,
                               FPSCR_VXIDI | FPSCR_VXZDZ);
            return;
          case XO_PS_SUB:
            translate_ps_arith(ctx, insn, RTLOP_FSUB, -1, FPSCR_VXISI);
            return;
          case XO_PS_ADD:
            translate_ps_arith(ctx, insn, RTLOP_FADD, -1, FPSCR_VXISI);
            return;
          case XO_PS_SEL:
            translate_ps_sel(ctx, insn);
            return;
          case XO_PS_RES:
            translate_ps_recip(ctx, insn, false);
            return;
          case XO_PS_MUL:
            translate_ps_arith(ctx, insn, RTLOP_FMUL, -1, FPSCR_VXIMZ);
            return;
          case XO_PS_RSQRTE:
            translate_ps_recip(ctx, insn, true);
            return;
          case XO_PS_MSUB:
            translate_ps_fma(ctx, insn, RTLOP_FMSUB, -1, false);
            return;
          case XO_PS_MADD:
            translate_ps_fma(ctx, insn, RTLOP_FMADD, -1, false);
            return;
          case XO_PS_NMSUB:
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FNMADD_ZERO_SIGN) {
                translate_ps_fma(ctx, insn, RTLOP_FNMADD, -1, false);
            } else {
                translate_ps_fma(ctx, insn, RTLOP_FMSUB, -1, true);
            }
            return;
          case XO_PS_NMADD:
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FNMADD_ZERO_SIGN) {
                translate_ps_fma(ctx, insn, RTLOP_FNMSUB, -1, false);
            } else {
                translate_ps_fma(ctx, insn, RTLOP_FMADD, -1, true);
            }
            return;
        }
        translate_illegal(ctx, insn);
        return;

      case OPCD_MULLI:
        translate_arith_imm(ctx, insn, RTLOP_MULI, false, false, false);
        return;

      case OPCD_SUBFIC: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int32_t imm = insn_SIMM(insn);
        const int imm_reg = rtl_imm32(unit, imm);
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, imm_reg, rA, 0);
        set_gpr(ctx, insn_rD(insn), result);

        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        if (imm == -1) {
            rtl_add_insn(unit, RTLOP_LOAD_IMM, ca, 0, 0, 1);
        } else {
            rtl_add_insn(unit, RTLOP_SLTUI, ca, result, 0, imm+1);
        }
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer, -1);

        return;
      }  // case OPCD_SUBFIC

      case OPCD_CMPLI:
        translate_compare(ctx, insn, true, false);
        return;

      case OPCD_CMPI:
        translate_compare(ctx, insn, true, true);
        return;

      case OPCD_ADDIC:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, false);
        return;

      case OPCD_ADDIC_:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, true);
        return;

      case OPCD_ADDI:
        if (insn_rA(insn) == 0) {  // li
            set_gpr(ctx, insn_rD(insn), rtl_imm32(unit, insn_SIMM(insn)));
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, false, false, false);
        }
        return;

      case OPCD_ADDIS:
        if (insn_rA(insn) == 0) {  // lis
            set_gpr(ctx, insn_rD(insn), rtl_imm32(unit, insn_SIMM(insn) << 16));
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, true, false, false);
        }
        return;

      case OPCD_BC:
        translate_branch(ctx, address, insn_BO(insn), insn_BI(insn),
                         insn_BD(insn), insn_AA(insn), insn_LK(insn));
        return;

      case OPCD_SC: {
        guest_ppc_flush_cr(ctx, false);
        guest_ppc_flush_fpscr(ctx);
        flush_live_regs(ctx, true);
        set_nia_imm(ctx, address + 4);
        const int sc_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD, sc_handler, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_sc_handler);
        rtl_add_insn(unit, RTLOP_CALL, 0, sc_handler, ctx->psb_reg, 0);
        post_insn_callback(ctx, address);
        rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);
        return;
      }  // case OPCD_SC

      case OPCD_B:
        translate_branch(ctx, address, 0x14, 0, insn_LI(insn),
                         insn_AA(insn), insn_LK(insn));
        return;

      case OPCD_x13:
        switch ((PPCExtendedOpcode13)insn_XO_10(insn)) {
          case XO_MCRF: {
            if (ctx->use_split_fields) {
                int crb[4];
                for (int i = 0; i < 4; i++) {
                    crb[i] = get_crb(ctx, insn_crfS(insn)*4 + i);
                }
                set_crf(ctx, insn_crfD(insn), crb[0], crb[1], crb[2], crb[3]);
            } else {
                const int crfS_bit = 4 * insn_crfS(insn);
                const int crfD_bit = 4 * insn_crfD(insn);
                const int old_cr = get_cr(ctx);
                const int field = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI,
                             field, old_cr, 0, 0xF0000000 >> crfS_bit);
                const int masked_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI,
                             masked_cr, old_cr, 0, ~(0xF0000000 >> crfD_bit));
                const int shifted_field =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                if (crfD_bit > crfS_bit) {
                    rtl_add_insn(unit, RTLOP_SRLI,
                                 shifted_field, field, 0, crfD_bit - crfS_bit);
                } else {
                    rtl_add_insn(unit, RTLOP_SLLI,
                                 shifted_field, field, 0, crfS_bit - crfD_bit);
                }
                const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_OR,
                             new_cr, masked_cr, shifted_field, 0);
                set_cr(ctx, new_cr);
            }
            return;
          }
          case XO_BCLR:
            translate_branch_terminal(ctx, address, insn_BO(insn),
                                      insn_BI(insn), insn_LK(insn),
                                      0, true, false);
            return;
          case XO_CRNOR:
            /* "crnor crbD,crbA,crbB" is also known as "crnot crbD,crbA",
             * but the usage frequency of crnot is probably not very high
             * in typical code and the potential speed gain is minimal, so
             * we don't bother with special handling. */
            translate_logic_crb(ctx, insn, RTLOP_OR, false, true);
            return;
          case XO_RFI:
            translate_unimplemented_insn(ctx, address, insn);
            return;
          case XO_CRANDC:
            translate_logic_crb(ctx, insn, RTLOP_AND, true, false);
            return;
          case XO_ISYNC:
            // FIXME: We currently act as if all instructions are sequential.
            return;
          case XO_CRXOR:
            if (insn_crbA(insn) == insn_crbB(insn)) {  // crclr
                set_crb(ctx, insn_crbD(insn), rtl_imm32(unit,0));
            } else {
                translate_logic_crb(ctx, insn, RTLOP_XOR, false, false);
            }
            return;
          case XO_CRNAND:
            translate_logic_crb(ctx, insn, RTLOP_AND, false, true);
            return;
          case XO_CRAND:
            translate_logic_crb(ctx, insn, RTLOP_AND, false, false);
            return;
          case XO_CREQV:
            if (insn_crbA(insn) == insn_crbB(insn)) {  // crset
                set_crb(ctx, insn_crbD(insn), rtl_imm32(unit,1));
            } else {
                /* See note at XO_EQV under opcode 0x1F (though it's less
                 * likely to help in this case). */
                translate_logic_crb(ctx, insn, RTLOP_XOR, true, false);
            }
            return;
          case XO_CRORC:
            translate_logic_crb(ctx, insn, RTLOP_OR, true, false);
            return;
          case XO_CROR:
            /* "cror crbD,crbA,crbB" is also known as "crmove crbD,crbA",
             * but the usage frequency of crmove is probably not very high
             * in typical code and the potential speed gain is minimal, so
             * we don't bother with special handling. */
            translate_logic_crb(ctx, insn, RTLOP_OR, false, false);
            return;
          case XO_BCCTR:
            if (!(insn_BO(insn) & 0x04)) {  // Invalid BO field for bcctr.
                translate_illegal(ctx, insn);
                return;
            }
            translate_branch_terminal(ctx, address, insn_BO(insn),
                                      insn_BI(insn), insn_LK(insn),
                                      0, false, true);
            return;
        }
        translate_illegal(ctx, insn);
        return;

      case OPCD_RLWIMI:
        translate_rotate_mask(ctx, insn, true, true);
        return;

      case OPCD_RLWINM:
        translate_rotate_mask(ctx, insn, true, false);
        return;

      case OPCD_RLWNM:
        translate_rotate_mask(ctx, insn, false, false);
        return;

      case OPCD_ORI:
        if (insn == 0x60000000) {  // nop
            return;
        }
        translate_logic_imm(ctx, insn, RTLOP_ORI, false, false);
        return;

      case OPCD_ORIS:
        translate_logic_imm(ctx, insn, RTLOP_ORI, true, false);
        return;

      case OPCD_XORI:
        translate_logic_imm(ctx, insn, RTLOP_XORI, false, false);
        return;

      case OPCD_XORIS:
        translate_logic_imm(ctx, insn, RTLOP_XORI, true, false);
        return;

      case OPCD_ANDI_:
        translate_logic_imm(ctx, insn, RTLOP_ANDI, false, true);
        return;

      case OPCD_ANDIS_:
        translate_logic_imm(ctx, insn, RTLOP_ANDI, true, true);
        return;

      case OPCD_x1F:
        translate_x1F(ctx, block, address, insn);
        return;

      case OPCD_LWZ:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, false, false);
        return;

      case OPCD_LWZU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, false, true);
        return;

      case OPCD_LBZ:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8,
                                 false, false, false);
        return;

      case OPCD_LBZU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8,
                                 false, false, true);
        return;

      case OPCD_STW:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, false, false);
        return;

      case OPCD_STWU:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, false, true);
        return;

      case OPCD_STB:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8,
                                 true, false, false);
        return;

      case OPCD_STBU:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8,
                                 true, false, true);
        return;

      case OPCD_LHZ:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16,
                                 false, false, false);
        return;

      case OPCD_LHZU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16,
                                 false, false, true);
        return;

      case OPCD_LHA:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16,
                                 false, false, false);
        return;

      case OPCD_LHAU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16,
                                 false, false, true);
        return;

      case OPCD_STH:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16,
                                 true, false, false);
        return;

      case OPCD_STHU:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16,
                                 true, false, true);
        return;

      case OPCD_LMW:
        translate_load_store_multiple(ctx, insn, false);
        return;

      case OPCD_STMW:
        translate_load_store_multiple(ctx, insn, true);
        return;

      case OPCD_LFS:
        translate_load_store_fpr(ctx, insn, true, false, false, false);
        return;

      case OPCD_LFSU:
        translate_load_store_fpr(ctx, insn, true, false, false, true);
        return;

      case OPCD_LFD:
        translate_load_store_fpr(ctx, insn, false, false, false, false);
        return;

      case OPCD_LFDU:
        translate_load_store_fpr(ctx, insn, false, false, false, true);
        return;

      case OPCD_STFS:
        translate_load_store_fpr(ctx, insn, true, true, false, false);
        return;

      case OPCD_STFSU:
        translate_load_store_fpr(ctx, insn, true, true, false, true);
        return;

      case OPCD_STFD:
        translate_load_store_fpr(ctx, insn, false, true, false, false);
        return;

      case OPCD_STFDU:
        translate_load_store_fpr(ctx, insn, false, true, false, true);
        return;

      case OPCD_PSQ_L:
        translate_load_store_ps(ctx, insn, false, false, false);
        return;

      case OPCD_PSQ_LU:
        translate_load_store_ps(ctx, insn, false, false, true);
        return;

      case OPCD_PSQ_ST:
        translate_load_store_ps(ctx, insn, true, false, false);
        return;

      case OPCD_PSQ_STU:
        translate_load_store_ps(ctx, insn, true, false, true);
        return;

      case OPCD_x3B:
        switch ((PPCExtendedOpcode3B)insn_XO_5(insn)) {
          case XO_FDIVS:
            translate_fp_arith(ctx, insn, RTLOP_FDIV, true,
                              FPSCR_VXIDI | FPSCR_VXZDZ);
            return;
          case XO_FSUBS:
            translate_fp_arith(ctx, insn, RTLOP_FSUB, true, FPSCR_VXISI);
            return;
          case XO_FADDS:
            translate_fp_arith(ctx, insn, RTLOP_FADD, true, FPSCR_VXISI);
            return;
          case XO_FRES:
            translate_fp_recip(ctx, insn, false);
            return;
          case XO_FMULS:
            translate_fp_arith(ctx, insn, RTLOP_FMUL, true, FPSCR_VXIMZ);
            return;
          case XO_FMSUBS:
            translate_fp_fma(ctx, insn, RTLOP_FMSUB, true, false);
            return;
          case XO_FMADDS:
            translate_fp_fma(ctx, insn, RTLOP_FMADD, true, false);
            return;
          case XO_FNMSUBS:
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FNMADD_ZERO_SIGN) {
                translate_fp_fma(ctx, insn, RTLOP_FNMADD, true, false);
            } else {
                translate_fp_fma(ctx, insn, RTLOP_FMSUB, true, true);
            }
            return;
          case XO_FNMADDS:
            if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_FNMADD_ZERO_SIGN) {
                translate_fp_fma(ctx, insn, RTLOP_FNMSUB, true, false);
            } else {
                translate_fp_fma(ctx, insn, RTLOP_FMADD, true, true);
            }
            return;
        }
        translate_illegal(ctx, insn);
        return;

      case OPCD_x3F:
        translate_x3F(ctx, insn);
        return;

    }  // switch (insn_OPCD(insn))

    translate_illegal(ctx, insn);
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

bool guest_ppc_translate_block(GuestPPCContext *ctx, int index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(index >= 0 && index < ctx->num_blocks);

    ctx->current_block = index;

    RTLUnit * const unit = ctx->unit;
    GuestPPCBlockInfo *block = &ctx->blocks[index];
    const uint32_t start = block->start;
    const uint32_t *memory_base =
        (const uint32_t *)ctx->handle->setup.guest_memory_base;

    if (block->is_branch_target) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, block->label);
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to add label at 0x%X", start);
            return false;
        }
    }

    if (UNLIKELY(block->len == 0)) {
        /* This block was a backward branch target that wasn't part of a
         * previous block (see block-splitting logic at the bottom of
         * guest_ppc_scan()).  Update NIA and return to the caller to
         * retranslate from the target address. */
        return_from_unit(ctx, ~0, rtl_imm32(unit, start), false);
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate empty block at 0x%X",
                    start);
            return false;
        }
        return true;
    }

    memset(&ctx->live, 0, sizeof(ctx->live));
    ctx->fpr_dirty = 0;
    ctx->fpr_is_safe = 0;
    ctx->ps1_is_safe = 0;
    ctx->crb_dirty = 0;
    memset(&ctx->last_set, -1, sizeof(ctx->last_set));

    ctx->paired_lwarx_data_be = 0;
    ctx->skip_next_insn = false;

    for (uint32_t ofs = 0; ofs < block->len; ofs += 4) {
        const uint32_t address = start + ofs;
        if (ctx->handle->pre_insn_callback) {
            flush_live_regs(ctx, false);
            const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_LOAD_IMM, func, 0, 0,
                         (uintptr_t)ctx->handle->pre_insn_callback);
            rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT,
                         0, func, ctx->psb_reg, rtl_imm32(unit, address));
        }

        const uint32_t insn = bswap_be32(memory_base[address/4]);
        translate_insn(ctx, block, address, insn);

        /* Explicitly check for the presence of a callback (even though
         * post_insn_callback() does so too) so we don't repeatedly set
         * NIA if it's not necessary. */
        if (ctx->handle->post_insn_callback) {
            set_nia_imm(ctx, address + 4);
            post_insn_callback(ctx, address);
        }

        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate instruction at 0x%X",
                    address);
            return false;
        }
    }

    /* If the last instruction of the block is not a branch or trap, check
     * for dead CR stores (if requested) before entering the next block. */
    if (ctx->trim_cr_stores && !block->has_branch && !block->has_trap) {
        guest_ppc_trim_cr_stores(ctx, 0x14, 0, NULL, NULL, NULL, NULL);
    }

    flush_live_regs(ctx, true);
    set_nia_imm(ctx, start + block->len);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to update registers after block end 0x%X",
                start + block->len - 4);
        return false;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

void guest_ppc_flush_cr(GuestPPCContext *ctx, bool make_live)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    if (!ctx->use_split_fields) {
        return;
    }

    RTLUnit * const unit = ctx->unit;

    if (ctx->crb_changed_bitrev) {
        const int cr = merge_cr(ctx, make_live);
        if (make_live) {
            set_cr(ctx, cr);
            memset(ctx->last_set.crb, -1, sizeof(ctx->last_set.crb));
            memset(ctx->live.crb, 0, sizeof(ctx->live.crb));
            ctx->crb_dirty = 0;
        } else {
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, cr, 0, ctx->alias.cr);
        }
    }
}

/*-----------------------------------------------------------------------*/

void guest_ppc_flush_fpscr(GuestPPCContext *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    if (!ctx->use_split_fields) {
        return;
    }

    RTLUnit * const unit = ctx->unit;

    if (ctx->fpscr_changed && ctx->alias.fr_fi_fprf) {
        const int fpscr = merge_fpscr(ctx, false);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, fpscr, 0, ctx->alias.fpscr);
        if (ctx->live.fpscr) {
            ctx->live.fpscr = fpscr;
            ctx->last_set.fpscr = -1;
        }
    }
}

/*-----------------------------------------------------------------------*/

int guest_ppc_get_epilogue_label(GuestPPCContext *ctx)
{
    if (!ctx->epilogue_label) {
        ctx->epilogue_label = rtl_alloc_label(ctx->unit);
    }
    return ctx->epilogue_label;
}

/*************************************************************************/
/*************************************************************************/
