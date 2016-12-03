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
static const unsigned int host_opt = BINREC_OPT_H_X86_FIXED_REGS
                                   | BINREC_OPT_H_X86_MERGE_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    /* reg4 should get EDX and reserve it for merging via MERGE_REGS. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, reg3, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg4, 0, alias));

    int reg5, reg6, reg7, reg8, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg6, 0, 0, 6));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* This should allocate EDX to reg7 via FIXED_REGS, then cancel the
     * merge from reg4 to reg8 at final allocation time. */
    EXPECT(rtl_add_insn(unit, RTLOP_MULHU, reg7, reg5, reg6, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg5, reg6, 0));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg8, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg8, 0, alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0xBA,0x04,0x00,0x00,0x00,           // mov $3,%edx
    0x44,0x8B,0xC2,                     // mov %edx,%r8d
    0xB8,0x05,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x06,0x00,0x00,0x00,           // mov $2,%ecx
    0x48,0x8B,0xF0,                     // mov %rax,%rsi
    0xF7,0xE1,                          // mul %ecx
    0x48,0x8B,0xC6,                     // mov %rsi,%rax
    0x44,0x89,0x87,0x34,0x12,0x00,0x00, // mov %r8d,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
