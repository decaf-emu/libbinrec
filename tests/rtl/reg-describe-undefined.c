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


int main(void)
{
    /* If operand sanity checks are disabled, rtl.c:rtl_describe_register()
     * will never see an RTLREG_UNDEFINED register, so we feed it one here
     * just to test the code path. */

    static RTLInsn insn = {.opcode = RTLOP_RETURN, .src1 = 1};
    static RTLRegister regs[2] =
        {{}, {.type = RTLTYPE_INT32, .source = RTLREG_UNDEFINED}};
    static RTLUnit unit = {
        .insns = &insn,
        .num_insns = 1,
        .blocks = (void *)1,  // Avoid assertion failure.
        .num_blocks = 0,
        .regs = regs,
        .next_reg = 2,
        .label_blockmap = (void *)1,  // Avoid assertion failure.
    };

    char *output = rtl_disassemble_unit(&unit, true);
    EXPECT_STREQ(output, ("    0: RETURN     r1\n"
                          "           r1: (undefined)\n"
                          "\n"));

    free(output);
    return EXIT_SUCCESS;
}
