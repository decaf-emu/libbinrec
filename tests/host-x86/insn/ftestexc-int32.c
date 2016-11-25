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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC, reg2, reg1, 0, RTLFEXC_ANY));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC, reg3, reg1, 0, RTLFEXC_INEXACT));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC, reg4, reg1, 0, RTLFEXC_INVALID));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg5, reg1, 0, RTLFEXC_OVERFLOW));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg6, reg1, 0, RTLFEXC_UNDERFLOW));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg7, reg1, 0, RTLFEXC_ZERO_DIVIDE));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0x33,0xD2,                          // xor %edx,%edx
    0xF6,0xC1,0x3D,                     // test $0x3D,%cl
    0x0F,0x95,0xC2,                     // setnz %dl
    0x33,0xD2,                          // xor %edx,%edx
    0xF6,0xC1,0x20,                     // test $0x20,%cl
    0x0F,0x95,0xC2,                     // setnz %dl
    0x8B,0xD1,                          // mov %ecx,%edx
    0x83,0xE2,0x01,                     // and $0x01,%edx
    0x33,0xD2,                          // xor %edx,%edx
    0xF6,0xC1,0x08,                     // test $0x08,%cl
    0x0F,0x95,0xC2,                     // setnz %dl
    0x33,0xD2,                          // xor %edx,%edx
    0xF6,0xC1,0x10,                     // test $0x10,%cl
    0x0F,0x95,0xC2,                     // setnz %dl
    0x33,0xD2,                          // xor %edx,%edx
    0xF6,0xC1,0x04,                     // test $0x04,%cl
    0x0F,0x95,0xC2,                     // setnz %dl
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
