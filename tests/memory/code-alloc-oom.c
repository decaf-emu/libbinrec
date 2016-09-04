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


/* Stubs for the memory allocation callbacks which always fail.
 * We replace callback pointers on the fly, so we don't need a full set. */
static void *custom_malloc(void *userdata, size_t size) {
    return NULL;
}
static void *custom_realloc(void *userdata, void *ptr, size_t size) {
    return NULL;
}


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    handle->setup.malloc = custom_malloc;
    /* Check both code paths. */
    EXPECT_FALSE(binrec_code_malloc(handle, 1, sizeof(void *)));
    EXPECT_FALSE(binrec_code_malloc(handle, 1, 2 * sizeof(void *)));
    handle->setup.malloc = NULL;

    handle->setup.realloc = custom_realloc;
    void *ptr;
    EXPECT(ptr = binrec_code_malloc(handle, 2, sizeof(void *)));
    EXPECT_FALSE(binrec_code_realloc(handle, ptr, 2, 1, sizeof(void *)));
    binrec_code_free(handle, ptr);
    EXPECT(ptr = binrec_code_malloc(handle, 2, 2 * sizeof(void *)));
    EXPECT_FALSE(binrec_code_realloc(handle, ptr, 2, 1, 2 * sizeof(void *)));
    binrec_code_free(handle, ptr);
    handle->setup.realloc = NULL;

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
