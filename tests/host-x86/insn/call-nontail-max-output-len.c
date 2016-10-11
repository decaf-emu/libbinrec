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
    int dummy_regs[29];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT64));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }
    alloc_dummy_registers(unit, 15, RTLTYPE_FLOAT32);

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    /* Touch XMM15 (though that shouldn't affect the output). */
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg4, reg1, reg2, reg3));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, reg4, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                                         // push %rbx
    0x55,                                         // push %rbp
    0x41,0x54,                                    // push %r12
    0x41,0x55,                                    // push %r13
    0x41,0x56,                                    // push %r14
    0x41,0x57,                                    // push %r15
    0x48,0x81,0xEC,0xD8,0x01,0x00,0x00,           // sub $472,%rsp
    0x4C,0x89,0x1C,0x24,                          // mov %r11,(%rsp)
    0x4C,0x89,0x5C,0x24,0x08,                     // mov %r11,8(%rsp)
    0x4C,0x89,0x5C,0x24,0x10,                     // mov %r11,16(%rsp)
    0x4C,0x89,0x5C,0x24,0x18,                     // mov %r11,24(%rsp)
    0x4C,0x89,0x5C,0x24,0x20,                     // mov %r11,32(%rsp)
    0x4C,0x89,0x5C,0x24,0x28,                     // mov %r11,40(%rsp)
    0x4C,0x89,0x5C,0x24,0x30,                     // mov %r11,48(%rsp)
    0x4C,0x89,0x5C,0x24,0x38,                     // mov %r11,56(%rsp)
    0x4C,0x89,0x5C,0x24,0x40,                     // mov %r11,64(%rsp)
    0x4C,0x89,0x5C,0x24,0x48,                     // mov %r11,72(%rsp)
    0x4C,0x89,0x5C,0x24,0x50,                     // mov %r11,80(%rsp)
    0x4C,0x89,0x5C,0x24,0x58,                     // mov %r11,88(%rsp)
    0x4C,0x89,0x5C,0x24,0x60,                     // mov %r11,96(%rsp)
    0x4C,0x89,0x5C,0x24,0x68,                     // mov %r11,104(%rsp)
    0x4C,0x89,0x5C,0x24,0x70,                     // mov %r11,112(%rsp)
    0x4C,0x89,0x5C,0x24,0x78,                     // mov %r11,120(%rsp)
    0x41,0xBB,0x01,0x00,0x00,0x00,                // mov $1,%r11d
    0x4C,0x89,0x9C,0x24,0x80,0x00,0x00,0x00,      // mov %r11,128(%rsp)
    0x41,0xBB,0x02,0x00,0x00,0x00,                // mov $2,%r11d
    0x4C,0x89,0x9C,0x24,0x88,0x00,0x00,0x00,      // mov %r11,136(%rsp)
    0x41,0xBB,0x03,0x00,0x00,0x00,                // mov $3,%r11d
    0x4C,0x8B,0xBC,0x24,0x80,0x00,0x00,0x00,      // mov 128(%rsp),%r15
    0xF2,0x44,0x0F,0x10,0xBC,0x24,0x88,0x00,0x00, // movsd 136(%rsp),%xmm15
      0x00,
    0xF2,0x45,0x0F,0x11,0x3F,                     // movsd %xmm15,(%r15)
    0x4C,0x89,0x9C,0x24,0xD0,0x01,0x00,0x00,      // mov %r11,464(%rsp)
    0x48,0x89,0x84,0x24,0x90,0x00,0x00,0x00,      // mov %rax,144(%rsp)
    0x48,0x89,0x8C,0x24,0x98,0x00,0x00,0x00,      // mov %rcx,152(%rsp)
    0x48,0x89,0x94,0x24,0xA0,0x00,0x00,0x00,      // mov %rdx,160(%rsp)
    0x48,0x89,0xB4,0x24,0xA8,0x00,0x00,0x00,      // mov %rsi,168(%rsp)
    0x48,0x89,0xBC,0x24,0xB0,0x00,0x00,0x00,      // mov %rdi,176(%rsp)
    0x4C,0x89,0x84,0x24,0xB8,0x00,0x00,0x00,      // mov %r8,184(%rsp)
    0x4C,0x89,0x8C,0x24,0xC0,0x00,0x00,0x00,      // mov %r9,192(%rsp)
    0x4C,0x89,0x94,0x24,0xC8,0x00,0x00,0x00,      // mov %r10,200(%rsp)
    0x4C,0x89,0x9C,0x24,0xD0,0x00,0x00,0x00,      // mov %r11,208(%rsp)
    0x0F,0x29,0x84,0x24,0xE0,0x00,0x00,0x00,      // movaps %xmm0,224(%rsp)
    0x0F,0x29,0x8C,0x24,0xF0,0x00,0x00,0x00,      // movaps %xmm1,240(%rsp)
    0x0F,0x29,0x94,0x24,0x00,0x01,0x00,0x00,      // movaps %xmm2,256(%rsp)
    0x0F,0x29,0x9C,0x24,0x10,0x01,0x00,0x00,      // movaps %xmm3,272(%rsp)
    0x0F,0x29,0xA4,0x24,0x20,0x01,0x00,0x00,      // movaps %xmm4,288(%rsp)
    0x0F,0x29,0xAC,0x24,0x30,0x01,0x00,0x00,      // movaps %xmm5,304(%rsp)
    0x0F,0x29,0xB4,0x24,0x40,0x01,0x00,0x00,      // movaps %xmm6,320(%rsp)
    0x0F,0x29,0xBC,0x24,0x50,0x01,0x00,0x00,      // movaps %xmm7,336(%rsp)
    0x44,0x0F,0x29,0x84,0x24,0x60,0x01,0x00,0x00, // movaps %xmm8,352(%rsp)
    0x44,0x0F,0x29,0x8C,0x24,0x70,0x01,0x00,0x00, // movaps %xmm9,368(%rsp)
    0x44,0x0F,0x29,0x94,0x24,0x80,0x01,0x00,0x00, // movaps %xmm10,384(%rsp)
    0x44,0x0F,0x29,0x9C,0x24,0x90,0x01,0x00,0x00, // movaps %xmm11,400(%rsp)
    0x44,0x0F,0x29,0xA4,0x24,0xA0,0x01,0x00,0x00, // movaps %xmm12,416(%rsp)
    0x44,0x0F,0x29,0xAC,0x24,0xB0,0x01,0x00,0x00, // movaps %xmm13,432(%rsp)
    0x44,0x0F,0x29,0xB4,0x24,0xC0,0x01,0x00,0x00, // movaps %xmm14,448(%rsp)
    0x48,0x8B,0xBC,0x24,0x88,0x00,0x00,0x00,      // mov 136(%rsp),%rdi
    0x48,0x8B,0xB4,0x24,0xD0,0x01,0x00,0x00,      // mov 464(%rsp),%rsi
    0x4C,0x8B,0xBC,0x24,0x80,0x00,0x00,0x00,      // mov 128(%rsp),%r15
    0x41,0xFF,0xD7,                               // call *%r15
    0x4C,0x8B,0xD8,                               // mov %rax,%r11
    0x48,0x8B,0x84,0x24,0x90,0x00,0x00,0x00,      // mov 144(%rsp),%rax
    0x48,0x8B,0x8C,0x24,0x98,0x00,0x00,0x00,      // mov 152(%rsp),%rcx
    0x48,0x8B,0x94,0x24,0xA0,0x00,0x00,0x00,      // mov 160(%rsp),%rdx
    0x48,0x8B,0xB4,0x24,0xA8,0x00,0x00,0x00,      // mov 168(%rsp),%rsi
    0x48,0x8B,0xBC,0x24,0xB0,0x00,0x00,0x00,      // mov 176(%rsp),%rdi
    0x4C,0x8B,0x84,0x24,0xB8,0x00,0x00,0x00,      // mov 184(%rsp),%r8
    0x4C,0x8B,0x8C,0x24,0xC0,0x00,0x00,0x00,      // mov 192(%rsp),%r9
    0x4C,0x8B,0x94,0x24,0xC8,0x00,0x00,0x00,      // mov 200(%rsp),%r10
    0x4C,0x8B,0x9C,0x24,0xD0,0x00,0x00,0x00,      // mov 208(%rsp),%r11
    0x0F,0x28,0x84,0x24,0xE0,0x00,0x00,0x00,      // movaps 224(%rsp),%xmm0
    0x0F,0x28,0x8C,0x24,0xF0,0x00,0x00,0x00,      // movaps 240(%rsp),%xmm1
    0x0F,0x28,0x94,0x24,0x00,0x01,0x00,0x00,      // movaps 256(%rsp),%xmm2
    0x0F,0x28,0x9C,0x24,0x10,0x01,0x00,0x00,      // movaps 272(%rsp),%xmm3
    0x0F,0x28,0xA4,0x24,0x20,0x01,0x00,0x00,      // movaps 288(%rsp),%xmm4
    0x0F,0x28,0xAC,0x24,0x30,0x01,0x00,0x00,      // movaps 304(%rsp),%xmm5
    0x0F,0x28,0xB4,0x24,0x40,0x01,0x00,0x00,      // movaps 320(%rsp),%xmm6
    0x0F,0x28,0xBC,0x24,0x50,0x01,0x00,0x00,      // movaps 336(%rsp),%xmm7
    0x44,0x0F,0x28,0x84,0x24,0x60,0x01,0x00,0x00, // movaps 352(%rsp),%xmm8
    0x44,0x0F,0x28,0x8C,0x24,0x70,0x01,0x00,0x00, // movaps 368(%rsp),%xmm9
    0x44,0x0F,0x28,0x94,0x24,0x80,0x01,0x00,0x00, // movaps 384(%rsp),%xmm10
    0x44,0x0F,0x28,0x9C,0x24,0x90,0x01,0x00,0x00, // movaps 400(%rsp),%xmm11
    0x44,0x0F,0x28,0xA4,0x24,0xA0,0x01,0x00,0x00, // movaps 416(%rsp),%xmm12
    0x44,0x0F,0x28,0xAC,0x24,0xB0,0x01,0x00,0x00, // movaps 432(%rsp),%xmm13
    0x44,0x0F,0x28,0xB4,0x24,0xC0,0x01,0x00,0x00, // movaps 448(%rsp),%xmm14
    0x48,0x81,0xC4,0xD8,0x01,0x00,0x00,           // add $472,%rsp
    0x41,0x5F,                                    // pop %r15
    0x41,0x5E,                                    // pop %r14
    0x41,0x5D,                                    // pop %r13
    0x41,0x5C,                                    // pop %r12
    0x5D,                                         // pop %rbp
    0x5B,                                         // pop %rbx
    0xC3,                                         // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
