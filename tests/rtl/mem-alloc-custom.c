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


/* Stubs for the memory allocation callbacks which record their parameters
 * for checking by the test. */

static bool malloc_called, realloc_called, free_called;
static void *arg_userdata, *arg_ptr;
static size_t arg_size;
static void *return_ptr;  // Should be set by the test before calling.

static void *custom_malloc(void *userdata, size_t size)
{
    malloc_called = true;
    arg_userdata = userdata;
    arg_size = size;
    return return_ptr;
}

static void *custom_realloc(void *userdata, void *ptr, size_t size)
{
    realloc_called = true;
    arg_userdata = userdata;
    arg_ptr = ptr;
    arg_size = size;
    return return_ptr;
}

static void custom_free(void *userdata, void *ptr)
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
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    /* Set these after creating the unit so regular allocations don't fail. */
    handle->setup.malloc = custom_malloc;
    handle->setup.realloc = custom_realloc;
    handle->setup.free = custom_free;

    void *result;

    malloc_called = realloc_called = free_called = false;
    arg_userdata = NULL;
    arg_size = 0;
    return_ptr = (void *)0x234;
    result = rtl_malloc(unit, 1);
    EXPECT(malloc_called);
    EXPECT_FALSE(realloc_called);
    EXPECT_FALSE(free_called);
    EXPECT_PTREQ(arg_userdata, setup.userdata);
    EXPECT_EQ(arg_size, 1);
    EXPECT_PTREQ(result, return_ptr);

    malloc_called = realloc_called = free_called = false;
    arg_userdata = NULL;
    arg_ptr = NULL;
    arg_size = 0;
    return_ptr = (void *)0x456;
    result = rtl_realloc(unit, (void *)0x345, 2);
    EXPECT_FALSE(malloc_called);
    EXPECT(realloc_called);
    EXPECT_FALSE(free_called);
    EXPECT_PTREQ(arg_userdata, setup.userdata);
    EXPECT_PTREQ(arg_ptr, (void *)0x345);
    EXPECT_EQ(arg_size, 2);

    malloc_called = realloc_called = free_called = false;
    arg_userdata = NULL;
    arg_ptr = NULL;
    rtl_free(unit, (void *)0x567);
    EXPECT_FALSE(malloc_called);
    EXPECT_FALSE(realloc_called);
    EXPECT(free_called);
    EXPECT_PTREQ(arg_userdata, setup.userdata);
    EXPECT_PTREQ(arg_ptr, (void *)0x567);

    /* Clear custom handlers before destroying the unit so memory is
     * properly freed. */
    handle->setup.malloc = NULL;
    handle->setup.realloc = NULL;
    handle->setup.free = NULL;

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
