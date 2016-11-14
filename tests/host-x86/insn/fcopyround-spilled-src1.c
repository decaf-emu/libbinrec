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
    int dummy_regs[12];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETROUND,
                        reg2, reg1, 0, RTLFROUND_NEAREST));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FCOPYROUND, reg3, reg1, reg2, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x44,0x8B,0x2C,0x24,                // mov (%rsp),%r13d
    0x45,0x8B,0xF5,                     // mov %r13d,%r14d
    0x41,0x81,0xE6,0xFF,0x9F,0x00,0x00, // and $0x9FFF,%r14d
    0x44,0x89,0x6C,0x24,0x04,           // mov %r13d,4(%rsp)
    0x44,0x8B,0x6C,0x24,0x04,           // mov 4(%rsp),%r13d
    0x41,0x81,0xE5,0xFF,0x9F,0x00,0x00, // and $0x9FFF,%r13d
    0x45,0x8B,0xFE,                     // mov %r14d,%r15d
    0x41,0x81,0xE7,0x00,0x60,0x00,0x00, // and $0x6000,%r15d
    0x45,0x0B,0xEF,                     // or %r15d,%r13d
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0x41,0x5F,                          // pop %r15
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"