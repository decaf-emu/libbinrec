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
#include "tests/log-capture.h"
/* Disable malloc() suppression from common.h. */
#undef malloc
#undef realloc
#undef free

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

/**
 * call:  Call the given buffer as a native function.  Handles setting
 * memory protections as needed.
 */
static void call(void *code, long code_size, void *arg)
{
    void (*func)(void *);

#if defined(__linux__)
    func = mmap(NULL, code_size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
    ASSERT(func != MAP_FAILED);
    memcpy(func, code, code_size);
    ASSERT(mprotect(func, code_size, PROT_READ | PROT_EXEC) == 0);
#elif defined(_WIN32)
    func = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);
    ASSERT(func != NULL);
    memcpy(func, code, code_size);
    ASSERT(VirtualProtect(func, code_size, PAGE_EXECUTE_READ, (DWORD[1]){}));
#else
    func = code;  // Assume it can be called directly.
#endif

    (*func)(arg);

#if defined(__linux__)
    munmap(func, code_size);
#elif defined(_WIN32)
    VirtualFree(func, 0, MEM_RELEASE);
#endif
}

/*-----------------------------------------------------------------------*/

bool call_guest_code(binrec_arch_t arch, void *state, void *memory,
                     uint32_t address, unsigned int common_opt,
                     unsigned int guest_opt, unsigned int host_opt,
                     int inline_length, int inline_depth)
{
    ASSERT(arch == BINREC_ARCH_PPC_7XX);
    PPCState *state_ppc = state;

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = arch;
    setup.host = binrec_native_arch();
    setup.host_features = binrec_native_features();
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
    setup.log = log_capture;

    binrec_t *handle;
    handle = binrec_create_handle(&setup);
    if (!handle) {
        return false;
    }
    binrec_set_optimization_flags(handle, common_opt, guest_opt, host_opt);
    binrec_set_max_inline_length(handle, inline_length);
    binrec_set_max_inline_depth(handle, inline_depth);

    const uint32_t RETURN_ADDRESS = -4;  // Used to detect return-to-caller.
    state_ppc->lr = RETURN_ADDRESS;
    state_ppc->nia = address;
    while (state_ppc->nia != RETURN_ADDRESS) {
        void *code;
        long code_size;
        if (!binrec_translate(handle, state_ppc->nia, -1, &code, &code_size)) {
            binrec_destroy_handle(handle);
            return false;
        }
        call(code, code_size, state_ppc);
        free(code);
    }

    binrec_destroy_handle(handle);
    return true;
}

/*************************************************************************/
/*************************************************************************/
