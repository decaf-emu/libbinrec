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
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


/* Simple wrappers for the code_* allocation functions.  These will never
 * be called, so we just return NULL for allocation attempts. */
static void *code_malloc(UNUSED void *userdata, UNUSED size_t size,
                         UNUSED size_t alignment) {
    return NULL;
}
static void *code_realloc(UNUSED void *userdata, UNUSED void *ptr,
                          UNUSED size_t old_size, UNUSED size_t new_size,
                          UNUSED size_t alignment) {
    return NULL;
}
static void code_free(UNUSED void *userdata, UNUSED void *ptr) {}

int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;

    /* Check that the default handlers don't overwrite a set of supplied
     * handlers.  We don't make use of forced failure here, but the memory
     * allocation wrappers are convenient pointers we can use to verify
     * that the supplied handlers are copied into the handle. */
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.code_malloc = code_malloc;
    setup.code_realloc = code_realloc;
    setup.code_free = code_free;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_PTREQ(handle->setup.malloc, setup.malloc);
    EXPECT_PTREQ(handle->setup.realloc, setup.realloc);
    EXPECT_PTREQ(handle->setup.free, setup.free);
    EXPECT_PTREQ(handle->setup.code_malloc, setup.code_malloc);
    EXPECT_PTREQ(handle->setup.code_realloc, setup.code_realloc);
    EXPECT_PTREQ(handle->setup.code_free, setup.code_free);
    binrec_destroy_handle(handle);
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check that if a set of supplied handlers is incomplete, all handlers
     * in that set are replaced with the default handlers. */
    binrec_setup_t setup2 = setup;
    setup2.malloc = NULL;
    EXPECT(handle = binrec_create_handle(&setup2));
    EXPECT_PTREQ(handle->setup.malloc, NULL);
    EXPECT_PTREQ(handle->setup.realloc, NULL);
    EXPECT_PTREQ(handle->setup.free, NULL);
    EXPECT_PTREQ(handle->setup.code_malloc, setup.code_malloc);
    EXPECT_PTREQ(handle->setup.code_realloc, setup.code_realloc);
    EXPECT_PTREQ(handle->setup.code_free, setup.code_free);
    binrec_destroy_handle(handle);
    EXPECT_STREQ(get_log_messages(),
                 "[warning] Some but not all memory allocation functions"
                 " were defined.  Using system allocator instead.\n");
    clear_log_messages();

    setup2 = setup;
    setup2.code_malloc = NULL;
    EXPECT(handle = binrec_create_handle(&setup2));
    EXPECT_PTREQ(handle->setup.malloc, setup.malloc);
    EXPECT_PTREQ(handle->setup.realloc, setup.realloc);
    EXPECT_PTREQ(handle->setup.free, setup.free);
    EXPECT_PTREQ(handle->setup.code_malloc, NULL);
    EXPECT_PTREQ(handle->setup.code_realloc, NULL);
    EXPECT_PTREQ(handle->setup.code_free, NULL);
    binrec_destroy_handle(handle);
    EXPECT_STREQ(get_log_messages(),
                 "[warning] Some but not all output code memory allocation"
                 " functions were defined.  Using default allocator"
                 " instead.\n");
    clear_log_messages();

    return EXIT_SUCCESS;
}
