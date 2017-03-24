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
static const unsigned int host_opt = BINREC_OPT_H_X86_CONDITION_CODES;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_SLTSI, reg2, reg1, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLTSI, reg4, reg1, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg5, reg3, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLTSI, reg6, reg5, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBB,0x01,0x00,0x00,0x00,           // mov $1,%ebx
    0x33,0xC0,                          // xor %eax,%eax
    0x48,0x85,0xDB,                     // test %rbx,%rbx
    0x0F,0x9C,0xC0,                     // setl %al
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xFF,0xD3,                          // call *%rbx
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x33,0xC9,                          // xor %ecx,%ecx
    0x48,0x85,0xDB,                     // test %rbx,%rbx
    0x0F,0x9C,0xC1,                     // setl %cl
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xFF,0xD0,                          // call *%rax
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x33,0xC9,                          // xor %ecx,%ecx
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x9C,0xC1,                     // setl %cl
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
