/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_GUEST_PPC_GENERAL_HOST_BIG_ENDIAN_SETUP_H
#define TESTS_GUEST_PPC_GENERAL_HOST_BIG_ENDIAN_SETUP_H

#include "include/binrec.h"

static const binrec_setup_t setup = {
    .guest = BINREC_ARCH_PPC_7XX,
    .host = BINREC_ARCH_PPC_7XX,  // To force host_little_endian to false.
    .state_offset_gpr = 0x100,
    .state_offset_fpr = 0x180,
    .state_offset_gqr = 0x380,
    .state_offset_cr = 0x3A0,
    .state_offset_lr = 0x3A4,
    .state_offset_ctr = 0x3A8,
    .state_offset_xer = 0x3AC,
    .state_offset_fpscr = 0x3B0,
    .state_offset_reserve_flag = 0x3B4,
    .state_offset_reserve_state = 0x3B8,
    .state_offset_nia = 0x3BC,
    /* 0x3C0: unused */
    .state_offset_timebase_handler = 0x3C8,
    .state_offset_sc_handler = 0x3D0,
    .state_offset_trap_handler = 0x3D8,
    .state_offset_chain_lookup = 0x3E0,
    .state_offset_branch_callback = 0x3E8,
    .state_offset_fres_lut = 0x3F0,
    .state_offset_frsqrte_lut = 0x3F8,
};

#endif  // TESTS_GUEST_PPC_GENERAL_HOST_BIG_ENDIAN_SETUP_H
