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
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg2, 0));
    /* Constant out of range. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, UINT64_C(3)<<32));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg3, 0));
    /* Negative constants allowed. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, UINT64_C(-4)));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg4, 0));
    /* Constant out of range (must be a sign-extended 32-bit value). */
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, UINT32_C(-5)));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg5, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x48,0xC7,0x07,0x02,0x00,0x00,0x00, // movq $2,(%rdi)
    0x48,0xB8,0x00,0x00,0x00,0x00,0x03, // mov $0x300000000,%rax
      0x00,0x00,0x00,
    0x48,0x89,0x07,                     // mov %rax,(%rdi)
    0x48,0xC7,0x07,0xFC,0xFF,0xFF,0xFF, // movq $-4,(%rdi)
    0xB8,0xFB,0xFF,0xFF,0xFF,           // mov $-5,%eax
    0x48,0x89,0x07,                     // mov %rax,(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 1\n"
        "[info] Killing instruction 5\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
