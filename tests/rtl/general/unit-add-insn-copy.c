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
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));

    RTLInsn insn;
    memset(&insn, 0, sizeof(insn));
    insn.opcode = RTLOP_ADD;
    EXPECT(rtl_add_insn_copy(unit, &insn));
    EXPECT_EQ(unit->num_insns, 1);
    EXPECT_EQ(unit->insns[0].opcode, RTLOP_ADD);
    EXPECT_EQ(unit->insns[0].dest, 0);
    EXPECT_EQ(unit->insns[0].src1, 0);
    EXPECT_EQ(unit->insns[0].src2, 0);
    EXPECT_EQ(unit->insns[0].src_imm, 0);
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT_FALSE(unit->error);

    /* Subsequent instructions should not start another new block.
     * Also check behavior when the instruction array needs to be expanded. */
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT_FALSE(unit->error);
    unit->insns_size = 3;
    memset(&insn, 0, sizeof(insn));
    insn.opcode = RTLOP_ILLEGAL;
    insn.dest = reg1;
    insn.src1 = reg2;
    insn.src2 = reg3;
    insn.src_imm = UINT64_C(0x123456789ABCDEF0);
    EXPECT(rtl_add_insn_copy(unit, &insn));
    EXPECT(unit->insns_size > 3);
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT_EQ(unit->insns[0].opcode, RTLOP_ADD);
    EXPECT_EQ(unit->insns[0].dest, 0);
    EXPECT_EQ(unit->insns[0].src1, 0);
    EXPECT_EQ(unit->insns[0].src2, 0);
    EXPECT_EQ(unit->insns[0].src_imm, 0);
    EXPECT_EQ(unit->insns[1].opcode, RTLOP_LOAD_IMM);
    EXPECT_EQ(unit->insns[1].dest, 2);
    EXPECT_EQ(unit->insns[1].src_imm, 2);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_LOAD_IMM);
    EXPECT_EQ(unit->insns[2].dest, 3);
    EXPECT_EQ(unit->insns[2].src_imm, 3);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_ILLEGAL);
    EXPECT_EQ(unit->insns[3].dest, reg1);
    EXPECT_EQ(unit->insns[3].src1, reg2);
    EXPECT_EQ(unit->insns[3].src2, reg3);
    EXPECT_EQ(unit->insns[3].src_imm, UINT64_C(0x123456789ABCDEF0));
    EXPECT(unit->regs[1].live);
    EXPECT_EQ(unit->regs[reg1].birth, 3);
    EXPECT_EQ(unit->regs[reg1].death, 3);
    EXPECT(unit->regs[reg2].live);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 3);
    EXPECT(unit->regs[reg3].live);
    EXPECT_EQ(unit->regs[reg3].birth, 2);
    EXPECT_EQ(unit->regs[reg3].death, 3);
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT_FALSE(unit->error);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
