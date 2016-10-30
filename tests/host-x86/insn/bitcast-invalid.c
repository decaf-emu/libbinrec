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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 2, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, reg1, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_BITCAST, reg3, reg2, 0, 0));
    unit->insns[unit->num_insns-1].src1 = reg1;
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0xBA,0x00,0x00,0x00,0x40,           // mov $0x40000000,%edx
    0x66,0x0F,0x6E,0xD2,                // movd %edx,%xmm2
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    "[error] Invalid data type fpstate in BITCAST at 5\n";

#include "tests/rtl-translate-test.i"
