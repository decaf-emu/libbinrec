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
#include "tests/guest-ppc/common.h"
#include "tests/log-capture.h"


static void configure_handle(binrec_t *handle)
{
    binrec_set_optimization_flags(handle, ~0u, ~0u, ~0u);
}

int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    uint8_t *memory;
    EXPECT(memory = malloc(0x10000));

    static const uint32_t ppc_code[] = {
        0x38602000,  // li r3,0x2000
        0x3C800010,  // li r4,1048576
        0x7CA01828,  // lwarx r5,0,r3
        0x7C00186C,  // dcbst 0,r3
        0x38A50001,  // addi r5,r5,1
        0x7CA0192D,  // stwcx. r5,0,r3
        0x40A2FFF0,  // bne 0x1008
        0x3884FFFF,  // addi r4,r4,-1
        0x2C040000,  // cmpi r4,0
        0x4082FFE4,  // bne 0x1008
        0x4E800020,  // blr
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    #define NUM_THREADS  4

    PPCState state[NUM_THREADS];
    memset(state, 0, sizeof(state));

    void *thread[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread[i] = spawn_guest_code(BINREC_ARCH_PPC_7XX, &state[i], memory,
                                     start_address, configure_handle, NULL);
        if (!thread[i]) {
            for (i--; i >= 0; i--) {
                wait_guest_code(thread[i]);
            }
            FAIL("Failed to spawn guest code (iteration %d)", i);
            break;
        }
    }

    bool failed = false;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (!wait_guest_code(thread[i])) {
            failed = true;
        }
    }
    if (failed) {
        FAIL("Failed to execute guest code");
    }

    uint32_t mem_0x2000;
    memcpy_be32(&mem_0x2000, (void *)(memory+0x2000), 4);
    EXPECT_EQ(mem_0x2000, NUM_THREADS*1048576);

    free(memory);
    return EXIT_SUCCESS;
}
