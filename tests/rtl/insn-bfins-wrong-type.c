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

    uint32_t reg1, reg2, reg3, reg4, reg5, reg6, reg7, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 30));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg4, 0, 0, alias));
    EXPECT_EQ(unit->num_insns, 4);
    /* int32 = address, int32 */
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BFINS, reg5, reg3, reg1, 2 | 5<<8));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[src1].type == unit->regs[dest].type");
    EXPECT_EQ(unit->num_insns, 4);
    /* int32 = int32, address */
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BFINS, reg5, reg1, reg3, 2 | 5<<8));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[src2].type == unit->regs[dest].type");
    EXPECT_EQ(unit->num_insns, 4);
    /* address = int32, int32 */
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BFINS, reg6, reg1, reg2, 2 | 5<<8));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[src1].type == unit->regs[dest].type");
    EXPECT_EQ(unit->num_insns, 4);
    /* float = float, float */
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_BFINS, reg7, reg4, reg4, 2 | 5<<8));
    EXPECT_ICE("Operand constraint violated:"
               " rtl_register_is_int(&unit->regs[dest])");
    EXPECT_EQ(unit->num_insns, 4);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);

#endif  // ENABLE_OPERAND_SANITY_CHECKS

    return EXIT_SUCCESS;
}
