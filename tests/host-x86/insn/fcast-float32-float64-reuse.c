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

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    /* reg2 does not reuse XMM1 even though reg1 dies here because the
     * input and output registers need to be separate. */
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg2, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xEB,0x2A,                          // jmp 0x30
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (padding)
    0x00,0x00,0x00,                     // (padding)

    0xFF,0xFF,0xBF,0xFF,0x00,0x00,0x00,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)

    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%rax
      0x00,0xF0,0x3F,
    0x66,0x48,0x0F,0x6E,0xC8,           // movq %rax,%xmm1
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x66,0x0F,0x2E,0xC9,                // ucomisd %xmm1,%xmm1
    0xF2,0x0F,0x5A,0xD1,                // cvtsd2ss %xmm1,%xmm2
    0x7B,0x15,                          // jnp L0
    0x66,0x48,0x0F,0x7E,0xC8,           // movq %xmm1,%rax
    0x48,0x85,0x05,0xC7,0xFF,0xFF,0xFF, // test %rax,-57(%rip)
    0x75,0x07,                          // jnz L0
    0x0F,0x54,0x15,0xAE,0xFF,0xFF,0xFF, // andps -82(%rip),%xmm2
    0x0F,0xAE,0x14,0x24,                // L0: ldmxcsr (%rsp)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
