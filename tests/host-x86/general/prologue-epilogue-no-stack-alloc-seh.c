/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/memory.h"
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

    /* Allocate enough RTL registers to force exactly one push, which will
     * align the stack properly. */
    alloc_dummy_registers(unit, 8, RTLTYPE_INT32);

    EXPECT(rtl_finalize_unit(unit));

    static const uint8_t expected_code[] = {
        /* Offset to code. */
        0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        /* Unwind data starts here. */
        0x01,0x01,0x01,0x00,            // UNWIND_INFO
        0x00,0x30,                      // 0x00: UWOP_PUSH_NONVOL rbx
        /* Padding to align the code. */
        0x00,0x00,
        /* Actual code starts here, 16-byte aligned. */
        0x53,                           // push %rbx
        /* Prologue ends, epilogue begins. */
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
