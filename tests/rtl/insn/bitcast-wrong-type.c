/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
#ifdef ENABLE_OPERAND_SANITY_CHECKS

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT64));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg4, 0, 0, UINT64_C(0x4010000000000000)));
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT_FALSE(unit->error);

    const char *error_msg = ("Operand constraint violated:"
                             " (unit->regs[dest].type == RTLTYPE_INT32"
                             " && unit->regs[src1].type == RTLTYPE_FLOAT32)"
                             " || (unit->regs[dest].type == RTLTYPE_INT64"
                             " && unit->regs[src1].type == RTLTYPE_FLOAT64)"
                             " || (unit->regs[dest].type == RTLTYPE_FLOAT32"
                             " && unit->regs[src1].type == RTLTYPE_INT32)"
                             " || (unit->regs[dest].type == RTLTYPE_FLOAT64"
                             " && unit->regs[src1].type == RTLTYPE_INT64)");

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg5, reg2, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg5, reg3, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg5, reg4, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg6, reg1, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg6, reg3, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg6, reg4, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg7, reg1, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg7, reg2, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg7, reg4, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg8, reg1, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg8, reg2, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BITCAST, reg8, reg3, 0, 0));
    EXPECT_ICE(error_msg);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT(unit->error);
    unit->error = false;

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);

#endif  // ENABLE_OPERAND_SANITY_CHECKS

    return EXIT_SUCCESS;
}
