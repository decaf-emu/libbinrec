/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "tests/execute.h"
/* Disable malloc() suppression from common.h. */
#undef malloc
#undef realloc
#undef free

#include <stdio.h>

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/mman.h>
    #ifndef MAP_ANONYMOUS
        #define MAP_ANONYMOUS  0x20
    #endif
#elif defined(_WIN32)
    #include <windows.h>
#endif

/*************************************************************************/
/*************************************************************************/

/* Cache of already-translated code, one entry per address. */
typedef void (*GuestCode)(void *);
static GuestCode *func_table = NULL;
static uint32_t func_table_base = -1;  // Guest address for func_table[0].
static uint32_t func_table_limit = 0;  // Address of last table entry plus one.

/*-----------------------------------------------------------------------*/

/**
 * make_callable:  Create a callable memory region containing the given
 * code.  Handles setting memory protections as needed.
 */
static void *make_callable(void *code, long code_size)
{
    void *func;

#if defined(__linux__) || defined(__APPLE__)
    /* Prepend the data size to the buffer so we know how much to free in
     * free_callable().  We prepend 64 bytes to preserve cache alignment. */
    long alloc_size = code_size + 64;
    void *base = mmap(NULL, alloc_size, PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        return NULL;
    }
    *((long *)base) = alloc_size;
    func = (void *)((uintptr_t)base + 64);
    memcpy(func, code, code_size);
    ASSERT(mprotect(base, alloc_size, PROT_READ | PROT_EXEC) == 0);
#elif defined(_WIN32)
    func = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);
    if (func == NULL) {
        return NULL;
    }
    memcpy(func, code, code_size);
    ASSERT(VirtualProtect(func, code_size, PAGE_EXECUTE_READ, (DWORD[1]){}));
#else
    func = code;  // Assume it can be called directly.
#endif

    return func;
}

/*-----------------------------------------------------------------------*/

/**
 * free_callable:  Free a pointer returned by make_callable().
 */
static void free_callable(void *ptr)
{
#if defined(__linux__) || defined(__APPLE__)
    long *base = (long *)((uintptr_t)ptr - 64);
    munmap(base, *base);
#elif defined(_WIN32)
    VirtualFree(ptr, 0, MEM_RELEASE);
#endif
}

/*-----------------------------------------------------------------------*/

/**
 * call:  Call the translated guest code for the given address, translating
 * it first if it has not yet been seen.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     address: Guest address from which to execute.
 *     arg: Argument to pass to the translated code.
 *     translated_code_callback: Function to call after translating code,
 *         or NULL for none.
 * [Return value]
 *     True on success, false on translation error.
 */
static bool call(binrec_t *handle, uint32_t address, void *arg,
    void (*translated_code_callback)(uint32_t, void *, long))
{
    if (address < func_table_base) {
        if (func_table_limit > func_table_base) {
            GuestCode *new_table =
                malloc(sizeof(*new_table) * (func_table_limit - address));
            if (!new_table) {
                fprintf(stderr, "Out of memory expanding translation cache\n");
                return false;
            }
            memset(new_table, 0,
                   sizeof(*new_table) * (func_table_base - address));
            memcpy(new_table + (func_table_base - address), func_table,
                   sizeof(*new_table) * (func_table_limit - func_table_base));
            free(func_table);
            func_table = new_table;
        } else {
            func_table_limit = address;
        }
        func_table_base = address;
    }
    if (address >= func_table_limit) {
        ASSERT(address != UINT32_C(-1));  // Just in case.
        const uint32_t new_limit = address + 1;
        GuestCode *new_table = realloc(
            func_table, sizeof(*new_table) * (new_limit - func_table_base));
        if (!new_table) {
            fprintf(stderr, "Out of memory expanding translation cache\n");
            return false;
        }
        memset(new_table + (func_table_limit - func_table_base), 0,
               sizeof(*new_table) * (new_limit - func_table_limit));
        func_table = new_table;
        func_table_limit = new_limit;
    }

    if (!func_table[address - func_table_base]) {
        void *code;
        long code_size;
        if (!binrec_translate(handle, address, -1, &code, &code_size)) {
            fprintf(stderr, "Failed to translate code at 0x%X\n", address);
            return false;
        }
        if (translated_code_callback) {
            (*translated_code_callback)(address, code, code_size);
        }
        GuestCode func = make_callable(code, code_size);
        free(code);
        if (!func) {
            fprintf(stderr, "Failed to make translated code callable for"
                    " 0x%X\n", address);
            return false;
        }
        func_table[address - func_table_base] = func;
    }

    (*func_table[address - func_table_base])(arg);
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * Clear the translation cache.
 */
static void clear_cache(void)
{
    if (func_table_limit > func_table_base) {
        for (int i = func_table_limit - func_table_base - 1; i >= 0; i--) {
            if (func_table[i]) {
                free_callable(func_table[i]);
            }
        }
    }
    free(func_table);
    func_table = NULL;
    func_table_base = -1;
    func_table_limit = 0;
}

/*-----------------------------------------------------------------------*/

bool call_guest_code(
    binrec_arch_t arch, void *state, void *memory, uint32_t address,
    void (*log)(void *userdata, binrec_loglevel_t level, const char *message),
    void (*configure_handle)(binrec_t *handle),
    void (*translated_code_callback)(uint32_t address, void *code,
                                    long code_size))
{
    ASSERT(arch == BINREC_ARCH_PPC_7XX);
    PPCState *state_ppc = state;

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = arch;
    setup.host = binrec_native_arch();
    setup.host_features = 0;//binrec_native_features();
    setup.guest_memory_base = memory;
    setup.host_memory_base = (uintptr_t)memory;
    setup.state_offset_gpr = offsetof(PPCState,gpr);
    setup.state_offset_fpr = offsetof(PPCState,fpr);
    setup.state_offset_gqr = offsetof(PPCState,gqr);
    setup.state_offset_cr = offsetof(PPCState,cr);
    setup.state_offset_lr = offsetof(PPCState,lr);
    setup.state_offset_ctr = offsetof(PPCState,ctr);
    setup.state_offset_xer = offsetof(PPCState,xer);
    setup.state_offset_fpscr = offsetof(PPCState,fpscr);
    setup.state_offset_reserve_flag = offsetof(PPCState,reserve_flag);
    setup.state_offset_reserve_state = offsetof(PPCState,reserve_state);
    setup.state_offset_nia = offsetof(PPCState,nia);
    setup.state_offset_timebase_handler = offsetof(PPCState,timebase_handler);
    setup.state_offset_sc_handler = offsetof(PPCState,sc_handler);
    setup.state_offset_trap_handler = offsetof(PPCState,trap_handler);
    setup.state_offset_branch_callback = offsetof(PPCState,branch_callback);
    setup.log = log;

    binrec_t *handle;
    handle = binrec_create_handle(&setup);
    if (!handle) {
        return false;
    }
    if (state_ppc->branch_callback) {
        binrec_enable_branch_callback(handle, true);
    }
    if (configure_handle) {
        (*configure_handle)(handle);
    }

    const uint32_t RETURN_ADDRESS = -4;  // Used to detect return-to-caller.
    state_ppc->lr = RETURN_ADDRESS;
    state_ppc->nia = address;
    while (state_ppc->nia != RETURN_ADDRESS) {
        if (!call(handle, state_ppc->nia, state_ppc, translated_code_callback)) {
            clear_cache();
            binrec_destroy_handle(handle);
            return false;
        }
    }

    clear_cache();
    binrec_destroy_handle(handle);
    return true;
}

/*************************************************************************/
/*************************************************************************/
