/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef LIBBINREC_HOST_X86_H
#define LIBBINREC_HOST_X86_H

#include "common.h"

struct RTLUnit;

/*************************************************************************/
/*************************************************************************/

/**
 * host_x86_translate:  Translate an RTL unit into x86-64 machine code.
 *
 * [Parameters]
 *     handle: Handle to use for translation.
 *     unit: RTLUnit to translate.
 * [Return value]
 *     True on success, false on error.
 */
#define host_x86_translate INTERNAL(host_x86_translate)
extern bool host_x86_translate(binrec_t *handle, struct RTLUnit *unit);

/*************************************************************************/
/*************************************************************************/

#endif  // LIBBINREC_HOST_X86_H
