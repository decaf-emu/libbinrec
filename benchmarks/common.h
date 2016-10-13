/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BENCHMARKS_COMMON_H
#define BENCHMARKS_COMMON_H

#ifndef SRC_COMMON_H
    #include "src/common.h"
#endif

/* Disable malloc() suppression from src/common.h. */
#undef malloc
#undef realloc
#undef free

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct RTLUnit;

/*************************************************************************/
/********************** Benchmark utility routines ***********************/
/*************************************************************************/

/**
 * alloc_guest_memory:  Allocate a region of host memory to use as the
 * guest address space.  If possible, the first host page of the region
 * is made inaccessible to detect null pointer dereference attempts.
 * The contents of the memory region are initialized to all zero.
 *
 * [Parameters]
 *     size: Amount of memory to allocate (starting from guest address zero).
 * [Return value]
 *     Base of the guest address space, or NULL on error.
 */
extern void *alloc_guest_memory(uint64_t size);

/**
 * free_guest_memory:  Free memory allocated with alloc_guest_memory().
 *
 * [Parameters]
 *     ptr: Memory region to free.
 *     size: Size of the memory region.  Must be the same as the value
 *         passed to alloc_guest_memory().
 */
extern void free_guest_memory(void *ptr, uint64_t size);

/**
 * get_cpu_time:  Return the current user-mode and system-mode CPU timers
 * for the current process.
 *
 * [Parameters]
 *     user_ret: Pointer to variable to receive the current user-mode CPU
 *         time consumed, in seconds.
 *     sys_ret: Pointer to variable to receive the current system-mode CPU
 *         time consumed, in seconds.
 */
extern void get_cpu_time(double *user_ret, double *sys_ret);

/*************************************************************************/
/*************************************************************************/

#endif  /* BENCHMARKS_COMMON_H */
