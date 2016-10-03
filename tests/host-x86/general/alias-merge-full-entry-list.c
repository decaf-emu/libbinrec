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
    int reg1, alias1, alias2, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias1, reg1, 0x1234);
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias2, reg1, 0x5678);
    EXPECT(label = rtl_alloc_label(unit));

    for (int i = 1; i <= 8; i++) {
        int regA, regB;
        EXPECT(regA = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, regA, 0, 0, alias1));
        EXPECT(regB = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_ADDI, regB, regA, 0, -1));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, regB, 0, alias1));
        EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, regB, 0, label));
    }

    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    int reg2, reg3, reg4, reg5;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLLI, reg3, reg2, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias1));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg4, 0, 0, alias2));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg5, reg3, reg4, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg5, 0, alias2));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x58,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x4D,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x42,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x37,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x2C,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x21,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x16,0x00,0x00,0x00,      // jz L1
    0x83,0xC0,0xFF,                     // add $-1,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x05,0x00,0x00,0x00,      // jz L1
    0xE9,0x17,0x00,0x00,0x00,           // jmp L0
    0xC1,0xE0,0x01,                     // L1: shl $1,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x8B,0x8F,0x78,0x56,0x00,0x00,      // mov 0x5678(%rdi),%ecx
    0x03,0xC1,                          // add %ecx,%eax
    0x89,0x87,0x78,0x56,0x00,0x00,      // mov %eax,0x5678(%rdi)
    0x48,0x83,0xC4,0x08,                // L0: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
