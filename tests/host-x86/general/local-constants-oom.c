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
    setup.host = BINREC_ARCH_X86_64_WINDOWS;
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    /* Allocate dummy floating-point aliases to push the XMM saves into
     * 32-bit stack displacements. */
    for (int i = 0; i < 16; i++) {
        EXPECT(rtl_alloc_alias_register(unit, RTLTYPE_FLOAT64));
    }

    /* Allocate enough RTL registers to use up all available GPRs and
     * most XMM registers. */
    alloc_dummy_registers(unit, 14, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 14, RTLTYPE_FLOAT32);

    /* Add an FNEG instruction to trigger local constant insertion. */
    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FNEG, reg2, reg1, 0, 0));

    EXPECT(rtl_finalize_unit(unit));

    static const uint8_t expected_code[] = {
        0x53,                           // push %rbx
        0x55,                           // push %rbp
        0x56,                           // push %rsi
        0x57,                           // push %rdi
        0x41,0x54,                      // push %r12
        0x41,0x55,                      // push %r13
        0x41,0x56,                      // push %r14
        0x41,0x57,                      // push %r15
        0x48,0x81,0xEC,0x18,0x01,0x00,  // sub $280,%rsp
          0x00,
        0x0F,0x29,0xB4,0x24,0x80,0x00,  // movaps %xmm6,128(%rsp)
          0x00,0x00,
        0x0F,0x29,0xBC,0x24,0x90,0x00,  // movaps %xmm7,144(%rsp)
          0x00,0x00,
        0x44,0x0F,0x29,0x84,0x24,0xA0,  // movaps %xmm8,160(%rsp)
          0x00,0x00,0x00,
        0x44,0x0F,0x29,0x8C,0x24,0xB0,  // movaps %xmm9,176(%rsp)
          0x00,0x00,0x00,
        0x44,0x0F,0x29,0x94,0x24,0xC0,  // movaps %xmm10,192(%rsp)
          0x00,0x00,0x00,
        0x44,0x0F,0x29,0x9C,0x24,0xD0,  // movaps %xmm11,208(%rsp)
          0x00,0x00,0x00,
        0x44,0x0F,0x29,0xA4,0x24,0xE0,  // movaps %xmm12,224(%rsp)
          0x00,0x00,0x00,
        0x44,0x0F,0x29,0xAC,0x24,0xF0,  // movaps %xmm13,240(%rsp)
          0x00,0x00,0x00,
        0x44,0x0F,0x29,0xB4,0x24,0x00,  // movaps %xmm14,256(%rsp)
          0x01,0x00,0x00,
        0xEB,0x1C,                      // jmp 0x80
        0x00,0x00,0x00,0x00,0x00,0x00,  // (padding)
        0x00,0x00,0x00,0x00,0x00,0x00,  // (padding)

        0x00,0x00,0x00,0x80,            // (data)
        0x00,0x00,0x00,0x00,            // (data)
        0x00,0x00,0x00,0x00,            // (data)
        0x00,0x00,0x00,0x00,            // (data)

        0x41,0xBF,0x00,0x00,0x80,0x3F,  // mov $0x3F800000,%r15
        0x66,0x45,0x0F,0x6E,0xF7,       // movd %r15,%xmm14
        0x44,0x0F,0x57,0x35,0xDD,0xFF,  // xorps -35(%rip),%xmm14
          0xFF,0xFF,

        0x44,0x0F,0x28,0xB4,0x24,0x00,  // movaps 256(%rsp),%xmm14
          0x01,0x00,0x00,
        0x44,0x0F,0x28,0xAC,0x24,0xF0,  // movaps 240(%rsp),%xmm13
          0x00,0x00,0x00,
        0x44,0x0F,0x28,0xA4,0x24,0xE0,  // movaps 224(%rsp),%xmm12
          0x00,0x00,0x00,
        0x44,0x0F,0x28,0x9C,0x24,0xD0,  // movaps 208(%rsp),%xmm11
          0x00,0x00,0x00,
        0x44,0x0F,0x28,0x94,0x24,0xC0,  // movaps 192(%rsp),%xmm10
          0x00,0x00,0x00,
        0x44,0x0F,0x28,0x8C,0x24,0xB0,  // movaps 176(%rsp),%xmm9
          0x00,0x00,0x00,
        0x44,0x0F,0x28,0x84,0x24,0xA0,  // movaps 160(%rsp),%xmm8
          0x00,0x00,0x00,
        0x0F,0x28,0xBC,0x24,0x90,0x00,  // movaps 144(%rsp),%xmm7
          0x00,0x00,
        0x0F,0x28,0xB4,0x24,0x80,0x00,  // movaps 128(%rsp),%xmm6
          0x00,0x00,
        0x48,0x81,0xC4,0x18,0x01,0x00,  // add $280,%rsp
          0x00,
        0x41,0x5F,                      // pop %r15
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
