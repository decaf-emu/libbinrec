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

    int reg, alias;
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, alias));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[src1].source != RTLREG_UNDEFINED");
    EXPECT_EQ(unit->num_insns, 0);
    EXPECT(unit->error);
    unit->error = false;

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);

#endif  // ENABLE_OPERAND_SANITY_CHECKS

    return EXIT_SUCCESS;
}
