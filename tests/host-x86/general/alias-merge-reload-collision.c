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
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, alias1, alias2, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias1, reg1, 0x1234);
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias2, reg1, 0x5678);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias2));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg4, reg2, reg3, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias2));
    EXPECT(label = rtl_alloc_label(unit));
    /* alias1 will merge to ECX after the label since reg7 takes EAX.
     * The code generator should detect the conflict with the merge of
     * alias2 in ECX on the fall-through code path. */
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg4, 0, label));

    int reg5, reg6;
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg5, 0, 0, alias2));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg6, reg5, 0, 6));

    int reg7, reg8, reg9;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg7, 0, 0, 7));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg8, 0, 0, alias1));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg9, reg7, reg8, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg9, 0, alias1));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0x8B,0x8F,0x78,0x56,0x00,0x00,      // mov 0x5678(%rdi),%ecx
    0x8B,0xD0,                          // mov %eax,%edx
    0x03,0xD1,                          // add %ecx,%edx
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x89,0x8F,0x78,0x56,0x00,0x00,      // mov %ecx,0x5678(%rdi)
    0x85,0xD2,                          // test %edx,%edx
    0x75,0x07,                          // jnz L0
    0x8B,0xC8,                          // mov %eax,%ecx
    0xE9,0x05,0x00,0x00,0x00,           // jmp L1
    0x83,0xC1,0x06,                     // L0: add $6,%ecx
    0x8B,0xC8,                          // mov %eax,%ecx
    0xB8,0x07,0x00,0x00,0x00,           // L1: mov $7,%eax
    0x03,0xC1,                          // add %ecx,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
