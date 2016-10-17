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
    int label;
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    for (int i = 0; i < 128; i += 7) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, i+7));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x0F,0x1F,0x05,0x07,0x00,0x00,0x00, // L1: nopl 0x7(%rip)
    0x0F,0x1F,0x05,0x0E,0x00,0x00,0x00, // nopl 0xE(%rip)
    0x0F,0x1F,0x05,0x15,0x00,0x00,0x00, // nopl 0x15(%rip)
    0x0F,0x1F,0x05,0x1C,0x00,0x00,0x00, // nopl 0x1C(%rip)
    0x0F,0x1F,0x05,0x23,0x00,0x00,0x00, // nopl 0x23(%rip)
    0x0F,0x1F,0x05,0x2A,0x00,0x00,0x00, // nopl 0x2A(%rip)
    0x0F,0x1F,0x05,0x31,0x00,0x00,0x00, // nopl 0x31(%rip)
    0x0F,0x1F,0x05,0x38,0x00,0x00,0x00, // nopl 0x38(%rip)
    0x0F,0x1F,0x05,0x3F,0x00,0x00,0x00, // nopl 0x3F(%rip)
    0x0F,0x1F,0x05,0x46,0x00,0x00,0x00, // nopl 0x46(%rip)
    0x0F,0x1F,0x05,0x4D,0x00,0x00,0x00, // nopl 0x4D(%rip)
    0x0F,0x1F,0x05,0x54,0x00,0x00,0x00, // nopl 0x54(%rip)
    0x0F,0x1F,0x05,0x5B,0x00,0x00,0x00, // nopl 0x5B(%rip)
    0x0F,0x1F,0x05,0x62,0x00,0x00,0x00, // nopl 0x62(%rip)
    0x0F,0x1F,0x05,0x69,0x00,0x00,0x00, // nopl 0x69(%rip)
    0x0F,0x1F,0x05,0x70,0x00,0x00,0x00, // nopl 0x70(%rip)
    0x0F,0x1F,0x05,0x77,0x00,0x00,0x00, // nopl 0x77(%rip)
    0x0F,0x1F,0x05,0x7E,0x00,0x00,0x00, // nopl 0x7E(%rip)
    0x0F,0x1F,0x05,0x85,0x00,0x00,0x00, // nopl 0x85(%rip)
    0xE9,0x76,0xFF,0xFF,0xFF,           // jmp L1
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
