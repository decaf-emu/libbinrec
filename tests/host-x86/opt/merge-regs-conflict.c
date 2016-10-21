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
static const unsigned int host_opt = BINREC_OPT_H_X86_MERGE_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, alias1, alias2, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias1, reg1, 0x1234);
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias2, reg1, 0x5678);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias2));
    EXPECT(label = rtl_alloc_label(unit));
    /* alias2 is merged to EAX on the branch path; this should be
     * detected as a conflict with alias1. */
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg2, 0, label));

    int reg4, reg5, reg6, reg7;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg5, 0, 0, alias1));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg6, 0, 0, alias2));
    /* reg5 (alias1) is no longer referenced, so reg7 gets EAX, which is
     * merged through to the next block. */
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg7, reg5, reg4, reg6));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg7, 0, alias2));

    int reg8;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg8, 0, 0, alias2));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg8, 0, alias1));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x89,0x8F,0x78,0x56,0x00,0x00,      // mov %ecx,0x5678(%rdi)
    0x85,0xC0,                          // test %eax
    0x75,0x07,                          // jnz L0
    0x8B,0xC1,                          // mov %ecx,%eax
    0xE9,0x10,0x00,0x00,0x00,           // jmp L1
    0xBA,0x04,0x00,0x00,0x00,           // L0: mov $4,%edx
    0x85,0xC9,                          // test %ecx,%ecx
    0x0F,0x44,0xC2,                     // cmovz %edx,%eax
    0x89,0x87,0x78,0x56,0x00,0x00,      // mov %eax,0x5678(%rdi)
    0x89,0x87,0x34,0x12,0x00,0x00,      // L1: mov %eax,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
