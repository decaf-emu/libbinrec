/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/endian.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/guest-ppc/insn/common.h"
#include "tests/log-capture.h"


int main(void)
{
    uint32_t input[1];
    input[0] = bswap_be32(0xEC21082A);  // fadds f1,f1,f1

    binrec_setup_t final_setup = setup;
    final_setup.guest_memory_base = input;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    binrec_set_optimization_flags(handle,
                                  0, BINREC_OPT_G_PPC_NO_FPSCR_STATE*0, 0);
    binrec_set_code_range(handle, 0, sizeof(input) - 1);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    unit->regs_size = REGS_LIMIT;
    EXPECT(unit->regs = realloc(unit->regs,
                                unit->regs_size * sizeof(*unit->regs)));
    memset(unit->regs, 0, unit->regs_size * sizeof(*unit->regs));
    unit->next_reg = unit->regs_size - 4;

    /* This should fail due to running out of registers, but it shouldn't
     * trigger an assertion failure. */
    EXPECT_FALSE(guest_ppc_translate(handle, 0, sizeof(input) - 1, unit));

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
