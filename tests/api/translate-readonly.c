/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "tests/common.h"
#include "tests/execute.h"
#include "tests/log-capture.h"

#include "src/rtl-internal.h"  // For RTL_DEBUG_OPTIMIZE definition.


static uint8_t memory[0x10000];


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = BINREC_ARCH_PPC_7XX;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.guest_memory_base = memory;
    ppc32_fill_setup(&setup);
    setup.log = log_capture;

    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));
    binrec_set_optimization_flags(handle, BINREC_OPT_FOLD_CONSTANTS, 0, 0);
    binrec_add_readonly_region(handle, 0x1000, 0xC);

    static const uint8_t ppc_code[] = {
        0x80,0x60,0x10,0x08,  // lwz r3,0x1008(0)
        0x48,0x00,0x00,0x08,  // b 0x100C
        0x12,0x34,0x56,0x78,  // .int 0x12345678
    };
    const uint32_t start_address = 0x1000;
    const uint32_t end_address = start_address + sizeof(ppc_code) - 1;
    memcpy(memory + start_address, ppc_code, sizeof(ppc_code));

    void *x86_code;
    long x86_code_size;
    EXPECT(binrec_translate(handle, NULL, start_address, end_address,
                            &x86_code, &x86_code_size));

    static const uint8_t x86_expected[] = {
        0x48,0x83,0xEC,0x08,            // sub $8,%rsp
        0xB8,0x78,0x56,0x34,0x12,       // mov $0x12345678,%eax
        0x89,0x47,0x0C,                 // mov %eax,12(%rdi)
        0xB8,0x0C,0x10,0x00,0x00,       // mov $0x100C,%eax
        0x89,0x87,0xC4,0x02,0x00,0x00,  // mov %eax,708(%rdi)
        0xE9,0x0B,0x00,0x00,0x00,       // jmp epilogue
        0xB8,0x08,0x10,0x00,0x00,       // mov $0x1008,%eax
        0x89,0x87,0xC4,0x02,0x00,0x00,  // mov %eax,708(%rdi)
        0x48,0x83,0xC4,0x08,            // epilogue: add $8,%rsp
        0xC3,                           // ret
    };
    EXPECT_MEMEQ(x86_code, x86_expected, sizeof(x86_expected));
    EXPECT_EQ(x86_code_size, sizeof(x86_expected));

#ifdef RTL_DEBUG_OPTIMIZE
    EXPECT_STREQ(get_log_messages(),
                 "[info] r3 loads constant value 0x12345678 from 0x1008 at 2\n"
                 "[info] r2 no longer used, setting death = birth\n");
#else
    EXPECT_STREQ(get_log_messages(), NULL);
#endif

    free(x86_code);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
