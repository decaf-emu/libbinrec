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


/* Stubs for the code_* allocation callbacks which record their parameters
 * for checking by the test. */

static bool malloc_called, realloc_called, free_called;
static void *arg_userdata, *arg_ptr;
static size_t arg_size, arg_old_size, arg_new_size, arg_alignment;
static void *return_ptr;  // Should be set by the test before calling.

static void *code_malloc(void *userdata, size_t size, size_t alignment)
{
    malloc_called = true;
    arg_userdata = userdata;
    arg_size = size;
    arg_alignment = alignment;
    return return_ptr;
}

static void *code_realloc(void *userdata, void *ptr, size_t old_size,
                          size_t new_size, size_t alignment)
{
    realloc_called = true;
    arg_userdata = userdata;
    arg_ptr = ptr;
    arg_old_size = old_size;
    arg_new_size = new_size;
    arg_alignment = alignment;
    return return_ptr;
}

static void code_free(void *userdata, void *ptr)
{
    free_called = true;
    arg_userdata = userdata;
    arg_ptr = ptr;
}


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.userdata = (void *)0x123;
    setup.code_malloc = code_malloc;
    setup.code_realloc = code_realloc;
    setup.code_free = code_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    void *result;

    malloc_called = realloc_called = free_called = false;
    arg_userdata = NULL;
    arg_size = 0;
    arg_alignment = 0;
    return_ptr = (void *)0x234;
    result = rtl_code_malloc(unit, 1, 2);
    EXPECT(malloc_called);
    EXPECT_FALSE(realloc_called);
    EXPECT_FALSE(free_called);
    EXPECT_PTREQ(arg_userdata, setup.userdata);
    EXPECT_EQ(arg_size, 1);
    EXPECT_EQ(arg_alignment, 2);
    EXPECT_PTREQ(result, return_ptr);

    malloc_called = realloc_called = free_called = false;
    arg_userdata = NULL;
    arg_ptr = NULL;
    arg_old_size = 0;
    arg_new_size = 0;
    arg_alignment = 0;
    return_ptr = (void *)0x456;
    result = rtl_code_realloc(unit, (void *)0x345, 2, 3, 1);
    EXPECT_FALSE(malloc_called);
    EXPECT(realloc_called);
    EXPECT_FALSE(free_called);
    EXPECT_PTREQ(arg_userdata, setup.userdata);
    EXPECT_PTREQ(arg_ptr, (void *)0x345);
    EXPECT_EQ(arg_old_size, 2);
    EXPECT_EQ(arg_new_size, 3);
    EXPECT_EQ(arg_alignment, 1);

    malloc_called = realloc_called = free_called = false;
    arg_userdata = NULL;
    arg_ptr = NULL;
    rtl_code_free(unit, (void *)0x567);
    EXPECT_FALSE(malloc_called);
    EXPECT_FALSE(realloc_called);
    EXPECT(free_called);
    EXPECT_PTREQ(arg_userdata, setup.userdata);
    EXPECT_PTREQ(arg_ptr, (void *)0x567);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
