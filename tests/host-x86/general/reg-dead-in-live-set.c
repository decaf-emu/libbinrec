/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/host-x86.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/host-x86/common.h"
#include "tests/log-capture.h"


int main(void)
{
#ifdef ENABLE_OPERAND_SANITY_CHECKS

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));
    binrec_set_optimization_flags(handle, 0, 0, BINREC_OPT_BASIC);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    int reg1, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));

    EXPECT(rtl_finalize_unit(unit));
    EXPECT(rtl_optimize_unit(unit, BINREC_OPT_BASIC));

    /* Fudge register live ranges to trigger the dead-register-in-live-set
     * error. */
    unit->regs[reg1].death = 2;

    handle->code_buffer_size = 128;
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));

    EXPECT_FALSE(host_x86_translate(handle, unit));
    EXPECT_ICE("Register 1 in live set but not live on entry to block 2");

    binrec_code_free(handle, handle->code_buffer);
    handle->code_buffer = NULL;
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);

#endif  // ENABLE_OPERAND_SANITY_CHECKS

    return EXIT_SUCCESS;
}
