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
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCAST, reg4, reg2, 0, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x28,                // sub $40,%rsp
    0xEB,0x1A,                          // jmp 0x20
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (padding)
    0x00,0x00,0x00,                     // (padding)

    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)

    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x44,0x0F,0x28,0x30,                // movaps (%rax),%xmm14
    0x44,0x0F,0x29,0x74,0x24,0x10,      // movaps %xmm14,16(%rsp)
    0x44,0x0F,0x28,0x30,                // movaps (%rax),%xmm14
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x83,0x24,0x24,0xFE,                // andl $-2,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x66,0x44,0x0F,0x5A,0x74,0x24,0x10, // cvtpd2ps 16(%rsp),%xmm14
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xF6,0x04,0x24,0x01,                // testb $1,(%rsp)
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x74,0x3A,                          // jz L0
    0x45,0x0F,0x57,0xF6,                // xorps %xmm14,%xmm14
    0x66,0x44,0x0F,0xC2,0x74,0x24,0x10, // cmpunordpd 16(%rsp),%xmm14
      0x03,
    0x44,0x0F,0x28,0x7C,0x24,0x10,      // movaps 16(%rsp),%xmm15
    0x44,0x0F,0x54,0x35,0x9C,0xFF,0xFF, // andps -100(%rip),%xmm14
      0xFF,
    0x66,0x45,0x0F,0xDF,0xFE,           // pandn %xmm14,%xmm15
    0x66,0x41,0x0F,0x73,0xD7,0x1D,      // psrlq $29,%xmm15
    0x66,0x44,0x0F,0x5A,0x74,0x24,0x10, // cvtpd2ps 16(%rsp),%xmm14
    0x66,0x45,0x0F,0x70,0xFF,0xD8,      // pshufd $0xD8,%xmm15,%xmm15
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x45,0x0F,0x57,0xF7,                // xorps %xmm15,%xmm14
    0x48,0x83,0xC4,0x28,                // L0: add $40,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
