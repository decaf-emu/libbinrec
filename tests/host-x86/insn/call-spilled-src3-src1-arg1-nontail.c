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
    int dummy_regs[14];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3;
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[3], 0, 0));  // Free SI.
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[13], 0, 0));  // R14
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    for (int i = 0; i < 13; i++) {
        if (i != 3) {
            EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
        }
    }
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, reg3, reg2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBE,0x01,0x00,0x00,0x00,           // mov $1,%esi
    0x41,0xBE,0x02,0x00,0x00,0x00,      // mov $2,%r14d
    0x44,0x89,0x34,0x24,                // mov %r14d,(%rsp)
    0x41,0xBE,0x03,0x00,0x00,0x00,      // mov $3,%r14d
    0x48,0x89,0x74,0x24,0x08,           // mov %rsi,8(%rsp)
    0x41,0x8B,0xFE,                     // mov %r14d,%edi
    0x48,0x8B,0xC6,                     // mov %rsi,%rax
    0x8B,0x34,0x24,                     // mov (%rsp),%esi
    0xFF,0xD0,                          // call *%rax
    0x48,0x8B,0x74,0x24,0x08,           // mov 8(%rsp),%rsi
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
