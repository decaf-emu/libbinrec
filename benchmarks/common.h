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
