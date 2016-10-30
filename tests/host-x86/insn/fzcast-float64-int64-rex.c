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
    alloc_dummy_registers(unit, 5, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 10, RTLTYPE_FLOAT32);

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_FZCAST, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x41,0xB8,0x01,0x00,0x00,0x00,      // mov $1,%r8d
    0x4D,0x85,0xC0,                     // test %r8,%r8
    0x78,0x07,                          // js L1
    0xF2,0x4D,0x0F,0x2A,0xD0,           // cvtsi2sd %r8,%xmm10
    0xEB,0x2C,                          // jmp L4
    0x4D,0x8B,0xC8,                     // L1: mov %r8,%r9
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x49,0xD1,0xE9,                     // shr %r9
    0x0F,0xBA,0x24,0x24,0x0D,           // btl $13,(%rsp)
    0x73,0x11,                          // jnc L3
    0x0F,0xBA,0x24,0x24,0x0E,           // btl $14,(%rsp)
    0x73,0x06,                          // jnc L2
    0x41,0xF6,0xC1,0x01,                // test $1,%r9b
    0x74,0x04,                          // jz L3
    0x49,0x83,0xC1,0x01,                // L2: add $1,%r9
    0xF2,0x4D,0x0F,0x2A,0xD1,           // L3: cvtsi2sd %r9,%xmm10
    0xF2,0x45,0x0F,0x58,0xD2,           // addsd %xmm10,%xmm10
    0x48,0x83,0xC4,0x08,                // L4: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
