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
    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Default allocation rules would put reg4 in ECX (avoiding EAX),
     * but that would clobber src2 when src1 is moved out of RAX, so the
     * allocator should avoid that case. */
    EXPECT(rtl_add_insn(unit, RTLOP_CMPXCHG, reg4, reg1, reg2, reg3));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    /* R15 is saved as a temporary even though it ends up not being used. */
    0x41,0x57,                          // push %r15
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0xBA,0x03,0x00,0x00,0x00,           // mov $3,%edx
    0x48,0x8B,0xF0,                     // mov %rax,%rsi
    0x8B,0xC1,                          // mov %ecx,%eax
    0xF0,0x0F,0xB1,0x16,                // lock cmpxchg %edx,(%rsi)
    0x8B,0xF0,                          // mov %eax,%esi
    0x41,0x5F,                          // pop %r15
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
