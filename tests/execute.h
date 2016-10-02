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
    uint8_t reserve_flag;
    uint32_t reserve_state;
    uint32_t nia;
    uint64_t (*timebase_handler)(struct PPCState *);
    void (*sc_handler)(struct PPCState *);
    void (*trap_handler)(struct PPCState *);
} PPCState;

/*-----------------------------------------------------------------------*/

/**
 * call_guest_code:  Execute guest code at the given address by translating
 * it to native code and calling the generated code.  This function returns
 * when the guest code executes a "return-to-caller" instruction at the top
 * call level; in other words, this function acts like an indirect function
 * call to the given code.
 *
 * [Parameters]
 *     arch: Guest architecture (BINREC_ARCH_*).
 *     state: Processor state block.  Must be of the appropriate type for
 *         the guest architecture (see definitions in tests/execute.h).
 *     memory: Pointer to base of guest memory.
 *     address: Address at which to start executing code.
 * [Return value]
 *     True if code was successfully executed; false if translation failed.
 */
extern bool call_guest_code(binrec_arch_t arch, void *state, void *memory,
                            uint32_t address);

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_EXECUTE_H
