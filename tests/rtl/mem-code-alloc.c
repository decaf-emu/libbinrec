/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    /* Check behavior at various alignments, both less and greater than the
     * system's default alignment. */

    /* Largest alignment value to test. */
    #define MAX_ALIGNMENT  4096

    uint8_t testbuf[MAX_ALIGNMENT];
    for (size_t i = 0; i < MAX_ALIGNMENT; i++) {
        testbuf[i] = (uint8_t)(i ^ (i >> 8));
    }

    for (size_t alignment = 1; alignment < MAX_ALIGNMENT; alignment *= 2) {

        const size_t size1 = alignment;
        void *ptr = rtl_code_malloc(unit, size1, alignment);
        if (!ptr) {
            FAIL("code_malloc failed for alignment %zu", alignment);
        } else if ((uintptr_t)ptr % alignment != 0) {
            FAIL("code_malloc returned unaligned pointer %p for alignment"
                 " %zu", ptr, alignment);
        }
        memcpy(ptr, testbuf, size1);

        /* Allocate a different-size buffer to try and force a change in
         * alignment of the next realloc call. */
        void *dummy;
        EXPECT(dummy = malloc(alignment*4/3));

        const size_t size2 = 2*alignment;
        ptr = rtl_code_realloc(unit, ptr, size1, size2, alignment);
        if (!ptr) {
            FAIL("code_realloc failed for alignment %zu", alignment);
        } else if ((uintptr_t)ptr % alignment != 0) {
            FAIL("code_realloc returned unaligned pointer %p for alignment"
                 " %zu", ptr, alignment);
        } else if (memcmp(ptr, testbuf, size1) != 0) {
            FAIL("code_realloc corrupted data for alignment %zu", alignment);
        }

        free(dummy);
        rtl_code_free(unit, ptr);
    }

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
