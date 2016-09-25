/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef LIBBINREC_GUEST_PPC_H
#define LIBBINREC_GUEST_PPC_H

#include "common.h"

struct RTLUnit;

/*************************************************************************/
/*************************************************************************/

/**
 * guest_ppc_translate:  Translate 32-bit PowerPC machine code starting at
 * the given address into RTL.
 *
 * [Parameters]
 *     handle: Handle to use for translation.
 *     address: Address (in guest memory) of first instruction to translate.
 *     limit: Address (in guest memory) at which to terminate translation.
 *     unit: RTLUnit into which to write RTL instructions.
 * [Return value]
 *     True on success, false on error.
 */
#define guest_ppc_translate INTERNAL(guest_ppc_translate)
extern bool guest_ppc_translate(binrec_t *handle, uint32_t address,
                                uint32_t limit, struct RTLUnit *unit);

/*************************************************************************/
/*************************************************************************/

#endif  // LIBBINREC_GUEST_PPC_H
