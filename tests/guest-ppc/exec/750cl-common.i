/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

/*
 * This file should be included in all tests that run the 750CL test
 * routine; it defines the data array for the routine itself and provides
 * appropriate handlers and setup code.  Call setup_750cl() to set up a
 * state block.
 */

#include "tests/ppc-lut.h"

/*************************************************************************/
/*************************** PowerPC test code ***************************/
/*************************************************************************/

/* These files are automatically generated with the contents of the test
 * code assembled under various configurations.  The test source
 * (ppc750cl.s) and the script used to generate this file
 * (gen-ppc750cl-bin.pl) live in the etc/ppc directory. */
#if defined(USE_750CL_NO_FPSCR_STATE)
    #include "tests/guest-ppc/exec/ppc750cl-nofpscr-bin.i"
#elif defined(USE_750CL_NO_RECIPROCAL_TABLES)
    #include "tests/guest-ppc/exec/ppc750cl-notables-bin.i"
#else
    #include "tests/guest-ppc/exec/ppc750cl-bin.i"
#endif

/* Constants for the test routine. */
#define PPC750CL_MEMORY_SIZE        0x2000000
#define PPC750CL_START_ADDRESS      0x1000000
#define PPC750CL_SCRATCH_ADDRESS    0x100000
#define PPC750CL_ERROR_LOG_ADDRESS  0x200000

/*************************************************************************/
/************************* Expected error lists **************************/
/*************************************************************************/

/*
 * Include the appropriate error sets in an array of FailureRecord, and
 * pass the array with its length to check_750cl_errors().
 */

typedef struct FailureRecord {
    uint32_t insn;
    uint32_t data[6];
} FailureRecord;

/* Expected errors for all tests. */
#define EXPECTED_ERRORS_COMMON  \
    /* stwcx. to different address, not handled by the translator. */   \
    {0x7C60212D, {0xFFFFFFFF,0xFFFFFFFF, 0x00000000,0x00000000}},       \
    /* lfd/paired-single data hazard. */                                \
    {0xC89F0008, {0x3FF00000,0x00000000, 0x00000000,0x00000000}},       \
    /* lfd/ps_merge misbehavior. */                                     \
    {0x10652C20, {0x00000000,0x00000000, 0x00000000,0x00000000}},       \
    /* lfd/ps_rsqrte misbehavior. */                                    \
    {0x11A01834, {0x00000000,0x00000000, 0x1F800041,0x00002000}},       \
    {0x11A02034, {0x7FF00000,0x00000000, 0x3F7FF400,0x84005000}}

