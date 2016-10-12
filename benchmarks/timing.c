/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "benchmarks/common.h"

#if defined(__linux__)
    #include <sys/resource.h>
    #include <sys/time.h>
#elif defined(_WIN32)
    #include <windows.h>
#else
    #error Timing method for this system unknown!
#endif

/*************************************************************************/
/*************************************************************************/

void get_cpu_time(double *user_ret, double *sys_ret)
{
    ASSERT(user_ret);
    ASSERT(sys_ret);

    #if defined(__linux__)
        struct rusage ru;
        ASSERT(getrusage(RUSAGE_SELF, &ru) == 0);
        *user_ret = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec * 1.0e-6;
        *sys_ret = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec * 1.0e-6;
    #elif defined(_WIN32)
        FILETIME dummy, sys, user;
        ASSERT(GetProcessTimes(GetCurrentProcess(),
                               &dummy, &dummy, &sys, &user));
        *user_ret = user.QuadPart * 1.0e-7;
        *sys_ret = sys.QuadPart * 1.0e-7;
    #endif
}

/*************************************************************************/
/*************************************************************************/
