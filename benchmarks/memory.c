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
    #include <sys/mman.h>
    #ifndef MAP_ANONYMOUS
        #define MAP_ANONYMOUS  0x20
    #endif
#elif defined(_WIN32)
    #include <windows.h>
#endif

/*************************************************************************/
/*************************************************************************/

void *alloc_guest_memory(uint64_t size)
{
    void *ptr;

    #if defined(__linux__)
        ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
            return NULL;
        }
        ASSERT(mprotect(ptr, 1, PROT_NONE) == 0);
    #elif defined(_WIN32)
        ptr = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE,
                           PAGE_READWRITE);
        ASSERT(VirtualProtect(ptr, 1, PAGE_NOACCESS, (DWORD[1]){}));
    #else
        ptr = calloc(size, 1);
    #endif

     return ptr;
}

/*-----------------------------------------------------------------------*/

void free_guest_memory(void *ptr, uint64_t size)
{
    ASSERT(ptr);

    #if defined(__linux__)
        munmap(ptr, size);
    #elif defined(_WIN32)
        VirtualFree(ptr, 0, MEM_RELEASE);
    #else
        free(ptr);
    #endif
}

/*************************************************************************/
/*************************************************************************/
