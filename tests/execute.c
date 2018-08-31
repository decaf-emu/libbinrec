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
    #include <pthread.h>
    #include <sys/mman.h>
    #ifndef MAP_ANONYMOUS
        #define MAP_ANONYMOUS  0x20
    #endif
#elif defined(_WIN32)
    #include <windows.h>
#endif

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/* Cache of already-translated code, one entry per address. */
typedef void (*GuestCode)(void *, void *);
typedef struct CodeCache {
    GuestCode *func_table;
    uint32_t func_table_base;   // Guest address for func_table[0].
    uint32_t func_table_limit;  // Address of last table entry plus one.
} CodeCache;

/* Extension of guest CPU state with cache pointer (for threaded calls). */
typedef struct PPCStateAndCache {
    PPCState state;
    CodeCache cache;
} PPCStateAndCache;

/* State block for thread spawning. */
typedef struct ThreadState {
    /* Thread handle. */
#if defined(__linux__) || defined(__APPLE__)
    pthread_t handle;
#elif defined(_WIN32)
    HANDLE handle;
#endif
    /* Function arguments. */
    binrec_arch_t arch;
    void *state;
    void *memory;
    uint32_t address;
    void (*log)(void *userdata, binrec_loglevel_t level, const char *message);
    void (*configure_handle)(binrec_t *handle);
    void (*translated_code_callback)(uint32_t address, void *code,
                                    long code_size);
} ThreadState;

/*-----------------------------------------------------------------------*/

/**
 * make_callable:  Create a callable memory region containing the given
 * code.  Handles setting memory protections as needed.
 */
static void *make_callable(void *code, long code_size, bool writable)
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
    const unsigned int flags =
        PROT_READ | PROT_EXEC | (writable ? PROT_WRITE : 0);
    ASSERT(mprotect(base, alloc_size, flags) == 0);
#elif defined(_WIN32)
    func = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);
    if (func == NULL) {
        return NULL;
    }
    memcpy(func, code, code_size);
    ASSERT(VirtualProtect(
               func, code_size,
               writable ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ,
               (DWORD[1]){0}));
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
 * Initialize the translation cache.
 */
