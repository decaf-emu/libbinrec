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
    alloc_dummy_registers(unit, 1, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    /* reg3 should not reuse reg2. */
    EXPECT(rtl_add_insn(unit, RTLOP_VFCAST, reg3, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xEB,0x1A,                          // jmp 0x20
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (padding)
    0x00,0x00,0x00,                     // (padding)

    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)

    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x0F,0x28,0x08,                     // movaps (%rax),%xmm1
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x83,0x24,0x24,0xFE,                // andl $-2,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x66,0x0F,0x5A,0xD1,                // cvtpd2ps %xmm1,%xmm2
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xF6,0x04,0x24,0x01,                // testb $1,(%rsp)
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x74,0x2A,                          // jz L0
    0x0F,0x57,0xD2,                     // xorps %xmm2,%xmm2
    0x66,0x0F,0xC2,0xD1,0x03,           // cmpunordpd %xmm1,%xmm2
    0x0F,0x28,0xD9,                     // movaps %xmm1,%xmm3
    0x0F,0x54,0x15,0xB2,0xFF,0xFF,0xFF, // andps -78(%rip),%xmm2
    0x0F,0x55,0xDA,                     // andnps %xmm2,%xmm3
    0x66,0x0F,0x73,0xD3,0x1D,           // psrlq $29,%xmm3
    0x66,0x0F,0x5A,0xD1,                // cvtpd2ps %xmm1,%xmm2
    0x66,0x0F,0x70,0xDB,0xD8,           // pshufd $0xD8,%xmm3,%xmm3
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x0F,0x57,0xD3,                     // xorps %xmm3,%xmm2
    0x48,0x83,0xC4,0x08,                // L0: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
