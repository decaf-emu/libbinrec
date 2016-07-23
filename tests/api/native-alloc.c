/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/common.h"
#include "tests/common.h"


/* Largest alignment value to test. */
#define MAX_ALIGNMENT  4096


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    /* binrec_create() should fill in the default allocation handlers. */
    void *(*default_malloc)(void *, size_t, size_t);
    void *(*default_realloc)(void *, void *, size_t, size_t, size_t);
    void (*default_free)(void *, void *);
    EXPECT(default_malloc = handle->setup.native_malloc);
    EXPECT(default_realloc = handle->setup.native_realloc);
    EXPECT(default_free = handle->setup.native_free);

    /* Check behavior at various alignments, both less and greater than the
     * system's default alignment. */

    uint8_t testbuf[MAX_ALIGNMENT];
    for (size_t i = 0; i < MAX_ALIGNMENT; i++) {
        testbuf[i] = (uint8_t)(i ^ (i >> 8));
    }

    for (size_t alignment = 1; alignment < MAX_ALIGNMENT; alignment *= 2) {

        const size_t size1 = alignment;
        void *ptr = (*default_malloc)(NULL, size1, alignment);
        if (!ptr) {
            FAIL("native_malloc failed for alignment %zu", alignment);
        } else if ((uintptr_t)ptr % alignment != 0) {
            FAIL("native_malloc returned unaligned pointer %p for alignment"
                 " %zu", ptr, alignment);
        }
        memcpy(ptr, testbuf, size1);

        /* Allocate a different-size buffer to try and force a change in
         * alignment of the next realloc call. */
        void *dummy;
        EXPECT(dummy = malloc(alignment*4/3));

        const size_t size2 = 2*alignment;
        ptr = (*default_realloc)(NULL, ptr, size1, size2, alignment);
        if (!ptr) {
            FAIL("native_realloc failed for alignment %zu", alignment);
        } else if ((uintptr_t)ptr % alignment != 0) {
            FAIL("native_realloc returned unaligned pointer %p for alignment"
                 " %zu", ptr, alignment);
        } else if (memcmp(ptr, testbuf, size1) != 0) {
            FAIL("native_realloc corrupted data for alignment %zu", alignment);
        }

        free(dummy);
        (*default_free)(NULL, ptr);
    }

    binrec_destroy_handle(handle);

    /* Check that the default handlers don't overwrite a set of supplied
     * handlers. */
    *(void **)&setup.native_malloc = (void *)1;  // Cast to avoid warning.
    *(void **)&setup.native_realloc = (void *)2;
    *(void **)&setup.native_free = (void *)3;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT(handle->setup.native_malloc == setup.native_malloc);
    EXPECT(handle->setup.native_realloc == setup.native_realloc);
    EXPECT(handle->setup.native_free == setup.native_free);
    binrec_destroy_handle(handle);

    /* Check that if the set of supplied handlers is incomplete, they are
     * replaced with the default handlers. */
    setup.native_malloc = NULL;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT(handle->setup.native_malloc == default_malloc);
    EXPECT(handle->setup.native_realloc == default_realloc);
    EXPECT(handle->setup.native_free == default_free);
    binrec_destroy_handle(handle);

    return EXIT_SUCCESS;
}
