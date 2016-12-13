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

    int reg1, reg2, reg3, reg4, reg5, alias1, alias2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32));

    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg1, 0, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias2));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT_FALSE(unit->error);

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_VFCMP, reg4, reg1, reg2, RTLFCMP_LT));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[dest].type == RTLTYPE_INT64");
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_VFCMP, reg5, reg3, reg1, RTLFCMP_LT));
    EXPECT_ICE("Operand constraint violated:"
               " rtl_register_is_vector(&unit->regs[src1])");
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_VFCMP, reg5, reg1, reg3, RTLFCMP_LT));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[src2].type == unit->regs[src1].type");
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT(unit->error);
    unit->error = false;

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);

#endif  // ENABLE_OPERAND_SANITY_CHECKS

    return EXIT_SUCCESS;
}
