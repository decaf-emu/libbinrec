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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    /* reg2 gets EDX since reg1 is occupying ECX here. */
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    /* reg3 should get ECX since it dies at reg4's birth and thus there's
     * no collision. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    /* reg4 should lose its ECX allocation here because reg3 already has
     * ECX and the SUB instruction doesn't allow dest and src2 to overlap.
     * The register allocator shouldn't overlook this just because reg2
     * dies here and thus is the preferred output register. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SUB, reg4, reg2, reg3, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    /* The SLL here assigns ECX to reg4 during the fixed-regs scan. */
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLL, reg6, reg5, reg4, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, reg5, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0xBA,0x02,0x00,0x00,0x00,           // mov $2,%edx
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0x2B,0xD1,                          // sub %ecx,%edx
    0xB9,0x05,0x00,0x00,0x00,           // mov $5,%ecx
    0x8B,0xF1,                          // mov %ecx,%esi
    0x48,0x87,0xCA,                     // xchg %rcx,%rdx
    0xD3,0xE6,                          // shl %cl,%esi
    0x48,0x87,0xCA,                     // xchg %rcx,%rdx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
