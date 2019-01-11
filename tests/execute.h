/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_EXECUTE_H
#define TESTS_EXECUTE_H

#ifndef BINREC_H
    #include "include/binrec.h"
#endif
#include <stdbool.h>

/*************************************************************************/
/*************************************************************************/

/* Processor state for 32-bit PowerPC architectures. */
typedef struct PPCState {
    uint32_t gpr[32];
    double fpr[32][2];
    uint32_t gqr[8];
    uint32_t cr;
    uint32_t lr;
    uint32_t ctr;
    uint32_t xer;
    uint32_t fpscr;
    uint32_t pvr;
    uint32_t pir;
    uint8_t reserve_flag;
    uint32_t reserve_state;
    uint32_t nia;
    uint64_t tb;
    uint64_t (*timebase_handler)(struct PPCState *state);
    struct PPCState *(*sc_handler)(struct PPCState *state, uint32_t insn);
    struct PPCState *(*trap_handler)(struct PPCState *state);
    void *(*chain_lookup)(struct PPCState *state, uint32_t branch_address);
    uint32_t branch_exit_flag;
    const uint16_t *fres_lut;
    const uint16_t *frsqrte_lut;
} PPCState;

/**
 * ppc32_fill_setup:  Fill the state offset fields of the given setup
 * structure with the proper offsets for a 32-bit PowerPC state block.
 */
static inline void ppc32_fill_setup(binrec_setup_t *setup)
{
    setup->state_offsets_ppc.gpr = offsetof(PPCState,gpr);
    setup->state_offsets_ppc.fpr = offsetof(PPCState,fpr);
    setup->state_offsets_ppc.gqr = offsetof(PPCState,gqr);
    setup->state_offsets_ppc.cr = offsetof(PPCState,cr);
    setup->state_offsets_ppc.lr = offsetof(PPCState,lr);
    setup->state_offsets_ppc.ctr = offsetof(PPCState,ctr);
    setup->state_offsets_ppc.xer = offsetof(PPCState,xer);
    setup->state_offsets_ppc.fpscr = offsetof(PPCState,fpscr);
    setup->state_offsets_ppc.pvr = offsetof(PPCState,pvr);
    setup->state_offsets_ppc.pir = offsetof(PPCState,pir);
    setup->state_offsets_ppc.reserve_flag = offsetof(PPCState,reserve_flag);
    setup->state_offsets_ppc.reserve_state = offsetof(PPCState,reserve_state);
    setup->state_offsets_ppc.nia = offsetof(PPCState,nia);
    setup->state_offsets_ppc.timebase_handler = offsetof(PPCState,timebase_handler);
    setup->state_offsets_ppc.sc_handler = offsetof(PPCState,sc_handler);
    setup->state_offsets_ppc.trap_handler = offsetof(PPCState,trap_handler);
    setup->state_offsets_ppc.fres_lut = offsetof(PPCState,fres_lut);
    setup->state_offsets_ppc.frsqrte_lut = offsetof(PPCState,frsqrte_lut);
    setup->state_offset_chain_lookup = offsetof(PPCState,chain_lookup);
    setup->state_offset_branch_exit_flag = offsetof(PPCState,branch_exit_flag);
}

/*-----------------------------------------------------------------------*/

/**
 * call_guest_code:  Execute guest code at the given address by translating
 * it to native code and calling the generated code.  This function returns
 * when the guest code executes a "return-to-caller" instruction at the top
 * call level; in other words, this function acts like an indirect function
 * call to the given code.
 *
 * Log messages are captured using the log_capture interface.
 *
 * [Parameters]
 *     arch: Guest architecture (BINREC_ARCH_*).
 *     state: Processor state block.  Must be of the appropriate type for
 *         the guest architecture (see definitions in tests/execute.h).
 *     memory: Pointer to base of guest memory.
 *     address: Address at which to start executing code.
 *     configure_handle: Pointer to function to set up translation
 *         parameters, or NULL to leave parameters at the defaults.
 *     translated_code_callback: Pointer to function which will be called
 *         for each translated unit of code, or NULL for no callback.
 * [Return value]
 *     True if code was successfully executed; false if translation failed.
 */
extern bool call_guest_code(
    binrec_arch_t arch, void *state, void *memory, uint32_t address,
    void (*configure_handle)(binrec_t *handle),
    void (*translated_code_callback)(uint32_t address, void *code,
                                    long code_size));

/**
 * spawn_guest_code:  Execute guest code at the given address on a separate
 * thread, and return a thread identifier for wait_guest_code().  The guest
 * code is translated and executed as for call_guest_code(), except that
 * the log_capture interface is not used (since it is not thread-safe).
 *
 * Note that callback functions will be called in the thread executing
 * guest code rather than the thread in which this function was called.
 *
 * This function will fail if the runtime environment does not support some
 * form of threads.
 *
 * [Parameters]
 *     arch: Guest architecture (BINREC_ARCH_*).
 *     state: Processor state block.  Must be of the appropriate type for
 *         the guest architecture (see definitions in tests/execute.h).
 *     memory: Pointer to base of guest memory.
 *     address: Address at which to start executing code.
 *     configure_handle: Pointer to function to set up translation
 *         parameters, or NULL to leave parameters at the defaults.
 *         (RTL verification via binrec_enable_verify() is always enabled.)
 *     translated_code_callback: Pointer to function which will be called
 *         for each translated unit of code, or NULL for no callback.
 * [Return value]
 *     Thread identifier (non-NULL) if the thread was successfuly started;
 *     NULL if thread creation failed.
 */
extern void *spawn_guest_code(
    binrec_arch_t arch, void *state, void *memory, uint32_t address,
    void (*configure_handle)(binrec_t *handle),
    void (*translated_code_callback)(uint32_t address, void *code,
                                    long code_size));

/**
 * wait_guest_code:  Wait for completion of guest code started with
 * spawn_guest_code().
 *
 * [Parameters]
 *     thread: ID of thread to wait for.  Behavior is undefined if this is
 *         not a value returned by spawn_guest_code() or the thread has
 *         already been waited for.
 * [Return value]
 *     Return value of call_guest_code() for the requested guest code.
 */
extern bool wait_guest_code(void *thread);

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_EXECUTE_H
