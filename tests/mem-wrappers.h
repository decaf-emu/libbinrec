/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_MEM_WRAPPERS_H
#define TESTS_MEM_WRAPPERS_H

#ifndef BINREC_H
    #include "include/binrec.h"
#endif

#include <stddef.h>

/*************************************************************************/
/*************************************************************************/

/**
 * mem_wrap_malloc, mem_wrap_realloc, mem_wrap_free:  Wrappers for
 * malloc(), realloc(), and free() which optionally fail the request (see
 * mem_wrap_fail() below).
 */
extern void *mem_wrap_malloc(void *userdata, size_t size);
extern void *mem_wrap_realloc(void *userdata, void *ptr, size_t size);
extern void mem_wrap_free(void *userdata, void *ptr);

/**
 * mem_wrap_fail_after:  Cause the (count+1)th call to mem_wrap_malloc() or
 * mem_wrap_realloc() to return NULL instead of allocating memory.
 *
 * [Parameters]
 *     count: Number of malloc() or realloc() calls that should be allowed
 *         to succeed before the failure (0 = fail the next allocation call).
 */
extern void mem_wrap_fail_after(int count);

/**
 * mem_wrap_cancel_fail:  Cancel the effect of any previous mem_wrap_fail()
 * call, whether or not the forced failure has been consumed.
 */
extern void mem_wrap_cancel_fail(void);

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_MEM_WRAPPERS_H