/* Expected errors for the ASSUME_NO_SNAN optimization. */
#define EXPECTED_ERRORS_ASSUME_NO_SNAN  \
    /* lfs on an SNaN. */                                               \
    {0xC1040000, {0x7FFC0000,0x00000000, 0xFFFFFFFF,0x00000000}},       \
    /* lfsx on an SNaN. */                                              \
    {0x7D00242E, {0x7FFC0000,0x00000000, 0xFFFFFFFF,0x00000000}},       \
    /* fres on an SNaN. */                                              \
    {0xEC605830, {0xFFFC0000,0x00000000, 0x00011000,0x00000000}},       \
    {0xEC602830, {0xFFFC0000,0x00000000, 0x00011000,0x00000000}},       \
    {0xEC805830, {0xFFFC0000,0x00000000, 0x00011000,0x00000000}},       \
    {0xEC801030, {0x3FDFFF00,0x00000000, 0x00004000,0x00000000}},       \
    {0xEC605831, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0xEC601831, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* Paired-single operations on SNaNs (which were quieted when       \
     * loaded with lfs). */                                             \
    {0x10673C60, {0xFFE00000,0xFFE00000, 0x80000000,0x80000000}},       \
    {0xE07E0014, {0x7FD00000,0xFFE00000, 0x00000000,0x00000000}},       \
    {0xE07E8018, {0xFFE00000,0x3F800000, 0x00000000,0x00000000}},       \
    {0x10603890, {0xFFE00000,0xFFE00000, 0x80000000,0x20000000}},       \
    {0x10603850, {0x7FE00000,0x7FE00000, 0x80000000,0x20000000}},       \
    {0x10603A10, {0x7FE00000,0x7FE00000, 0x80000000,0x20000000}},       \
    {0x10603910, {0xFFE00000,0xFFE00000, 0x80000000,0x20000000}},       \
    /* Stale SNaN exception picked up by ps_cmpu0. */                   \
    {0x10000000, {0x20000000,0x00000000, 0xA1002000,0x00000000}},       \
    {0x10810000, {0x24000000,0x00000000, 0xA1004000,0x00000000}},       \
    {0x13880000, {0x00000008,0x00000000, 0xA1008000,0x00000000}},       \
    {0x13860000, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    /* ps_cmpu0 on SNaNs. */                                            \
    {0x13870000, {0x00000001,0x00000000, 0x00001000,0x00000000}},       \
    {0x13803800, {0x00000001,0x00000000, 0x00001000,0x00000000}},       \
    {0x12803800, {0x00000100,0x00000000, 0x00001000,0x00000000}},       \
    {0x12800000, {0x00000200,0x00000000, 0x00002000,0x00000000}},       \
    /* ps_cmpo0 on SNaNs. */                                            \
    {0x13870040, {0x00000001,0x00000000, 0xA0081000,0x00000000}},       \
    {0x13803840, {0x00000001,0x00000000, 0xA0081000,0x00000000}},       \
    {0x12803840, {0x00000100,0x00000000, 0x20081000,0x00000000}},       \
    {0x12800040, {0x00000200,0x00000000, 0x20082000,0x00000000}},       \
    {0x13070040, {0x00000010,0x00000000, 0xE0081080,0x00000000}},       \
    {0x13003840, {0x00000010,0x00000000, 0xE0081080,0x00000000}},       \
    {0x12003840, {0x00001000,0x00000000, 0x60081080,0x00000000}},       \
    {0x12000040, {0x00002000,0x00000000, 0x60082080,0x00000000}},       \
    /* ps_cmpu1 on SNaNs. */                                            \
    {0x13870080, {0x00000001,0x00000000, 0x00001000,0x00000000}},       \
    {0x13803880, {0x00000001,0x00000000, 0x00001000,0x00000000}},       \
    {0x12803880, {0x00000100,0x00000000, 0x00001000,0x00000000}},       \
    {0x12800080, {0x00000200,0x00000000, 0x00002000,0x00000000}},       \
    /* ps_cmpo1 on SNaNs. */                                            \
    {0x138700C0, {0x00000001,0x00000000, 0xA0081000,0x00000000}},       \
    {0x138038C0, {0x00000001,0x00000000, 0xA0081000,0x00000000}},       \
    {0x128038C0, {0x00000100,0x00000000, 0x20081000,0x00000000}},       \
    {0x128000C0, {0x00000200,0x00000000, 0x20082000,0x00000000}},       \
    {0x130700C0, {0x00000010,0x00000000, 0xE0081080,0x00000000}},       \
    {0x130038C0, {0x00000010,0x00000000, 0xE0081080,0x00000000}},       \
    {0x120038C0, {0x00001000,0x00000000, 0x60081080,0x00000000}},       \
    {0x120000C0, {0x00002000,0x00000000, 0x60082080,0x00000000}},       \
    /* ps_add on SNaNs. */                                              \
    {0x1061582A, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B082A, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x1061702A, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x1078082A, {0x41500000,0x40000000, 0x4A800002,0x00004000}},       \
    {0x1065202A, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x1065682A, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x106D282A, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x1061582B, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x1061182B, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_sum0 on SNaNs. */                                             \
    {0x10615814, {0xFFFC0000,0x00000000, 0x00000000,0x00011000}},       \
    {0x106B0854, {0xFFFC0000,0x00000000, 0x3F800000,0x00011000}},       \
    {0x10615894, {0xFFFC0000,0x00000000, 0x40000000,0x00011000}},       \
    {0x1061C094, {0x41500000,0x40000000, 0x40000000,0x00004000}},       \
    {0x10642AD4, {0x40080000,0x00000000, 0xFFE00000,0x00004000}},       \
    {0x10615815, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10611895, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_sum1 on SNaNs. */                                             \
    {0x10615816, {0x00000000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B0856, {0x3FF00000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10615896, {0x40000000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x1061C096, {0x40000000,0x00000000, 0x4A800002,0x00004000}},       \
    {0x10642AD6, {0xFFFC0000,0x00000000, 0x40400000,0x00004000}},       \
    /* ps_sum1 (Rc=1) on SNaNs. */                                      \
    {0x10615817, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10611897, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_sub on SNaNs. */                                              \
    {0x10615828, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B0828, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10617028, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106E0828, {0x41500000,0x00000000, 0x4A800000,0x00004000}},       \
    {0x10652028, {0xFFFC0000,0x00000000, 0xFF800000,0x92031000}},       \
    {0x10656828, {0xFFFC0000,0x00000000, 0xFF800000,0x92031000}},       \
    {0x106D2828, {0xFFF00000,0x00000000, 0xFFE00000,0x92029000}},       \
    {0x10615829, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10611829, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_mul on SNaNs. */                                              \
    {0x106102F2, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B0072, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106103B2, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x107C00B2, {0x41500000,0x00000000, 0x4A800000,0x00004000}},       \
    {0x10650132, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x10650372, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x106D0172, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x106102F3, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x106100F3, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_muls0 on SNaNs. */                                            \
    {0x10650118, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10650358, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x10650119, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10650359, {0xFFFFFFFF,0x00000000, 0x09000000,0x00000000}},       \
    /* ps_muls1 on SNaNs. */                                            \
    {0x1065011A, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x1065035A, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x1065011B, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x1065035B, {0xFFFFFFFF,0x00000000, 0x09000000,0x00000000}},       \
    /* ps_div on SNaNs. */                                              \
    {0x10615824, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B0824, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10617024, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10780824, {0x41500000,0x00000000, 0x4A800000,0x00004000}},       \
    {0x10652024, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x10656824, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x106D2824, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x10615825, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10611825, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_madd on SNaNs. */                                             \
    {0x10605B7A, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106112FA, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106158BA, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B107A, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106113BA, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x107C08BA, {0x41500000,0x40000000, 0x4A800002,0x00004000}},       \
    {0x1065693A, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x1065237A, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x106D217A, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x106112FB, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x106110FB, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_madds0 on SNaNs. */                                           \
    {0x1065291C, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10650B5C, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x1065291D, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10650B5D, {0xFFFFFFFF,0x00000000, 0x09000000,0x00000000}},       \
    /* ps_madds1 on SNaNs. */                                           \
    {0x1065291E, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10650B5E, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x1065291F, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10650B5F, {0xFFFFFFFF,0x00000000, 0x09000000,0x00000000}},       \
    /* ps_msub on SNaNs. */                                             \
    {0x10605B78, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106112F8, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106158B8, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B1078, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106113B8, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10610FB8, {0x41500000,0x00000000, 0x4A800000,0x00004000}},       \
    {0x10656938, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x10652378, {0xFFFC0000,0x00000000, 0x7F800000,0x92031000}},       \
    {0x106D2178, {0x7FF00000,0x00000000, 0xFFE00000,0x92025000}},       \
    {0x106112F9, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x106110F9, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_nmadd on SNaNs. */                                            \
    {0x10605B7E, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106112FE, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106158BE, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B107E, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106113BE, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x107C08BE, {0xC1500000,0x40000000, 0xCA800002,0x00008000}},       \
    {0x1065693E, {0xFFFC0000,0x00000000, 0xFF800000,0x92031000}},       \
    {0x1065237E, {0xFFFC0000,0x00000000, 0xFF800000,0x92031000}},       \
    {0x106D217E, {0xFFF00000,0x00000000, 0xFFE00000,0x92029000}},       \
    {0x106112FF, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x106110FF, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_nmsub on SNaNs. */                                            \
    {0x10605B7C, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106112FC, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106158BC, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106B107C, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x106112FC, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x107E087C, {0xC1500000,0x00000000, 0xCA800000,0x00008000}},       \
    {0x1065693C, {0xFFFC0000,0x00000000, 0xFF800000,0x92031000}},       \
    {0x1065237C, {0xFFFC0000,0x00000000, 0xFF800000,0x92031000}},       \
    {0x106D217C, {0xFFF00000,0x00000000, 0xFFE00000,0x92029000}},       \
    {0x106112FD, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x106110FD, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_res on SNaNs. */                                              \
    {0x10605830, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10602830, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10607030, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10607830, {0x3FDFFF00,0x00000000, 0x3EFFF800,0x00004000}},       \
    {0x11C01830, {0xFFFC0000,0x00000000, 0x7F7FFFFF,0x90031000}},       \
    {0x11C02030, {0xFFFC0000,0x00000000, 0x7F7FFFFF,0x90031000}},       \
    {0x11C02830, {0x47EFFFFF,0xE0000000, 0xFFE00000,0x90024000}},       \
    {0x10605831, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10601831, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_rsqrte on SNaNs. */                                           \
    {0x10605834, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10602834, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10607034, {0xFFFC0000,0x00000000, 0xFFE00000,0x00011000}},       \
    {0x10607834, {0x3FFFFE80,0x00000000, 0x3FFFF400,0x00004000}},       \
    {0x11C01834, {0xFFFC0000,0x00000000, 0x3F34FD00,0x00011000}},       \
    {0x11C02034, {0xFFFC0000,0x00000000, 0x3F34FD00,0x00011000}},       \
    {0x11C02834, {0x3FE69FA0,0x00000000, 0xFFE00000,0x00004000}},       \
    {0x10605835, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    {0x10601835, {0xFFFFFFFF,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_sel with Rc=1 after an SNaN operation. */                     \
    {0x1062006F, {0x00000000,0xFFFFFFFF, 0xFFFFFFFF,0xFFFFFFFF}}

/* Expected errors for the ASSUME_NO_SNAN optimization without FAST_FTFS. */
#define EXPECTED_ERRORS_ASSUME_NO_SNAN_NO_FAST_STFS  \
    /* Exception raised during single-to-double conversion in alias store \
     * after the preceding ps_add, detected at this instruction. */     \
    {0x1067382A, {0x7FF00000,0x00000000, 0x7F800000,0xB3025000}}

/* Expected errors for the ASSUME_NO_SNAN optimization with FAST_STFS. */
#define EXPECTED_ERRORS_ASSUME_NO_SNAN_FAST_STFS  \
    /* stfs on an SNaN. */                                              \
    {0xD1040000, {0x7FE00000,0x00000000, 0xFFFFFFFF,0x00000000}},       \
    /* stfsx on an SNaN. */                                             \
    {0x7D00252E, {0x7FE00000,0x00000000, 0xFFFFFFFF,0x00000000}}

/* Expected errors for the FAST_FMADDS optimization. */
#define EXPECTED_ERRORS_FAST_FMADDS  \
    /* Rounding error with fmadds. */                                   \
    {0xEC637B7A, {0x47E00000,0x00000000, 0x82024000,0x00000000}},       \
    {0xEC637BBA, {0x36B00000,0x00000000, 0x8A034000,0x00000000}},       \
    /* Rounding error with ps_madd. */                                  \
    {0x10637B7A, {0x47E00000,0x00000000, 0x7F000000,0x82024000}},       \
    {0x10637BBA, {0x36B00000,0x00000000, 0x00000002,0x8A034000}}

/* Expected errors for the FAST_FMULS optimization. */
#define EXPECTED_ERRORS_FAST_FMULS  \
    /* frC rounding tests for fmuls. */                                 \
    {0xEC6D00F2, {0x40000000,0x00000000, 0x82024000,0x00000000}},       \
    {0xEDA40372, {0x3CD00000,0x00000000, 0x82024000,0x00000000}},       \
    {0xEC640372, {0xBCD00000,0x00000000, 0x82028000,0x00000000}},       \
    {0xEDA100F2, {0x3FF00000,0x00000000, 0x82024000,0x00000000}},       \
    {0xEC830132, {0x3E5FFFFF,0xE0000000, 0x82024000,0x00000000}},       \
    {0xEDA30132, {0xBE5FFFFF,0xE0000000, 0x82028000,0x00000000}},       \
    /* frC rounding tests for fmadds. */                                \
    {0xEC6D20FA, {0x40000000,0x00000000, 0x00004000,0x00000000}},       \
    {0xEC81693A, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    /* frC rounding tests for fmsubs. */                                \
    {0xEC6D20F8, {0x40000000,0x00000000, 0x00004000,0x00000000}},       \
    {0xEC812138, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    /* frC rounding tests for fnmadds. */                               \
    {0xEC6D20FE, {0xC0000000,0x00000000, 0x00008000,0x00000000}},       \
    {0xEC81693E, {0x80000000,0x00000000, 0x00012000,0x00000000}},       \
    /* frC rounding tests for fnmsubs. */                               \
    {0xEC6D20FC, {0xC0000000,0x00000000, 0x00008000,0x00000000}},       \
    {0xEC81213C, {0x80000000,0x00000000, 0x00012000,0x00000000}},       \
    /* frC rounding tests for ps_mul. */                                \
    {0x106D00F2, {0x40000000,0x00000000, 0x00000000,0x82024000}},       \
    {0x106403B2, {0x3CD00000,0x00000000, 0x00000000,0x82024000}},       \
    {0x10640372, {0xBCD00000,0x00000000, 0x00000000,0x82028000}},       \
    {0x106100F2, {0x3FF00000,0x00000000, 0x00000000,0x82024000}},       \
    {0x106303B2, {0x3E5FFFFF,0xE0000000, 0x00000000,0x82024000}},       \
    {0x106D03F2, {0xBE5FFFFF,0xE0000000, 0x00000000,0x82028000}},       \
    /* frC rounding tests for ps_madd. */                               \
    {0x106D20FA, {0x40000000,0x00000000, 0x00000000,0x00004000}},       \
    {0x1061713A, {0x00000000,0x00000000, 0x00000000,0x00002000}},       \
    /* frC rounding tests for ps_msub. */                               \
    {0x106D20F8, {0x40000000,0x00000000, 0x00000000,0x00004000}},       \
    {0x10612138, {0x00000000,0x00000000, 0x00000000,0x00002000}},       \
    /* frC rounding tests for ps_nmadd. */                              \
    {0x106D20FE, {0xC0000000,0x00000000, 0x80000000,0x00008000}},       \
    {0x1061713E, {0x80000000,0x00000000, 0x80000000,0x00012000}},       \
    /* frC rounding tests for ps_nmsub. */                              \
    {0x106D20FC, {0xC0000000,0x00000000, 0x80000000,0x00008000}},       \
    {0x1061213C, {0x80000000,0x00000000, 0x80000000,0x00012000}}

/* Expected errors for the FAST_STFS optimization. */
#define EXPECTED_ERRORS_FAST_STFS  \
    {0xD0640020, {0x3FF55555,0x55555555, 0x3FAAAAAB,0x00000000}},       \
    {0xD0840020, {0x380FFFFF,0xFFFFFFFF, 0x00800000,0x00000000}},       \
    {0xD0A40020, {0x50123456,0x789ABCDE, 0x7F800000,0x00000000}}

/* Expected errors for the FNMADD_ZERO_SIGN optimization. */
#define EXPECTED_ERRORS_FNMADD_ZERO_SIGN  \
    /* fnmadd expecting a negative zero. */                             \
    {0xFC64007E, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    {0xFC60013E, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    {0xFC60207E, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    /* fnmadds with double-precision input expecting a negative zero. */ \
    {0xEC84687E, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    /* fnmsubs with double-precision input expecting a negative zero. */ \
    {0xEC84207C, {0x00000000,0x00000000, 0x00002000,0x00000000}},       \
    /* ps_nmadd expecting a negative zero. */                           \
    {0x1064007E, {0x00000000,0x00000000, 0x00000000,0x00002000}},       \
    {0x1060013E, {0x00000000,0x00000000, 0x00000000,0x00002000}},       \
    {0x1060207E, {0x00000000,0x00000000, 0x00000000,0x00002000}},       \
    /* ps_nmadd with double-precision input expecting a negative zero. */ \
    {0x1064707E, {0x00000000,0x00000000, 0x80000000,0x00002000}},       \
    /* ps_nmsub with double-precision input expecting a negative zero. */ \
    {0x1064207C, {0x00000000,0x00000000, 0x00000000,0x00002000}}

/* Expected errors for the IGNORE_FPSCR_VXFOO optimization. */
#define EXPECTED_ERRORS_IGNORE_FPSCR_VXFOO  \
    /* fcmpo expecting VXVC. */                                         \
    {0xFF850040, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0xFF802840, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    /* fcmpo expecting VXSNAN+VXVC. */                                  \
    {0xFF860040, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0xFF803040, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0xFE803040, {0x00000100,0x00000000, 0x21001000,0x00000000}},       \
    {0xFE800040, {0x00000200,0x00000000, 0x21002000,0x00000000}},       \
    /* fadd expecting VXISI. */                                         \
    {0xFC69682A, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC6D482A, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fsub expecting VXISI. */                                         \
    {0xFC694828, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC6D6828, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fmul expecting VXIMZ. */                                         \
    {0xFC690032, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600372, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600272, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fdiv expecting VXIDI or VXZDZ. */                                \
    {0xFC694824, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600024, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC696824, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC642024, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fmadd expecting VXIMZ, VXIMZ+VXSNAN, or VXISI. */                \
    {0xFC6D48BA, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69083A, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69203A, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69207A, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC60227A, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC61227A, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC605B7A, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600A7A, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC6968BA, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fmsub expecting VXIMZ, VXIMZ+VXSNAN, or VXISI. */                \
    {0xFC6D68B8, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC690838, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC694838, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC605B78, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600A78, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC6948B8, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fnmadd expecting VXIMZ, VXIMZ+VXSNAN, or VXISI. */               \
    {0xFC6D48BE, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69083E, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69203E, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC605B7E, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600A7E, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC6968BE, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fnmsub expecting VXIMZ, VXIMZ+VXSNAN, or VXISI. */               \
    {0xFC6D68BC, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69083C, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC69483C, {0xFFF80000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC605B7C, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    {0xFC600A7C, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC6948BC, {0x41500000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fctiw expecting VXCVI without VXSNAN. */                         \
    {0xFC60D01C, {0x7FFFFFFF,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFC60D81C, {0x80000000,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFC60E01C, {0x7FFFFFFF,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFC60E81C, {0x80000000,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFC60501C, {0x80000000,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFC60701C, {0x7FFFFFFF,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFDE0781C, {0x80000000,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFCA0701C, {0x7FFFFFFF,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFDC0781C, {0x80000000,0x00000000, 0xA1000100,0x00000000}},       \
    {0xFC80D01C, {0x3FF00000,0x00000000, 0xE1000100,0x00000000}},       \
    {0xFC80E01C, {0x3FF00000,0x00000000, 0xE1000100,0x00000000}},       \
    {0xFC80501C, {0x3FF00000,0x00000000, 0xE1000100,0x00000000}},       \
    /* ps_cmpo0 expecting VXVC. */                                      \
    {0x13860040, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0x13803040, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    /* ps_cmpo0 expecting VXSNAN+VXVC. */                               \
    {0x13870040, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0x13803840, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0x12803840, {0x00000100,0x00000000, 0x21001000,0x00000000}},       \
    {0x12800040, {0x00000200,0x00000000, 0x21002000,0x00000000}},       \
    /* ps_cmpo1 expecting VXVC. */                                      \
    {0x138600C0, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0x138030C0, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    /* ps_cmpo1 expecting VXSNAN+VXVC. */                               \
    {0x138700C0, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0x138038C0, {0x00000001,0x00000000, 0xA1001000,0x00000000}},       \
    {0x128038C0, {0x00000100,0x00000000, 0x21001000,0x00000000}},       \
    {0x128000C0, {0x00000200,0x00000000, 0x21002000,0x00000000}},       \
    /* ps_add expecting VXISI. */                                       \
    {0x1069682A, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x106D482A, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    /* ps_sum0 expecting VXISI. */                                      \
    {0x10696814, {0xFFF80000,0x00000000, 0x00000000,0xA1011000}},       \
    {0x106D4814, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106D2914, {0xFFF80000,0x00000000, 0x3F800000,0xA1011000}},       \
    /* ps_sum1 expecting VXISI. */                                      \
    {0x10696816, {0x00000000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x106D4816, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106D2916, {0x3FF00000,0x00000000, 0xFFC00000,0xA1011000}},       \
    /* ps_sub expecting VXISI. */                                       \
    {0x10694828, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x106D6828, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    /* ps_mul expecting VXIMZ. */                                       \
    {0x10690032, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10600372, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10600272, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    /* ps_div expecting VXIDI, VXZDZ, or both. */                       \
    {0x10694824, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10600024, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10696824, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x10642024, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x10631824, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10652824, {0x3FF00000,0x00000000, 0x3F800000,0xE1000000}},       \
    /* ps_madd expecting some combination of VXIMZ (possibly with       \
     * VXSNAN) and VXISI. */                                            \
    {0x106D48BA, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x1069083A, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x1069203A, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10605B7A, {0xFFFC0000,0x00000000, 0xFFE00000,0xA1011000}},       \
    {0x10600A7A, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106968BA, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106D48FA, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    /* ps_msub expecting VXIMZ[+VXSNAN] and/or VXISI. */                \
    {0x106D68B8, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10690838, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10694838, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10605B78, {0xFFFC0000,0x00000000, 0xFFE00000,0xA1011000}},       \
    {0x10600A78, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106948B8, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    /* ps_nmadd expecting VXIMZ[+VXSNAN] and/or VXISI. */               \
    {0x106D48BE, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x1069083E, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x1069203E, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10605B7E, {0xFFFC0000,0x00000000, 0xFFE00000,0xA1011000}},       \
    {0x10600A7E, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106968BE, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    /* ps_nmsub expecting VXIMZ[+VXSNAN] and/or VXISI. */               \
    {0x106D68BC, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x1069083C, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x1069483C, {0xFFF80000,0x00000000, 0xFFC00000,0xA1011000}},       \
    {0x10605B7C, {0xFFFC0000,0x00000000, 0xFFE00000,0xA1011000}},       \
    {0x10600A7C, {0x41500000,0x00000000, 0x4A800000,0xE1000000}},       \
    {0x106948BC, {0x41500000,0x00000000, 0x4A800000,0xE1000000}}

/* Expected errors for the NATIVE_IEEE_NAN optimization. */
#define EXPECTED_ERRORS_NATIVE_IEEE_NAN  \
    /* fadd returns default QNaN from subtraction of infinities. */     \
    {0xFC69682A, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    /* fsub returns default QNaN from subtraction of infinities. */     \
    {0xFC694828, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    /* fmul returns default QNaN from infinity * 0. */                  \
    {0xFC690032, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC600372, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    /* fdiv returns default QNaN from infinity / infinity. */           \
    {0xFC694824, {0xFFF80000,0x00000000, 0xA0411000,0x00000000}},       \
    /* fdiv returns default QNaN from 0 / 0. */                         \
    {0xFC600024, {0xFFF80000,0x00000000, 0xA0211000,0x00000000}},       \
    /* fmadd returns default QNaN from inf - inf or inf * 0. */         \
    {0xFC6D48BA, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    {0xFC69083A, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC69203A, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC69207A, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    {0xFC60227A, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC61227A, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    /* fmsub returns default QNaN from inf - inf or inf * 0. */         \
    {0xFC6D68B8, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    {0xFC690838, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC694838, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    /* fnmadd returns default QNaN from inf - inf or inf * 0. */        \
    {0xFC6D48BE, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    {0xFC69083E, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC69203E, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    /* fnmsub returns default QNaN from inf - inf or inf * 0. */        \
    {0xFC6D68BC, {0xFFF80000,0x00000000, 0xA0811000,0x00000000}},       \
    {0xFC69083C, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    {0xFC69483C, {0xFFF80000,0x00000000, 0xA0111000,0x00000000}},       \
    /* ps_add returns default QNaN from subtraction of infinities. */   \
    {0x1069682A, {0xFFF80000,0x00000000, 0xFFC00000,0xA0811000}},       \
    /* ps_sum0 returns default QNaN from subtraction of infinities. */  \
    {0x10696814, {0xFFF80000,0x00000000, 0x00000000,0xA0811000}},       \
    {0x106D2914, {0xFFF80000,0x00000000, 0x3F800000,0xA0811000}},       \
    /* ps_sum1 returns default QNaN from subtraction of infinities. */  \
    {0x10696816, {0x00000000,0x00000000, 0xFFC00000,0xA0811000}},       \
    {0x106D2916, {0x3FF00000,0x00000000, 0xFFC00000,0xA0811000}},       \
    /* ps_sub returns default QNaN from subtraction of infinities. */   \
    {0x10694828, {0xFFF80000,0x00000000, 0xFFC00000,0xA0811000}},       \
    /* ps_mul returns default QNaN from infinity * 0. */                \
    {0x10690032, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    {0x10600372, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    /* ps_div returns default QNaN from infinity / infinity or 0 / 0. */ \
    {0x10694824, {0xFFF80000,0x00000000, 0xFFC00000,0xA0411000}},       \
    {0x10600024, {0xFFF80000,0x00000000, 0xFFC00000,0xA0211000}},       \
    {0x10631824, {0xFFF80000,0x00000000, 0xFFC00000,0xA0611000}},       \
    /* ps_madd returns default QNaN from inf - inf or inf * 0. */       \
    {0x106D48BA, {0xFFF80000,0x00000000, 0xFFC00000,0xA0811000}},       \
    {0x1069083A, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    {0x1069203A, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    {0x106D48FA, {0xFFF80000,0x00000000, 0xFFC00000,0xA0911000}},       \
    /* ps_msub returns default QNaN from inf - inf or inf * 0. */       \
    {0x106D68B8, {0xFFF80000,0x00000000, 0xFFC00000,0xA0811000}},       \
    {0x10690838, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    {0x10694838, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    /* ps_nmadd returns default QNaN from inf - inf or inf * 0. */      \
    {0x106D48BE, {0xFFF80000,0x00000000, 0xFFC00000,0xA0811000}},       \
    {0x1069083E, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    {0x1069203E, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    /* ps_nmsub returns default QNaN from inf - inf or inf * 0. */      \
    {0x106D68BC, {0xFFF80000,0x00000000, 0xFFC00000,0xA0811000}},       \
    {0x1069083C, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}},       \
    {0x1069483C, {0xFFF80000,0x00000000, 0xFFC00000,0xA0111000}}

/* Expected errors for the NATIVE_IEEE_NAN optimization without
 * NO_FPSCR_STATE. */
#define EXPECTED_ERRORS_NATIVE_IEEE_NAN_NO_NO_FPSCR_STATE  \
    /* fmadd chooses frB NaN (addend) over frC NaN (multiplier). */     \
    {0xFC6052FA, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    /* fmsub chooses frB NaN (addend) over frC NaN (multiplier). */     \
    {0xFC6052F8, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    /* fnmadd chooses frB NaN (addend) over frC NaN (multiplier). */    \
    {0xFC6052FE, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}},       \
    /* fnmsub chooses frB NaN (addend) over frC NaN (multiplier). */    \
    {0xFC6052FC, {0xFFFC0000,0x00000000, 0xA1011000,0x00000000}}

/* Expected errors for the NATIVE_IEEE_UNDERFLOW optimization. */
#define EXPECTED_ERRORS_NATIVE_IEEE_UNDERFLOW  \
    /* fadds result is tiny before rounding. */                         \
    {0xEC63682A, {0x38100000,0x00000000, 0x82024000,0x00000000}},       \
    /* fsubs result is tiny before rounding. */                         \
    {0xEC636828, {0x38100000,0x00000000, 0x82024000,0x00000000}},       \
    /* fmadd result is tiny before rounding. */                         \
    {0xFC6320FA, {0x80100000,0x00000000, 0x82028000,0x00000000}},       \
    /* fmadds result is tiny before rounding. */                        \
    {0xEC6320FA, {0xB8100000,0x00000000, 0x82028000,0x00000000}},       \
    {0xEC64193A, {0xB8100000,0x00000000, 0x82028000,0x00000000}},       \
    /* fmsub result is tiny before rounding. */                         \
    {0xFC6318F8, {0x80100000,0x00000000, 0x82028000,0x00000000}},       \
    /* fmsubs result is tiny before rounding. */                        \
    {0xEC6318F8, {0xB8100000,0x00000000, 0x82028000,0x00000000}},       \
    {0xEC641938, {0xB8100000,0x00000000, 0x82028000,0x00000000}},       \
    /* fnmadd result is tiny before rounding. */                        \
    {0xFC6320FE, {0x00100000,0x00000000, 0x82024000,0x00000000}},       \
    /* fnmadds result is tiny before rounding. */                       \
    {0xEC6320FE, {0x38100000,0x00000000, 0x82024000,0x00000000}},       \
    {0xEC64193E, {0x38100000,0x00000000, 0x82024000,0x00000000}},       \
    /* fnmsub result is tiny before rounding. */                        \
    {0xFC6318FC, {0x00100000,0x00000000, 0x82024000,0x00000000}},       \
    /* fnmsubs result is tiny before rounding. */                       \
    {0xEC6318FC, {0x38100000,0x00000000, 0x82024000,0x00000000}},       \
    {0xEC64193C, {0x38100000,0x00000000, 0x82024000,0x00000000}},       \
    /* ps_madd result is tiny before rounding. */                       \
    {0x106320FA, {0xB8100000,0x00000000, 0x80800000,0x82028000}},       \
    {0x1064193A, {0xB8100000,0x00000000, 0x80800000,0x82028000}},       \
    /* ps_msub result is tiny before rounding. */                       \
    {0x106318F8, {0xB8100000,0x00000000, 0x80800000,0x82028000}},       \
    {0x10641938, {0xB8100000,0x00000000, 0x80800000,0x82028000}},       \
    /* ps_nmadd result is tiny before rounding. */                      \
    {0x106320FE, {0x38100000,0x00000000, 0x00800000,0x82024000}},       \
    {0x1064193E, {0x38100000,0x00000000, 0x00800000,0x82024000}},       \
    /* ps_nmsub result is tiny before rounding. */                      \
    {0x106318FC, {0x38100000,0x00000000, 0x00800000,0x82024000}},       \
    {0x1064193C, {0x38100000,0x00000000, 0x00800000,0x82024000}}

/* Expected errors for the NATIVE_RECIPROCAL optimization (assuming the
 * no-reciprocal-tables version of the test code). */
#define EXPECTED_ERRORS_NATIVE_RECIPROCAL  \
    /* fres expecting HUGE_VALF instead of infinity. */                 \
    {0xEC606830, {0x7FF00000,0x00000000, 0x90025000,0x00000000}},       \
    {0xEC806830, {0xFFF00000,0x00000000, 0x90029000,0x00000000}},       \
    /* ps_res expecting HUGE_VALF instead of infinity. */               \
    {0x10606830, {0x7FF00000,0x00000000, 0x7F800000,0x92025000}},       \
    {0x10806830, {0xFFF00000,0x00000000, 0xFF800000,0x92029000}},       \
    {0x11C01830, {0xFFFC0000,0x00000000, 0x7F800000,0xB1031000}},       \
    {0x11C02830, {0x3FF00000,0x00000000, 0x3F800000,0xF1025000}},       \
    /* ps_rsqrte expecting exact table result. */                       \
    {0x11C01834, {0xFFFC0000,0x00000000, 0x3F3504F3,0xA1011000}},       \
    /* lfd/ps_rsqrte misbehavior. */                                    \
    {0x11A01834, {0x00000000,0x00000000, 0x1F800001,0x82022000}},       \
    {0x11A02034, {0x7FF00000,0x00000000, 0x3F800000,0x84005000}}

/* Expected errors for the NO_FPSCR_STATE optimization. */
#define EXPECTED_ERRORS_NO_FPSCR_STATE  \
    /* frsp not aborted by enabled exception. */                        \
    {0xFC805818, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC601819, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fadd not aborted by enabled exception. */                        \
    {0xFC6B082A, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC6D482A, {0xFFF80000,0x00000000, 0xE0800000,0x00000000}},       \
    {0xFC61182B, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fsub not aborted by enabled exception. */                        \
    {0xFC6B0828, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC6D6828, {0xFFF80000,0x00000000, 0xE0800000,0x00000000}},       \
    {0xFC611829, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fmul not aborted by enabled exception. */                        \
    {0xFC6B0072, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC600272, {0xFFF80000,0x00000000, 0xE0100000,0x00000000}},       \
    {0xFC6100F3, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fdiv not aborted by enabled exception. */                        \
    {0xFC6B0824, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC696824, {0xFFF80000,0x00000000, 0xE0400000,0x00000000}},       \
    {0xFC642024, {0xFFF80000,0x00000000, 0xE0200000,0x00000000}},       \
    {0xFC610024, {0x7FF00000,0x00000000, 0xC4000000,0x00000000}},       \
    {0xFC611825, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fmadd not aborted by enabled exception. */                       \
    {0xFC6B107A, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC600A7A, {0xFFF80000,0x00000000, 0xE0100000,0x00000000}},       \
    {0xFC6968BA, {0xFFF80000,0x00000000, 0xE0800000,0x00000000}},       \
    {0xFC6110FB, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fmsub not aborted by enabled exception. */                       \
    {0xFC6B1078, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC600A78, {0xFFF80000,0x00000000, 0xE0100000,0x00000000}},       \
    {0xFC6948B8, {0xFFF80000,0x00000000, 0xE0800000,0x00000000}},       \
    {0xFC6110F9, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fnmadd not aborted by enabled exception. */                      \
    {0xFC6B107E, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC600A7E, {0xFFF80000,0x00000000, 0xE0100000,0x00000000}},       \
    {0xFC6968BE, {0xFFF80000,0x00000000, 0xE0800000,0x00000000}},       \
    {0xFC6110FF, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fnmsub not aborted by enabled exception. */                      \
    {0xFC6B107C, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC600A7C, {0xFFF80000,0x00000000, 0xE0100000,0x00000000}},       \
    {0xFC6948BC, {0xFFF80000,0x00000000, 0xE0800000,0x00000000}},       \
    {0xFC6110FD, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* fctiw not aborted by enabled exception. */                       \
    {0xFC80D01C, {0xFFF80000,0x7FFFFFFF, 0xE0000100,0x00000000}},       \
    {0xFC80E01C, {0xFFF80000,0x7FFFFFFF, 0xE0000100,0x00000000}},       \
    {0xFC80501C, {0xFFF80000,0x80000000, 0xE0000100,0x00000000}},       \
    {0xFC80581C, {0xFFF80000,0x80000000, 0xE1000100,0x00000000}},       \
    {0xFC60181D, {0xFFF80000,0x80000000, 0xE1000100,0x00000000}},       \
    /* fres not aborted by enabled exception. */                        \
    {0xEC602830, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xEC602030, {0x7FF00000,0x00000000, 0xC4000000,0x00000000}},       \
    {0xEC601831, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* frsqrte not aborted by enabled exception. */                     \
    {0xFC602834, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    {0xFC60D834, {0x7FF80000,0x00000000, 0xE0000200,0x00000000}},       \
    {0xFC602034, {0xFFF00000,0x00000000, 0xC4000000,0x00000000}},       \
    {0xFC601835, {0xFFFC0000,0x00000000, 0xE1000000,0x00000000}},       \
    /* ps_add not aborted by enabled exception. */                      \
    {0x106B082A, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x106D482A, {0xFFF80000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x1065682A, {0xFFFC0000,0x00000000, 0x7F800000,0xF3020000}},       \
    {0x106D282A, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x1061182B, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_sum0 not aborted by enabled exception. */                     \
    {0x106B0854, {0xFFFC0000,0x00000000, 0x3F800000,0xE1000000}},       \
    {0x106D4814, {0xFFF80000,0x00000000, 0x00000000,0xE0800000}},       \
    {0x10611895, {0xFFFC0000,0x00000000, 0x40000000,0xE1000000}},       \
    /* ps_sum1 not aborted by enabled exception. */                     \
    {0x106B0856, {0x3FF00000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x106D4816, {0x00000000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x10611897, {0x40000000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_sub not aborted by enabled exception. */                      \
    {0x106B0828, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x106D6828, {0xFFF80000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x10656828, {0xFFFC0000,0x00000000, 0xFF800000,0xF3020000}},       \
    {0x106D2828, {0xFFF00000,0x00000000, 0xFFE00000,0xF3029000}},       \
    {0x10611829, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_mul not aborted by enabled exception. */                      \
    {0x106B0072, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10600272, {0xFFF80000,0x00000000, 0xFFC00000,0xE0100000}},       \
    {0x10650372, {0xFFFC0000,0x00000000, 0x7F800000,0xF3020000}},       \
    {0x106D0172, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x106100F3, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_muls0 not aborted by enabled exception. */                    \
    {0x10650358, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x10650359, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    /* ps_muls1 not aborted by enabled exception. */                    \
    {0x1065035A, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x1065035B, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    /* ps_div not aborted by enabled exception. */                      \
    {0x106B0824, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10696824, {0xFFF80000,0x00000000, 0xFFC00000,0xE0400000}},       \
    {0x10642024, {0xFFF80000,0x00000000, 0xFFC00000,0xE0200000}},       \
    {0x10610024, {0x7FF00000,0x00000000, 0x7F800000,0xC4000000}},       \
    {0x10656824, {0xFFFC0000,0x00000000, 0x7F800000,0xF3020000}},       \
    {0x106D2824, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x10652824, {0xFFF80000,0x00000000, 0xFFC00000,0xE0600000}},       \
    {0x10612024, {0x3FE00000,0x00000000, 0x7F800000,0xC4004000}},       \
    {0x10657024, {0x7FF00000,0x00000000, 0x7F800000,0xD6020000}},       \
    {0x10611825, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_madd not aborted by enabled exception. */                     \
    {0x106B107A, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10600A7A, {0xFFF80000,0x00000000, 0xFFC00000,0xE0100000}},       \
    {0x106968BA, {0xFFF80000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x1065237A, {0xFFFC0000,0x00000000, 0x7F800000,0xF3020000}},       \
    {0x106D217A, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x106110FB, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_madds0 not aborted by enabled exception. */                   \
    {0x10650B5C, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x10650B5D, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    /* ps_madds1 not aborted by enabled exception. */                   \
    {0x10650B5E, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x10650B5F, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    /* ps_msub not aborted by enabled exception. */                     \
    {0x106B1078, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10600A78, {0xFFF80000,0x00000000, 0xFFC00000,0xE0100000}},       \
    {0x106948B8, {0xFFF80000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x10652378, {0xFFFC0000,0x00000000, 0x7F800000,0xF3020000}},       \
    {0x106D2178, {0x7FF00000,0x00000000, 0xFFE00000,0xF3025000}},       \
    {0x106110F9, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_nmadd not aborted by enabled exception. */                    \
    {0x106B107E, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10600A7E, {0xFFF80000,0x00000000, 0xFFC00000,0xE0100000}},       \
    {0x106968BE, {0xFFF80000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x1065237E, {0xFFFC0000,0x00000000, 0xFF800000,0xF3020000}},       \
    {0x106D217E, {0xFFF00000,0x00000000, 0xFFE00000,0xF3029000}},       \
    {0x106110FF, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_nmsub not aborted by enabled exception. */                    \
    {0x106B107C, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10600A7C, {0xFFF80000,0x00000000, 0xFFC00000,0xE0100000}},       \
    {0x106948BC, {0xFFF80000,0x00000000, 0xFFC00000,0xE0800000}},       \
    {0x1065237C, {0xFFFC0000,0x00000000, 0xFF800000,0xF3020000}},       \
    {0x106D217C, {0xFFF00000,0x00000000, 0xFFE00000,0xF3029000}},       \
    {0x106110FD, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_res not aborted by enabled exception. */                      \
    {0x10602830, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x10602030, {0x7FF00000,0x00000000, 0x7F800000,0xC4000000}},       \
    {0x11C02030, {0xFFFC0000,0x00000000, 0x7F7FFFFF,0xF1020000}},       \
    {0x11C02830, {0x47EFFFFF,0xE0000000, 0xFFE00000,0xF1024000}},       \
    {0x11C06830, {0x3FDFFF00,0x00000000, 0x7F800000,0xC4004000}},       \
    {0x10601831, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    /* ps_rsqrte not aborted by enabled exception. */                   \
    {0x10602834, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}},       \
    {0x1060D834, {0x7FF80000,0x00000000, 0x7FC00000,0xE0000200}},       \
    {0x10602034, {0xFFF00000,0x00000000, 0xFF800000,0xC4000000}},       \
    {0x11C02034, {0xFFFC0000,0x00000000, 0x3F34FD00,0xE1000000}},       \
    {0x11C02834, {0x3FE69FA0,0x00000000, 0xFFE00000,0xE1004000}},       \
    {0x11C06834, {0x3FE69FA0,0x00000000, 0x7F800000,0xC4004000}},       \
    /* lfd/ps_rsqrte misbehavior. */                                    \
    {0x11A01834, {0x00000000,0x00000000, 0x1F800041,0x00000000}},       \
    {0x11A02034, {0x7FF00000,0x00000000, 0x00000000,0x00000000}},       \
    /* ps_rsqrte not aborted by enabled exception, continued. */        \
    {0x10601835, {0xFFFC0000,0x00000000, 0xFFE00000,0xE1000000}}

/* Expected errors for the PS_STORE_DENORMALS optimization. */
#define EXPECTED_ERRORS_PS_STORE_DENORMALS  \
    /* psq_st of denormal values, not zeroed due to the optimization. */ \
    {0xF0640000, {0x00000003,0x00000004, 0x00000000,0x00000000}}

/*************************************************************************/
/************************** Callback functions ***************************/
/*************************************************************************/

/* Handler for system call exceptions. */
static void sc_handler(PPCState *state)
{
    ASSERT(state);
    state->gpr[3] = 1;
}

/*-----------------------------------------------------------------------*/

/* Handler for timebase register reads.  Returns a value which increments
 * on each TBL or TBU read. */
static uint64_t timebase_handler(PPCState *state)
{
    ASSERT(state);
    return state->tb++;
}

/*-----------------------------------------------------------------------*/

/* Handler for trap exceptions. */
static void trap_handler(PPCState *state)
{
    ASSERT(state);
    state->gpr[3] = 0;
    state->nia += 4;  // Continue with the next instruction.
}

/*************************************************************************/
/************************* Test helper functions *************************/
/*************************************************************************/

/**
 * setup_750cl:  Set up a guest memory region and processor state block
 * for running the PowerPC 750CL test routine.
 *
 * [Parameters]
 *     state: Processor state block to initialize.
 * [Return value]
 *     Newly allocated guest memory region (free with free() when no
 *     longer needed).
 */
static void *setup_750cl(PPCState *state)
{
    uint8_t *memory;
    memory = malloc(PPC750CL_MEMORY_SIZE);
    if (!memory) {
        return NULL;
    }
    memset(memory, 0, PPC750CL_MEMORY_SIZE);
    memcpy(memory + PPC750CL_START_ADDRESS, ppc750cl_bin,
           sizeof(ppc750cl_bin));

    memset(state, 0, sizeof(*state));
    state->timebase_handler = timebase_handler;
    state->sc_handler = sc_handler;
    state->trap_handler = trap_handler;
    state->fres_lut = ppc_fres_lut;
    state->frsqrte_lut = ppc_frsqrte_lut;
    state->gpr[4] = PPC750CL_SCRATCH_ADDRESS;
    state->gpr[5] = PPC750CL_ERROR_LOG_ADDRESS;
    state->fpr[1][0] = 1.0;

    return memory;
}

/*-----------------------------------------------------------------------*/

/**
 * check_750cl_errors:  Check the errors reported from the 750CL test
 * routine against the expected error list, and print any discrepancies
 * to stdout.
 *
 * [Parameters]
 *     count: Return value from the test routine.
 *     memory: Pointer to guest memory region.
 *     expected_errors: Array of expected errors.  Errors are matched by
 *         the first two words of the record; if the same test is listed
 *         multiple times, the first occurrence is used.
 *     expected_length: Length of the expected_errors array.
 * [Return value]
 *     True if only the expected errors were detected, false otherwise.
 */
static bool check_750cl_errors(
    int count, void *memory, const FailureRecord *expected_errors,
    int expected_length)
{
    if (count < 0) {
        printf("Test failed to bootstrap (error %d)\n", count);
        return false;
    }

    bool success = true;
    const uint32_t *error_log =
        (const uint32_t *)((uintptr_t)memory + PPC750CL_ERROR_LOG_ADDRESS);
    uint8_t *error_seen = malloc(expected_length);
    ASSERT(error_seen || !expected_length);
    memset(error_seen, 0, expected_length);
    bool printed_header = false;

    for (int i = 0; i < count; i++, error_log += 8) {
        const uint32_t insn = bswap_be32(error_log[0]);
        const uint32_t address = bswap_be32(error_log[1]);
        bool pass = false;

        int found_index = -1;
        for (int j = 0; j < expected_length; j++) {
            if (expected_errors[j].insn == insn) {
                error_seen[j] = 1;
                if (found_index < 0) {
                    found_index = j;
                    pass = true;
                    for (int k = 0; k < 6; k++) {
                        const uint32_t data = bswap_be32(error_log[2+k]);
                        if (data != expected_errors[j].data[k]) {
                            pass = false;
                            break;
                        }
                    }
                }
            }
        }

        if (!pass) {
            success = false;
            if (!printed_header) {
                printf("Unexpected errors detected:\n");
                printed_header = true;
            }
            printf("    %08X %08X  %08X %08X  %08X %08X", insn, address,
                   bswap_be32(error_log[2]), bswap_be32(error_log[3]),
                   bswap_be32(error_log[4]), bswap_be32(error_log[5]));
            if ((insn & 0xFC00003E) == 0xFC000034) {  // frsqrte
                printf("  %08X %08X",
                       bswap_be32(error_log[6]), bswap_be32(error_log[7]));
            }
            printf("\n");
            if (found_index >= 0) {
                printf("        `--> Expected: %08X %08X  %08X %08X",
                       expected_errors[found_index].data[0],
                       expected_errors[found_index].data[1],
                       expected_errors[found_index].data[2],
                       expected_errors[found_index].data[3]);
                if ((insn & 0xFC00003E) == 0xFC000034) {  // frsqrte
                    printf("  %08X %08X",
                           expected_errors[found_index].data[4],
                           expected_errors[found_index].data[5]);
                }
                printf("\n");
            }
        }
    }

    printed_header = false;
    for (int i = 0; i < expected_length; i++) {
        if (!error_seen[i]) {
            success = false;
            if (!printed_header) {
                printf("Expected errors not detected:\n");
                printed_header = true;
            }
            printf("    %08X --------  %08X %08X  %08X %08X",
                   expected_errors[i].insn,
                   expected_errors[i].data[0], expected_errors[i].data[1],
                   expected_errors[i].data[2], expected_errors[i].data[3]);
            if ((expected_errors[i].insn & 0xFC00003E) == 0xFC000034) {
                printf("  %08X %08X",
                       expected_errors[i].data[4], expected_errors[i].data[5]);
            }
            printf("\n");
            for (int j = i+1; j < expected_length; j++) {
                if (expected_errors[j].insn == expected_errors[i].insn) {
                    error_seen[j] = 1;
                }
            }
        }
    }

    free(error_seen);
    return success;
}

/*************************************************************************/
/*************************************************************************/
