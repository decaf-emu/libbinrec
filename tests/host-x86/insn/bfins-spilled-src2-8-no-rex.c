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
    int dummy_regs[12], reg1, reg2, reg3, reg4;
    for (int i = 0; i < 3; i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));  // Gets ESI.
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    for (int i = 3; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));  // Spills reg2.
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[3], 0, 0));
    /* ESI and EDI are now free, so reg4 will get ESI and EDI will be
     * used as the temporary, ensuring that no REX prefix is required for
     * handling src2. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg4, reg1, reg2, 2 | 8<<8));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        if (i != 3) {
            EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
        }
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBE,0x02,0x00,0x00,0x00,           // mov $2,%esi
    0x41,0xBE,0x01,0x00,0x00,0x00,      // mov $1,%r14d
    0x89,0x34,0x24,                     // mov %esi,(%rsp)
    0xBE,0x03,0x00,0x00,0x00,           // mov $3,%esi
    0x41,0x8B,0xF6,                     // mov %r14d,%esi
    0x81,0xE6,0x03,0xFC,0xFF,0xFF,      // and $0xFFFFFC03,%esi
    /* This should not get an empty REX prefix even though src2 (originally
     * in ESI) would require one if it was not spilled. */
    0x0F,0xB6,0x3C,0x24,                // movzbl (%rsp),%edi
    0xC1,0xE7,0x02,                     // shl $2,%edi
    0x0B,0xF7,                          // or %edi,%esi
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
