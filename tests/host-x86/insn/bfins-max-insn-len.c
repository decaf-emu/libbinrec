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
    int dummy_xmm[23];
    for (int i = 0; i < lenof(dummy_xmm); i++) {
        EXPECT(dummy_xmm[i] = rtl_alloc_register(unit, RTLTYPE_V2_DOUBLE));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_xmm[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(dummy_xmm); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_xmm[i], 0, 0));
    }

    int dummy_regs[12];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg4, reg1, reg2, 2 | 48<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x81,0xEC,0x98,0x00,0x00,0x00, // sub $152,%rsp
    0x44,0x0F,0x29,0x34,0x24,           // movaps %xmm14,(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x10,      // movaps %xmm14,16(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x20,      // movaps %xmm14,32(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x30,      // movaps %xmm14,48(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x40,      // movaps %xmm14,64(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x50,      // movaps %xmm14,80(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x60,      // movaps %xmm14,96(%rsp)
    0x44,0x0F,0x29,0x74,0x24,0x70,      // movaps %xmm14,112(%rsp)
    0x45,0x33,0xED,                     // xor %r13d,%r13d
    0x45,0x33,0xF6,                     // xor %r14d,%r14d
    0x4C,0x89,0xB4,0x24,0x80,0x00,0x00, // mov %r14,128(%rsp)
      0x00,
    0x45,0x33,0xF6,                     // xor %r14d,%r14d
    0x4C,0x89,0xAC,0x24,0x88,0x00,0x00, // mov %r13,136(%rsp)
      0x00,
    0x49,0xBD,0x03,0x00,0x00,0x00,0x00, // mov $0xFFFC000000000003,%r13
      0x00,0xFC,0xFF,
    0x4C,0x23,0xAC,0x24,0x88,0x00,0x00, // and 136(%rsp),%r13
      0x00,
    0x4C,0x8B,0xBC,0x24,0x80,0x00,0x00, // mov 128(%rsp),%r15
      0x00,
    0x49,0xC1,0xE7,0x10,                // shl $16,%r15
    0x49,0xC1,0xEF,0x0E,                // shr $14,%r15
    0x4D,0x0B,0xEF,                     // or %r15,%r13
    0x48,0x81,0xC4,0x98,0x00,0x00,0x00, // add $152,%rsp
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
