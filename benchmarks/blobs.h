/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BENCHMARKS_BLOBS_H
#define BENCHMARKS_BLOBS_H

#include <stdint.h>

/*
 * This header declares all of the binary objects ("blobs") and entry
 * points used by the benchmarking tool.  The actual data is located in
 * the "blobs" subdirectory; the scripts used to generate the files can
 * be found under the "etc" directory of the source tree.
 */

/* Data for a single blob.  As a convention, the guest stack pointer
 * will be initialized at the base address and grow downwards. */
typedef struct Blob {
    const void *data;  // The actual code.
    uint32_t size;     // Size of the code, in bytes.
    uint32_t reserve;  // Amount of guest memory to reserve.
    uint64_t base;     // Address in guest memory at which to load the code.
    uint32_t entry;    // Offset of the entry point from the base address.
} Blob;

/* Dhrystone */
extern const Blob ppc32_dhry_noopt;
extern const Blob ppc32_dhry_opt;

#endif  /* BENCHMARKS_BLOBS_H */