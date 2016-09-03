/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "tests/mem-wrappers.h"
#include <stdbool.h>


/* Counter for remaining calls until a forced failure (1 = fail the next
 * allocation call), or 0 if no forced failure is pending. */
static int fail_counter = 0;

void *mem_wrap_malloc(UNUSED void *userdata, size_t size)
{
    if (fail_counter > 0) {
        if (--fail_counter == 0) {
            return NULL;
        }
    }
    return malloc(size);
}

void *mem_wrap_realloc(UNUSED void *userdata, void *ptr, size_t size)
{
    if (fail_counter > 0) {
        if (--fail_counter == 0) {
            return NULL;
        }
    }
    return realloc(ptr, size);
}

void mem_wrap_free(UNUSED void *userdata, void *ptr)
{
    return free(ptr);
}

void mem_wrap_fail_after(int count)
{
    fail_counter = count + 1;
}

void mem_wrap_cancel_fail()
{
    fail_counter = 0;
}
