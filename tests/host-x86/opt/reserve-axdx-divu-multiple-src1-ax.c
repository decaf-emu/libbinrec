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
static const unsigned int host_opt = BINREC_OPT_H_X86_FIXED_REGS;

static int add_rtl(RTLUnit *unit)
{
    uint32_t reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));  // reg1=ECX
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));  // reg2=EDX
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg3, reg1, reg2, 0));  // reg3=EAX

    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg3 dying here (with EAX) should not keep reg4 from getting EAX. */
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg4, reg3, reg1, 0));  // reg4=EAX

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC9,                          // xor %ecx,%ecx
    0x33,0xD2,                          // xor %edx,%edx
    0x48,0x8B,0xF2,                     // mov %rdx,%rsi
    0x8B,0xC1,                          // mov %ecx,%eax
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF6,                          // div %esi
    0x48,0x8B,0xD6,                     // mov %rsi,%rdx
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF1,                          // div %ecx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
