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
#include "tests/mem-wrappers.h"


static void *code_malloc_fail(UNUSED void *userdata, UNUSED size_t size,
                              UNUSED size_t alignment) {
    return NULL;
}

static void *code_realloc_fail(UNUSED void *userdata, UNUSED void *ptr,
                               UNUSED size_t old_size, UNUSED size_t new_size,
                               size_t alignment) {
    return NULL;
}

int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.host = BINREC_ARCH_X86_64_WINDOWS_SEH;
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    /* Allocate enough RTL registers to use up all available GPRs. */
    alloc_dummy_registers(unit, 14, RTLTYPE_INT32);

    /* Add some NOPs to pad the code stream, so the entire buffer doesn't
     * fit in the space reserved for the prologue. */
    for (int i = 1; i <= 15; i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, i));
    }

    EXPECT(rtl_finalize_unit(unit));

    static const uint8_t expected_code[] = {
        /* Offset to code. */
        0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        /* Unwind data starts here. */
        0x01,0x0A,0x07,0x00,            // UNWIND_INFO
        0x08,0xE0,                      // 0x08: UWOP_PUSH_NONVOL r14
        0x06,0xD0,                      // 0x06: UWOP_PUSH_NONVOL r13
        0x04,0xC0,                      // 0x04: UWOP_PUSH_NONVOL r12
        0x03,0x70,                      // 0x03: UWOP_PUSH_NONVOL rdi
        0x02,0x60,                      // 0x02: UWOP_PUSH_NONVOL rsi
        0x01,0x50,                      // 0x01: UWOP_PUSH_NONVOL rbp
        0x00,0x30,                      // 0x00: UWOP_PUSH_NONVOL rbx
        /* Padding to align the code. */
        0x00,0x00,0x00,0x00,0x00,0x00,
        /* Actual code starts here, 16-byte aligned. */
        0x53,                           // push %rbx
        0x55,                           // push %rbp
        0x56,                           // push %rsi
        0x57,                           // push %rdi
        0x41,0x54,                      // push %r12
        0x41,0x55,                      // push %r13
        0x41,0x56,                      // push %r14
        0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nopl 1(%rip)
        0x0F,0x1F,0x05,0x02,0x00,0x00,0x00, // nopl 2(%rip)
        0x0F,0x1F,0x05,0x03,0x00,0x00,0x00, // nopl 3(%rip)
        0x0F,0x1F,0x05,0x04,0x00,0x00,0x00, // nopl 4(%rip)
        0x0F,0x1F,0x05,0x05,0x00,0x00,0x00, // nopl 5(%rip)
        0x0F,0x1F,0x05,0x06,0x00,0x00,0x00, // nopl 6(%rip)
        0x0F,0x1F,0x05,0x07,0x00,0x00,0x00, // nopl 7(%rip)
        0x0F,0x1F,0x05,0x08,0x00,0x00,0x00, // nopl 8(%rip)
        0x0F,0x1F,0x05,0x09,0x00,0x00,0x00, // nopl 9(%rip)
        0x0F,0x1F,0x05,0x0A,0x00,0x00,0x00, // nopl 10(%rip)
        0x0F,0x1F,0x05,0x0B,0x00,0x00,0x00, // nopl 11(%rip)
        0x0F,0x1F,0x05,0x0C,0x00,0x00,0x00, // nopl 12(%rip)
        0x0F,0x1F,0x05,0x0D,0x00,0x00,0x00, // nopl 13(%rip)
        0x0F,0x1F,0x05,0x0E,0x00,0x00,0x00, // nopl 14(%rip)
        0x0F,0x1F,0x05,0x0F,0x00,0x00,0x00, // nopl 15(%rip)
        0x41,0x5E,                      // pop %r14
        0x41,0x5D,                      // pop %r13
        0x41,0x5C,                      // pop %r12
        0x5F,                           // pop %rdi
        0x5E,                           // pop %rsi
        0x5D,                           // pop %rbp
        0x5B,                           // pop %rbx
        0xC3,                           // ret
    };

    handle->code_alignment = 16;

    /* Check failures on setting up internal data. */
    for (int count = 0; ; count++) {
        handle->code_buffer_size = sizeof(expected_code);
        EXPECT(handle->code_buffer = binrec_code_malloc(
                   handle, handle->code_buffer_size, handle->code_alignment));
        handle->code_len = 0;
        if (count >= 100) {
            FAIL("Failed to translate unit after 100 tries");
        }
        mem_wrap_fail_after(count);
        const bool result = host_x86_translate(handle, unit);
        mem_wrap_cancel_fail();
        binrec_code_free(handle, handle->code_buffer);
        handle->code_buffer = NULL;
        handle->code_buffer_size = 0;
        if (result) {
            if (count == 0) {
                FAIL("Translation did not fail on memory allocation failure");
            }
            break;
        }
    }

    /* Check failures on expanding code buffer. */
    int limit = sizeof(expected_code) + 1000;
    for (int size = 1; size <= limit; size++) {
        handle->code_buffer_size = size;
        EXPECT(handle->code_buffer = binrec_code_malloc(
                   handle, handle->code_buffer_size, handle->code_alignment));
        handle->code_len = 0;
        handle->setup.code_malloc = code_malloc_fail;
        handle->setup.code_realloc = code_realloc_fail;
        const bool result = host_x86_translate(handle, unit);
        handle->setup.code_malloc = NULL;
        handle->setup.code_realloc = NULL;
        if (result) {
            if (size < (int)sizeof(expected_code)) {
                FAIL("Translation did not fail with buffer size %d", size);
            }
            break;
        } else {
            binrec_code_free(handle, handle->code_buffer);
            handle->code_buffer = NULL;
            handle->code_buffer_size = 0;
            if (size == limit) {
                FAIL("Translation failed with sufficiently large buffer");
            }
        }
    }

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
