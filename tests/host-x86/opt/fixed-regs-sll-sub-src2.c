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

    int reg1, reg2, reg3, reg4;
    /* reg1 should get EDX because it overlaps reg3, which at this point
     * still has ECX reserved. */
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    /* reg2 should get ECX since it dies at reg3's birth and thus there's
     * no collision. */
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    /* reg3 should lose its ECX allocation here because reg2 already has
     * ECX and the SUB instruction doesn't allow dest and src2 to overlap. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SUB, reg3, reg1, reg2, 0));
    /* The SLL here assigns ECX to reg3 during the fixed-regs scan. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLL, reg4, reg1, reg3, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg3, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xBA,0x01,0x00,0x00,0x00,           // mov $1,%edx
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0x8B,0xF2,                          // mov %edx,%esi
    0x2B,0xF1,                          // sub %ecx,%esi
    0x8B,0xFA,                          // mov %edx,%edi
    0x8B,0xCE,                          // mov %esi,%ecx
    0xD3,0xE7,                          // shl %cl,%edi
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
