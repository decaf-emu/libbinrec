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
    int reg1, alias, label1, label2, label3, label4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(label3 = rtl_alloc_label(unit));
    EXPECT(label4 = rtl_alloc_label(unit));

    int reg2;
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));

    int reg3, reg4;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg4, reg3, 0, 4));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label4));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    /* The store of reg2 is carried through this block, so reg2 should
     * be kept live through here, and the ADDI in the previous block
     * should use a separate destination. */
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label3));
    /* reg2 is now live at label1, so its live range should be extended
     * through this block (which also has an edge to label1). */
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    int reg5;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label4));
    /* reg2 is now live at label3, so its live range should be extended
     * through this block (which also has an edge to label3), and reg5
     * should be placed in a different register. */
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label3));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0xE9,0x08,0x00,0x00,0x00,           // jmp L2
    0x83,0xC1,0x04,                     // L1: add $4,%ecx
    0xE9,0x0C,0x00,0x00,0x00,           // jmp L4
    0x8B,0xC8,                          // L2: mov %eax,%ecx
    0xEB,0xF4,                          // jmp L1
    0x8B,0x8F,0x34,0x12,0x00,0x00,      // L3: mov 0x1234(%rdi),%ecx
    0xEB,0xEC,                          // jmp L1
    0xB9,0x05,0x00,0x00,0x00,           // L4: mov $5,%ecx
    0xEB,0xF1,                          // jmp L3
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
