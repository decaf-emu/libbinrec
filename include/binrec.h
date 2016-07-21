/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_H
#define BINREC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************/
/*********************** Data types and constants ************************/
/*************************************************************************/

/**
 * binrec_t:  Type of a translation handle.  This handle stores global
 * translation settings, such as functions to use for memory allocation.
 */
typedef struct binrec_t binrec_t;

/*************************************************************************/
/**************** Interface: Library version information *****************/
/*************************************************************************/

/**
 * binrec_version:  Return the version number of the library as a string
 * (for example, "1.2.3").
 *
 * [Return value]
 *     Library version number.
 */
extern const char *binrec_version(void);

/*************************************************************************/
/*************************************************************************/

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // BINREC_H
