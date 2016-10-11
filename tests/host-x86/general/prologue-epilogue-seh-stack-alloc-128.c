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


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.host = BINREC_ARCH_X86_64_WINDOWS_SEH;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    alloc_dummy_registers(unit, 14, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 14, RTLTYPE_FLOAT32);

    EXPECT(rtl_finalize_unit(unit));

    static const uint8_t expected_code[] = {
        /* Offset to code. */
        0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        /* Unwind data starts here. */
        0x01,0x3E,0x18,0x00,            // UNWIND_INFO
        0x38,0xD8,0x07,0x00,            // 0x3A: UWOP_SAVE_XMM128 xmm13,7
        0x32,0xC8,0x06,0x00,            // 0x34: UWOP_SAVE_XMM128 xmm12,6
        0x2C,0xB8,0x05,0x00,            // 0x2E: UWOP_SAVE_XMM128 xmm11,5
        0x26,0xA8,0x04,0x00,            // 0x28: UWOP_SAVE_XMM128 xmm10,4
        0x20,0x98,0x03,0x00,            // 0x22: UWOP_SAVE_XMM128 xmm9,3
        0x1A,0x88,0x02,0x00,            // 0x1C: UWOP_SAVE_XMM128 xmm8,2
        0x15,0x78,0x01,0x00,            // 0x17: UWOP_SAVE_XMM128 xmm7,1
        0x11,0x68,0x00,0x00,            // 0x13: UWOP_SAVE_XMM128 xmm6,0
        0x0A,0xF2,                      // 0x0A: UWOP_ALLOC_SMALL 15
        0x08,0xE0,                      // 0x08: UWOP_PUSH_NONVOL r14
        0x06,0xD0,                      // 0x06: UWOP_PUSH_NONVOL r13
        0x04,0xC0,                      // 0x04: UWOP_PUSH_NONVOL r12
        0x03,0x70,                      // 0x03: UWOP_PUSH_NONVOL rdi
        0x02,0x60,                      // 0x02: UWOP_PUSH_NONVOL rsi
        0x01,0x50,                      // 0x01: UWOP_PUSH_NONVOL rbp
        0x00,0x30,                      // 0x00: UWOP_PUSH_NONVOL rbx
        /* Padding to align the code. */
        0x00,0x00,0x00,0x00,
        /* Actual code starts here, 16-byte aligned. */
        0x53,                           // push %rbx
        0x55,                           // push %rbp
        0x56,                           // push %rsi
        0x57,                           // push %rdi
        0x41,0x54,                      // push %r12
        0x41,0x55,                      // push %r13
        0x41,0x56,                      // push %r14
        0x48,0x81,0xEC,0x80,0x00,0x00,  // sub $128,%rsp
          0x00,
        0x0F,0x29,0x34,0x24,            // movaps %xmm6,(%rsp)
        0x0F,0x29,0x7C,0x24,0x10,       // movaps %xmm7,16(%rsp)
        0x44,0x0F,0x29,0x44,0x24,0x20,  // movaps %xmm8,32(%rsp)
        0x44,0x0F,0x29,0x4C,0x24,0x30,  // movaps %xmm9,48(%rsp)
        0x44,0x0F,0x29,0x54,0x24,0x40,  // movaps %xmm10,64(%rsp)
        0x44,0x0F,0x29,0x5C,0x24,0x50,  // movaps %xmm11,80(%rsp)
        0x44,0x0F,0x29,0x64,0x24,0x60,  // movaps %xmm12,96(%rsp)
        0x44,0x0F,0x29,0x6C,0x24,0x70,  // movaps %xmm13,112(%rsp)
        /* End of prologue, beginning of epilogue. */
        0x44,0x0F,0x28,0x6C,0x24,0x70,  // movaps %xmm13,112(%rsp)
        0x44,0x0F,0x28,0x64,0x24,0x60,  // movaps %xmm12,96(%rsp)
        0x44,0x0F,0x28,0x5C,0x24,0x50,  // movaps %xmm11,80(%rsp)
        0x44,0x0F,0x28,0x54,0x24,0x40,  // movaps %xmm10,64(%rsp)
        0x44,0x0F,0x28,0x4C,0x24,0x30,  // movaps %xmm9,48(%rsp)
        0x44,0x0F,0x28,0x44,0x24,0x20,  // movaps %xmm8,32(%rsp)
        0x0F,0x28,0x7C,0x24,0x10,       // movaps %xmm7,16(%rsp)
        0x0F,0x28,0x34,0x24,            // movaps %xmm6,(%rsp)
        0x48,0x81,0xC4,0x80,0x00,0x00,  // add $128,%rsp
          0x00,
        0x41,0x5E,                      // pop %r14
        0x41,0x5D,                      // pop %r13
        0x41,0x5C,                      // pop %r12
        0x5F,                           // pop %rdi
        0x5E,                           // pop %rsi
        0x5D,                           // pop %rbp
        0x5B,                           // pop %rbx
        0xC3,                           // ret
    };

    handle->code_buffer_size = sizeof(expected_code);
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));
    EXPECT(host_x86_translate(handle, unit));
    uint8_t *code = handle->code_buffer;
    long code_len = handle->code_len;
    handle->code_buffer = NULL;

    EXPECT_MEMEQ(code, expected_code, sizeof(expected_code));
    EXPECT_EQ(code_len, sizeof(expected_code));

    binrec_code_free(handle, code);
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
