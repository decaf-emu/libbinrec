/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_GUEST_PPC_INSN_COMMON_H
#define TESTS_GUEST_PPC_INSN_COMMON_H

#include "include/binrec.h"
#include <stdbool.h>

/* For RTL_DEBUG_OPTIMIZE definition. */
#include "src/rtl-internal.h"

/* PowerPC state block for translation tests.  We include initial padding
 * to verify that GPR references actually make use of the GPR offset. */
typedef struct PPCInsnTestState {
    uint8_t pad[256];
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
    uint64_t tb;
    uint64_t (*timebase_handler)(void *);
    void (*sc_handler)(void *);
    void (*trap_handler)(void *);
    void *(*chain_lookup)(void *, uint32_t);
    int (*branch_exit_flag)(void *, uint32_t);
    const uint16_t *fres_lut;
    const uint16_t *frsqrte_lut;
} PPCInsnTestState;

static const binrec_setup_t setup = {
    .guest = BINREC_ARCH_PPC_7XX,
#ifdef TEST_PPC_HOST_BIG_ENDIAN
    .host = BINREC_ARCH_PPC_7XX,  // To force host_little_endian to false.
#else
    .host = BINREC_ARCH_X86_64_SYSV,
#endif
    .state_offset_gpr = offsetof(PPCInsnTestState,gpr),
    .state_offset_fpr = offsetof(PPCInsnTestState,fpr),
    .state_offset_gqr = offsetof(PPCInsnTestState,gqr),
    .state_offset_cr = offsetof(PPCInsnTestState,cr),
    .state_offset_lr = offsetof(PPCInsnTestState,lr),
    .state_offset_ctr = offsetof(PPCInsnTestState,ctr),
    .state_offset_xer = offsetof(PPCInsnTestState,xer),
    .state_offset_fpscr = offsetof(PPCInsnTestState,fpscr),
    .state_offset_reserve_flag = offsetof(PPCInsnTestState,reserve_flag),
    .state_offset_reserve_state = offsetof(PPCInsnTestState,reserve_state),
    .state_offset_nia = offsetof(PPCInsnTestState,nia),
    .state_offset_timebase_handler = offsetof(PPCInsnTestState,timebase_handler),
    .state_offset_sc_handler = offsetof(PPCInsnTestState,sc_handler),
    .state_offset_trap_handler = offsetof(PPCInsnTestState,trap_handler),
    .state_offset_chain_lookup = offsetof(PPCInsnTestState,chain_lookup),
    .state_offset_branch_exit_flag = offsetof(PPCInsnTestState,branch_exit_flag),
    .state_offset_fres_lut = offsetof(PPCInsnTestState,fres_lut),
    .state_offset_frsqrte_lut = offsetof(PPCInsnTestState,frsqrte_lut),
};

#endif  // TESTS_GUEST_PPC_INSN_COMMON_H