static void init_cache(CodeCache *cache)
{
    cache->func_table = NULL;
    cache->func_table_base = -1;
    cache->func_table_limit = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * translate:  Translate guest code at the given address into host code
 * and store it in the translated code cache.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     state: Processor state block for reference by optimizers.
 *     address: Guest address from which to execute.
 *     translated_code_callback: Function to call after translating code,
 *         or NULL for none.
 * [Return value]
 *     True on success, false on error.
 */
static bool translate(
    binrec_t *handle, void *state, uint32_t address, CodeCache *cache,
    void (*translated_code_callback)(uint32_t, void *, long))
{
    if (address < cache->func_table_base) {
        if (cache->func_table_limit > cache->func_table_base) {
            GuestCode *new_table = malloc(
                sizeof(*new_table) * (cache->func_table_limit - address));
            if (!new_table) {
                fprintf(stderr, "Out of memory expanding translation cache\n");
                return false;
            }
            memset(new_table, 0,
                   sizeof(*new_table) * (cache->func_table_base - address));
            memcpy(new_table + (cache->func_table_base - address),
                   cache->func_table,
                   sizeof(*new_table) * (cache->func_table_limit
                                         - cache->func_table_base));
            free(cache->func_table);
            cache->func_table = new_table;
        } else {
            cache->func_table_limit = address;
        }
        cache->func_table_base = address;
    }
    if (address >= cache->func_table_limit) {
        ASSERT(address != UINT32_C(-1));  // Just in case.
        const uint32_t new_limit = address + 1;
        GuestCode *new_table = realloc(
            cache->func_table,
            sizeof(*new_table) * (new_limit - cache->func_table_base));
        if (!new_table) {
            fprintf(stderr, "Out of memory expanding translation cache\n");
            return false;
        }
        memset(new_table + (cache->func_table_limit - cache->func_table_base),
               0, sizeof(*new_table) * (new_limit - cache->func_table_limit));
        cache->func_table = new_table;
        cache->func_table_limit = new_limit;
    }

    if (!cache->func_table[address - cache->func_table_base]) {
        void *code;
        long code_size;
        bool success = binrec_translate(handle, state, address, -1,
                                        &code, &code_size);
        if (!success) {
            uint32_t limit = 4096;
            do {
                success = binrec_translate(handle, state, address,
                                           address+limit-1, &code, &code_size);
            } while (!success && (limit /= 2) >= 4);
        }
        if (!success) {
            fprintf(stderr, "Failed to translate code at 0x%X\n", address);
            return false;
        }
        if (translated_code_callback) {
            (*translated_code_callback)(address, code, code_size);
        }
        GuestCode func = make_callable(code, code_size, handle->use_chaining);
        free(code);
        if (!func) {
            fprintf(stderr, "Failed to make translated code callable for"
                    " 0x%X\n", address);
            return false;
        }
        cache->func_table[address - cache->func_table_base] = func;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * Clear the translation cache.
 */
static void clear_cache(CodeCache *cache)
{
    if (cache->func_table_limit > cache->func_table_base) {
        for (int i = cache->func_table_limit - cache->func_table_base - 1;
             i >= 0; i--)
        {
            if (cache->func_table[i]) {
                free_callable(cache->func_table[i]);
            }
        }
    }
    free(cache->func_table);
    init_cache(cache);
}

/*-----------------------------------------------------------------------*/

/**
 * Look up the given address in the translation cache.  Used as a
 * cache_lookup() callback when chaining is enabled.
 */
static void *cache_lookup(PPCState *state, uint32_t address)
{
    const PPCStateAndCache *state_cache = (PPCStateAndCache *)state;
    const CodeCache *cache = &state_cache->cache;
    if (address >= cache->func_table_base
     && address < cache->func_table_limit) {
        return cache->func_table[address - cache->func_table_base];
    } else {
        return NULL;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * Thread runner for spawn_guest_code().
 */
static
#if defined(__linux__) || defined(__APPLE__)
    void *
#elif defined(_WIN32)
    WINAPI DWORD
#else
    int
#endif
    thread_runner(void *arg)
{
    ThreadState *thread = arg;

    const bool result = call_guest_code(
        thread->arch, thread->state, thread->memory, thread->address,
        thread->log, thread->configure_handle,
        thread->translated_code_callback);

#if defined(__linux__) || defined(__APPLE__)
    return (void *)(uintptr_t)result;
#else
    return result;
#endif
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

bool call_guest_code(
    binrec_arch_t arch, void *state, void *memory, uint32_t address,
    void (*log)(void *userdata, binrec_loglevel_t level, const char *message),
    void (*configure_handle)(binrec_t *handle),
    void (*translated_code_callback)(uint32_t address, void *code,
                                    long code_size))
{
    bool retval = false;

    ASSERT(arch == BINREC_ARCH_PPC_7XX);
    PPCStateAndCache state_cache_ppc;
    memcpy(&state_cache_ppc.state, state, sizeof(state_cache_ppc.state));
    init_cache(&state_cache_ppc.cache);

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = arch;
    setup.host = binrec_native_arch();
    setup.host_features = binrec_native_features();
    setup.guest_memory_base = memory;
    ppc32_fill_setup(&setup);
    setup.log = log;

    binrec_t *handle;
    handle = binrec_create_handle(&setup);
    if (!handle) {
        return false;
    }
    if (configure_handle) {
        (*configure_handle)(handle);
    }
    if (handle->use_chaining) {
        state_cache_ppc.state.chain_lookup = cache_lookup;
    }

    /* Pull cache info into registers to reduce the number of loads needed. */
    GuestCode *table = state_cache_ppc.cache.func_table;
    uint32_t base = state_cache_ppc.cache.func_table_base;
    uint32_t limit = (state_cache_ppc.cache.func_table_limit
                      - state_cache_ppc.cache.func_table_base);

    const uint32_t RETURN_ADDRESS = -4;  // Used to detect return-to-caller.
    state_cache_ppc.state.lr = RETURN_ADDRESS;
    state_cache_ppc.state.nia = address;
    while (state_cache_ppc.state.nia != RETURN_ADDRESS) {
        const uint32_t nia = state_cache_ppc.state.nia;
        if (UNLIKELY(nia - base >= limit) || UNLIKELY(!table[nia - base])) {
            if (!translate(handle, &state_cache_ppc.state, nia,
                           &state_cache_ppc.cache, translated_code_callback)) {
                goto out;
            }
            table = state_cache_ppc.cache.func_table;
            base = state_cache_ppc.cache.func_table_base;
            limit = (state_cache_ppc.cache.func_table_limit
                     - state_cache_ppc.cache.func_table_base);
        }
        (*table[nia - base])(&state_cache_ppc.state, memory);
    }

    retval = true;

  out:
    clear_cache(&state_cache_ppc.cache);
    binrec_destroy_handle(handle);
    memcpy(state, &state_cache_ppc.state, sizeof(state_cache_ppc.state));
    return retval;
}

/*-----------------------------------------------------------------------*/

void *spawn_guest_code(
    binrec_arch_t arch, void *state, void *memory, uint32_t address,
    void (*log)(void *userdata, binrec_loglevel_t level, const char *message),
    void (*configure_handle)(binrec_t *handle),
    void (*translated_code_callback)(uint32_t address, void *code,
                                    long code_size))
{
    ThreadState *thread = malloc(sizeof(*thread));
    if (!thread) {
        fprintf(stderr, "Out of memory for pthread handle\n");
        return NULL;
    }
    thread->arch = arch;
    thread->state = state;
    thread->memory = memory;
    thread->address = address;
    thread->log = log;
    thread->configure_handle = configure_handle;
    thread->translated_code_callback = translated_code_callback;

#if defined(__linux__) || defined(__APPLE__)
    int error = pthread_create(&thread->handle, NULL, thread_runner, thread);
    if (error) {
        fprintf(stderr, "pthread_create(): %s", strerror(error));
        goto error;
    }
#elif defined(_WIN32)
    thread->handle = CreateThread(NULL, 0, thread_runner, thread, 0, NULL);
    if (!thread->handle) {
        fprintf(stderr, "CreateThread() failed: %d", GetLastError());
        goto error;
    }
#else
    fprintf(stderr, "No thread library available\n");
    goto error;
#endif

    return thread;

  error:
    free(thread);
    return NULL;
}

/*-----------------------------------------------------------------------*/

bool wait_guest_code(void *thread_)
{
    ThreadState *thread = thread_;
    ASSERT(thread);

    bool result = false;
#if defined(__linux__) || defined(__APPLE__)
    void *result_;
    pthread_join(thread->handle, &result_);
    result = ((uintptr_t)result_ != 0);
#elif defined(_WIN32)
    WaitForSingleObject(thread->handle, INFINITE);
    DWORD result_ = 0;
    GetExitCodeThread(thread->handle, &result_);
    result = (result_ != 0);
#endif

    free(thread);
    return result;
}

/*************************************************************************/
/*************************************************************************/
