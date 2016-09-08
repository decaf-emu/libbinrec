/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/common.h"
#include "tests/host-x86/common.h"


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_SYSV,
};

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    uint32_t reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));

    /* If operand sanity checks are enabled, we can't directly add an
     * instruction with undefined inputs, so we add a LOAD_IMM (to set the
     * live range on the destination register properly) and modify the
     * instruction entry directly. */
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0));
    unit->insns[unit->num_insns-1].opcode = RTLOP_SELECT;
    unit->insns[unit->num_insns-1].src1 = reg1;
    unit->insns[unit->num_insns-1].src2 = reg2;
    unit->insns[unit->num_insns-1].cond = reg3;

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x85,0xC0,                          // test %eax,%eax
    0x8B,0xC8,                          // mov %eax,%ecx
    0x0F,0x44,0xC8,                     // cmovz %eax,%ecx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
