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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS;

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg2, reg2, reg3));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg4, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0x48,0x03,0xFE,                     // add %rsi,%rdi
    0x48,0x89,0x3C,0x24,                // mov %rdi,(%rsp)
    0x48,0x87,0xF7,                     // xchg %rsi,%rdi
    0xFF,0xD7,                          // call *%rdi
    0x0F,0xAE,0x5C,0x24,0x08,           // stmxcsr 8(%rsp)
    0x83,0x64,0x24,0x08,0xC0,           // andl $-64,8(%rsp)
    0x0F,0xAE,0x54,0x24,0x08,           // ldmxcsr 8(%rsp)
    0x48,0x8B,0x3C,0x24,                // mov (%rsp),%rdi
    0x8B,0x3F,                          // mov (%rdi),%edi
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
