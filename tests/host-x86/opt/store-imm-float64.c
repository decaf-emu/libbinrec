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
static const unsigned int host_opt = BINREC_OPT_H_X86_STORE_IMMEDIATE;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg2, 0));
    /* Constant out of range. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg3, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg3, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x48,0xC7,0x07,0x00,0x00,0x00,0x00, // movq $0,(%rdi)
    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%rax
      0x00,0xF0,0x3F,
    0x66,0x48,0x0F,0x6E,0xC0,           // movq %rax,%xmm0
    0xF2,0x0F,0x11,0x07,                // movsd %xmm0,(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 1\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
