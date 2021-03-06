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
static const unsigned int host_opt = BINREC_OPT_H_X86_FORWARD_CONDITIONS;

static int add_rtl(RTLUnit *unit)
{
    int label, reg1, reg2, reg3;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg3, reg1, reg2, RTLFCMP_EQ));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg3, 0, label));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x00,0x00,0x80,0x3F,           // L1: mov $0x3F800000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0xB8,0x00,0x00,0x00,0x40,           // mov $0x40000000,%eax
    0x66,0x0F,0x6E,0xC8,                // movd %eax,%xmm1
    /* RTLFCMP_EQ can't be optimized since it requires a 2-step test. */
    0x33,0xC0,                          // xor %eax,%eax
    0x0F,0x2E,0xC1,                     // ucomiss %xmm1,%xmm0
    0x7A,0x03,                          // jp L0
    0x0F,0x94,0xC0,                     // setz %al
    0x85,0xC0,                          // L0: test %eax,%eax
    0x75,0xE0,                          // jnz L1
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
