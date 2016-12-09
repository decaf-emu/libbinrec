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
static const unsigned int host_opt = BINREC_OPT_H_X86_BRANCH_ALIGNMENT;

static int add_rtl(RTLUnit *unit)
{
    const int target = 0xD;

    int size = 9;
    while ((size & 15) != target) {
        int reg;
        EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
        if (((target - size) & 15) <= 8 && ((target - size) & 1) == 0) {
            EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, 0));
            size += 2;
        } else {
            EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, 1));
            size += 5;
        }
    }

    int label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC0,                          // xor %eax,%eax
    0x33,0xC0,                          // xor %eax,%eax
    0xE9,0x03,0x00,0x00,0x00,           // jmp L1
    0x0F,0x1F,0x00,                     // nopl (%rax)
    0x48,0x83,0xC4,0x08,                // L1: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
