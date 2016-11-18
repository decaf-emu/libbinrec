# PowerPC 750CL test code.
# Written by Andrew Church <achurch@achurch.org>
# No copyright is claimed on this file.
#
# Update history:
#    - 2016-11-18: Added more tests for various instructions.
#    - 2016-11-16: Added reciprocal table tests for ps_res and ps_rsqrte.
#    - 2016-11-16: Added tests for excess range on ps_merge* and ps_rsqrte.
#    - 2016-11-16: Added more tests for paired-single exception handling.
#    - 2016-11-15: Fixed a psq_l test skipping the next test on failure.
#    - 2016-11-14: Added a TEST_UNDOCUMENTED symbol (enabled by default)
#         protecting tests of instruction formats which are not documented
#         or are documented to have undefined behavior.
#    - 2016-11-13: Rewrote fres and frsqrte tests to only check that the
#         instruction output is within the specification-mandated bounds,
#         and added separate tests (enabled by TEST_RECIPROCAL_TABLES) to
#         check for exact results.  TEST_RECIPROCAL_TABLES is disabled by
#         default.
#    - 2016-11-13: Added tests for underflow exception behavior with
#         two-operand floating-point instructions.
#    - 2016-11-13: Added tests for NaN selection on floating-point
#         instructions with multiple NaN operands.
#    - 2016-11-12: Added a few more fmadd tests to verify that VXIMZ and
#         VXISI exceptions are properly distinguished.
#    - 2016-11-12: Added tests to verify that certain unused FPR fields in
#         floating-point instructions are ignored.  See notes at "fadd
#         (nonzero frC)" for details.
#    - 2016-11-11: Added more tests for frC rounding behavior with
#         single-precision multiply operations.
#    - 2016-11-11: Added tests to verify that double-precision multiply
#         operations do not round the frC mantissa to 24 bits.
#    - 2016-11-11: Fixed an fmadd test which could fail to detect
#         FPSCR[XX,FI] not being set on an inexact operation.
#    - 2016-11-11: Changed register usage in a few tests to avoid having
#         multiple tests with the same instruction word.
#    - 2016-11-03: Fixed an incorrect comment.
#    - 2016-10-14: Fixed cases in which mtcrf and mcrxr tests could leave
#         blank failure records.
#    - 2016-10-14: Made the icbi test a little more semantically sensible.
#    - 2016-10-12: Added missing tests for ps_merge instructions with Rc=1.
#    - 2016-10-12: Fixed incorrect encoding of mftb in mftbu tests.  (The
#         error has no impact on correctness, only on the usefulness of
#         the failure record contents).
#    - 2016-10-02: Added tests to verify that lmw/stmw with a very positive
#         offset do not wrap around to negative offsets.  Also changed the
#         lmw/stmw tests to record the incorrect value detected and the
#         associated register index in words 2 and 3 of the failure record.
#    - 2016-09-30: Added a test to verify that lwarx followed by stwcx. on
#         a different address still succeeds.
#    - 2016-09-18: Fixed a typo in the documentation.
#    - 2016-08-16: Added a test for blrl to verify that LR is read and
#         written in the correct order.
#    - 2016-07-19: Added tests for lfs (with respect to paired-single
#         slot 1) and ps_{mr,neg,abs,nabs} with denormal inputs.
#    - 2016-07-19: Fixed overwrite of the result value in the ps_merge01
#         SNaN test.
#    - 2016-07-19: Fixed duplicate instruction "fmsubs %f3,%f4,%f4,%f3"
#         (should have been fnmsubs).
#    - 2016-01-29: Fixed incorrect encoding of "bc 31,2,target".
#         Thanks to Matt Mastracci for spotting the error.
#    - 2015-12-15: Added tests for various floating-point edge cases.
#    - 2015-12-14: Added tests verifying behavior of single-precision
#         floating-point instructions on double-precision operands.
#    - 2015-12-10: Added tests for paired-single instructions and
#         interactions between paired-single and double-precision formats.
#    - 2015-12-09: Added tests verifying that single-precision-output
#         floating-point instructions perform their computations in
#         double rather than single precision.
#    - 2015-12-09: Clarified that the PowerPC 750CL implements version
#         1.10 of the PowerPC UISA, and changed documentation references
#         to refer to version 2.01 (the closest version I have available).
#    - 2015-12-08: Initial release, based on the Cell PPU tests at
#         <http://achurch.org/cpu-tests/cell-ppu.s>.
#
# This file implements a test routine which exercises all instructions
# implemented by the PowerPC 750CL CPU and verifies that they return
# correct results.  The routine (excluding the tests of the absolute
# branch, trap, and system call instructions) has been validated against
# a real Espresso (750CL compatible) processor.
#
# Call with four arguments:
#    R3: 0 (to bootstrap a constant value into the register file)
#    R4: pointer to a 32k (32768-byte) pre-cleared, cache-aligned scratch
#           memory block (must not be address zero)
#    R5: pointer to a memory block into which failing instructions will be
#           written (should be at least 64k to avoid overruns)
#    F1: 1.0 (for register file bootstrapping)
# This follows the PowerPC ABI and can be called from C code with a
# prototype like:
#     extern int test(int zero, void *scratch, void *failures, double one);
# (Note that this file does not itself define a symbol for the beginning
# of the test routine.  You can edit this file to insert such a symbol, or
# include this file in another file immediately after a symbol definition.)
#
# Returns (in R3) the number of failing instructions, or a negative value
# if the test failed to bootstrap itself.
#
# On return, if R3 > 0 then the buffer in R5 contains R3 failure records.
# Each failure record is 8 words (32 bytes) long and has the following
# format:
#    Word 0: Failing instruction word
#    Word 1: Address of failing instruction word
#    Words 2-5: Auxiliary data (see below)
#    Words 6-7: Unused (except for frsqrte reciprocal table tests)
# Instructions have been coded so that generally, the operands uniquely
# identify the failing instruction, but in some cases (such as rounding
# mode tests with frsp) there are multiple copies of the same instruction
# in different locations; in such cases, use the address to locate the
# failing instruction.
#
# The "auxiliary data" stored in a failure record varies by instruction
# (see the code around the failure point for details), but it is usually
# the value produced by the failing instruction, with integer values stored
# to word 2 (leaving word 3 unset) and floating-point values stored to
# words 2-3 of the failure record.  In particular, instructions checked by
# the check_alu_* and check_fpu_* subroutines store the incorrect result as
# an integer word in word 2 (or double-precision floating point value in
# words 2-3), the value of CR (or FPSCR) in word 4, and (for check_alu_*
# only) the value of XER in word 5; the check_ps_* subroutines store the
# first slot (ps0) of the result as a double-precision value in words 2-3,
# the second slot (ps1) as a single-precision value in word 4, and the
# value of FPSCR in word 5.
#
# The code assumes that the following instructions work correctly:
#    - beq cr0,target
#    - bne cr0,target
#    - bl
#    - blr (bclr 20,0,0)
#    - fcmpu (to the extent that two FP registers can be tested for equality)
#    - mflr
#    - mtlr
#
# The following instructions and features are not tested:
#    - The bca, bcla, eciwx, and ecowx instructions
#    - D-form and DS-form loads and stores to absolute addresses (RA=0)
#    - Floating-point operations with FPSCR[OE] or FPSCR[UE] set
#    - The effect of floating-point operations on FPSCR[FR]
#

# MFTB_SPIN_COUNT:  Set this to the number of times to loop while waiting
# for the time base register to increment.
.ifndef MFTB_SPIN_COUNT
MFTB_SPIN_COUNT = 256
.endif

# TEST_BA:  Set this to 1 to test the ba and bla (branch absolute)
# instructions.  For these tests to work, the code must be loaded at
# absolute address 0x1000000.  (If this option is enabled and the code
# is loaded at the wrong address, the bootstrap will fail with return
# value -256 in R3 and the detected load address in R4.)
.ifndef TEST_BA
TEST_BA = 1
.endif

# TEST_SC:  Set this to 1 to test the sc (system call) instruction.  The
# implementation should set R3 to the value 1 and resume execution at the
# next instruction.
.ifndef TEST_SC
TEST_SC = 1
.endif

# TEST_TRAP:  Set this to 1 to test the tdi, twi, td, and tw (trap)
# instructions.  The implementation should set R3 to the value zero and
# resume execution at the next instruction.
.ifndef TEST_TRAP
TEST_TRAP = 1
.endif

# TEST_RECIPROCAL_TABLES:  Set this to 1 to test for the precise outputs
# of the fres and frsqrte instructions.
.ifndef TEST_RECIPROCAL_TABLES
TEST_RECIPROCAL_TABLES = 0
.endif

# TEST_PAIRED_SINGLE:  Set this to 1 to test the paired-single floating
# point instructions and interactions between double-precision and
# paired-single formats.  This requires HID2[PSE] to be set to 1.
.ifndef TEST_PAIRED_SINGLE
TEST_PAIRED_SINGLE = 1
.endif

# TEST_UNDOCUMENTED:  Set this to 1 to test the behavior of undocumented
# or documented-as-undefined instruction formats.
.ifndef TEST_UNDOCUMENTED
TEST_UNDOCUMENTED = 1
.endif

# CAN_SET_GQR:  Set this to 1 if the routine will be run in an environment
# which allows writing to the GQR registers.  If this is not set, the
# routine will test only the floating point load and store operations, and
# will assume that all fields of GQR0 are set to zero.
.ifndef CAN_SET_GQR
CAN_SET_GQR = 1
.endif

# ESPRESSO:  Set this to 1 if testing the Espresso CPU.  This enables use
# of the Espresso-specific UGQR registers to set GQRs without supervisor
# privileges.
.ifndef ESPRESSO
ESPRESSO = 0
.endif


# Convenience macro for loading a double-precision value into the first
# paired-single slot of an FPR register, properly delaying the instruction
# stream to avoid data hazards on the second slot.  The double-precision
# value stored at source is loaded to frD(ps0), and frB(ps1) is copied to
# frD(ps1).
.macro lfd_ps frD, source, frB
   ps_mr \frD,\frB
   isync
   lfd \frD,\source
   isync
.endm


   .machine "750cl"

   ########################################################################
   # Jump over a couple of tests which need to be located at a known
   # address when testing absolute branch instructions.  As we do so,
   # check that cmpwi against zero works.
   ########################################################################

0: cmpwi %r3,0          # 0x1000000
   beq 0f               # 0x1000004
   li %r3,-1            # 0x1000008
   blr                  # 0x100000C

.if TEST_BA

   ########################################################################
   # Subroutine called from the bootstrap code to verify the load address.
   # Returns the load address of the test code (label 0b above) in R3;
   # destroys R0.
   ########################################################################

get_load_address:
   mflr %r0             # 0x1000010
   bl 1f                # 0x1000014
1: mflr %r3             # 0x1000018
   mtlr %r0             # 0x100001C
   addi %r3,%r3,0b-1b   # 0x1000020
   blr                  # 0x1000024

   ########################################################################
   # 2.4.1 Branch Instructions - ba, bla (second half of test)
   ########################################################################

   mflr %r3             # 0x1000028
   ba 0x1000038         # 0x100002C
   # These two instructions should be skipped.
   bl record            # 0x1000030
   addi %r6,%r6,32      # 0x1000034
   # Execution continues here.
   addi %r3,%r3,8       # 0x1000038
   mtlr %r3             # 0x100003C
   blr                  # 0x1000040

.endif  # TEST_BA

   ########################################################################
   # Bootstrap a few instructions so we have a little more flexibility.
   ########################################################################

   # mr RT,RA (or RT,RA,RA)
0: mr %r0,%r3       # Zero value.
   cmpwi %r0,0
   beq 0f
   li %r3,-2
   blr
0: mr %r0,%r4       # Nonzero value.
   cmpwi %r0,0
   bne 0f
   li %r3,-3
   blr

   # li RT,0 (addi RT,0,0)
0: li %r0,0
   cmpwi %r0,0
   beq 0f
   li %r3,-4
   blr

   # li RT,imm (addi RT,0,imm), positive value
0: li %r0,1
   cmpwi %r0,0
   bne 0f
   li %r3,-5
   blr

   # lis RT,imm (addis RT,0,imm), positive value
0: li %r0,0
   lis %r0,1
   cmpwi %r0,0
   bne 0f
   li %r3,-6
   blr

   # b target (forward displacement)
0: b 0f
   li %r3,-7
   blr

   # cmpw RA,RB (cmp 0,0,RA,RB)
0: cmpw %r0,%r0
   beq 0f
   li %r3,-8
   blr
0: cmpw %r0,%r3
   bne 0f
   li %r3,-9
   blr

   # cmpwi RA,imm (cmpi 0,0,RA,imm) with imm != 0
0: li %r0,1
   cmpwi %r0,1
   beq 0f
   li %r3,-10
   blr

   # addi RT,RT,imm (RT != 0, imm > 0)
0: mr %r3,%r0
   addi %r3,%r3,2
   cmpwi %r3,3
   beq 0f
   li %r3,-11
   blr

   # addi RT,RT,imm (RT != 0, imm < 0)
0: addi %r3,%r3,-1
   cmpwi %r3,2
   beq 0f
   li %r3,-12
   blr

   # addi RT,RA,imm (RT != RA)
0: addi %r7,%r3,2
   cmpwi %r7,4
   beq 0f
   li %r3,-13
   blr

   # addis RT,RA,imm
0: lis %r7,1
   addis %r3,%r7,2
   lis %r7,3
   cmpw %r3,%r7
   beq 0f
   li %r3,-14
   blr

   # lwz RT,0(RA)
0: lwz %r0,0(%r4)
   cmpwi %r0,0
   beq 0f
   li %r3,-15
   blr

   # stw RS,0(RA)
0: li %r0,1
   stw %r0,0(%r4)
   li %r0,0
   lwz %r0,0(%r4)
   cmpwi %r0,1
   beq 0f
   li %r3,-16
   blr

   # lwz RT,imm(RA) (imm > 0)
   # stw RS,imm(RA) (imm > 0)
0: li %r0,2
   stw %r0,8(%r4)
   li %r0,0
   lwz %r0,8(%r4)
   cmpwi %r0,2
   beq 0f
   li %r3,-17
   blr

   # lwz RT,imm(RA) (imm < 0)
   # stw RS,imm(RA) (imm < 0)
0: addi %r3,%r4,16
   li %r0,3
   stw %r0,-12(%r3)
   li %r0,0
   lwz %r0,-12(%r3)
   cmpwi %r0,3
   beq 0f
   li %r3,-18
   blr
0: li %r0,0
   lwz %r0,4(%r4)
   cmpwi %r0,3
   beq 0f
   li %r3,-19
   blr

   ########################################################################
   # If we'll be testing the absolute branch instructions, ensure that the
   # test code was loaded at the correct address.
   ########################################################################

.if TEST_BA
0: mflr %r7
   bl get_load_address
   mtlr %r7
   lis %r7,0x100
   cmpw %r3,%r7
   beq 0f
   mr %r4,%r3
   li %r3,-256
   blr
.endif

   ########################################################################
   # Save nonvolatile registers used in the test.
   ########################################################################

0: mfcr %r0
   stw %r0,0x7F00(%r4)
   mflr %r0
   stw %r0,0x7F04(%r4)
   stw %r14,0x7F08(%r4)
   stw %r15,0x7F0C(%r4)
   stw %r24,0x7F10(%r4)
   stw %r25,0x7F14(%r4)
   stw %r26,0x7F18(%r4)
   stw %r27,0x7F1C(%r4)
   stw %r28,0x7F20(%r4)
   stw %r29,0x7F24(%r4)
   stw %r30,0x7F28(%r4)
   stw %r31,0x7F2C(%r4)
   stfd %f14,0x7F30(%r4)
   stfd %f15,0x7F38(%r4)
   stfd %f24,0x7F40(%r4)
   stfd %f25,0x7F48(%r4)
   stfd %f26,0x7F50(%r4)
   stfd %f27,0x7F58(%r4)
   stfd %f28,0x7F60(%r4)
   stfd %f29,0x7F68(%r4)
   stfd %f30,0x7F70(%r4)
   stfd %f31,0x7F78(%r4)

   ########################################################################
   # Set up some registers used in the test proper:
   #     R6 = pointer to next slot in failed-instruction buffer
   ########################################################################

   mr %r6,%r5

   ########################################################################
   # Beginning of the test proper.  The test is divided into sections,
   # each with a header comment (like this one) which indicates the group
   # of instructions being tested.  Where applicable, the header also
   # indicates the reference material used to write the tests:
   #
   # - "X.Y.Z Section Title" (where X, Y, Z are numbers): the given section
   #   in "PowerPC User Instruction Set Architecture, Book I, Version 2.01"
   #
   # - "Book II X.Y.Z Section Title": the given section in "PowerPC Virtual
   #   Environment Architecture, Book II, Version 2.01"
   #
   # - "PowerPC 750CL Manual": the instruction set reference in "IBM
   #   PowerPC 750CL RISC Microprocessor User's Manual, Version 1.0"
   #
   # (Note: The 750CL implements version 1.10 of the PowerPC UISA, but I
   # haven't been able to find the v1.10 UISA documents, so the comments
   # here refer to version 2.01, which seems a close analogue if one
   # ignores the 64-bit integer instructions.)
   ########################################################################

   ########################################################################
   # 2.4.1 Branch Instructions - b, bl
   ########################################################################

   # b (forward displacement) was tested in the bootstrap code.

   # bl (forward displacement)
0: bl 3f
1: bl record
   lwz %r3,(2f-1b)(%r3)
   lis %r0,1
   cmpw %r0,%r3
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
   b 0f
2: .int 0x00010000
3: mflr %r3
   blr

   # b (backward displacement)
0: b 1f
2: b 0f
1: b 2b
   bl record
   addi %r6,%r6,32

   # bl (backward displacement)
0: b 2f
1: mflr %r3
   blr
2: bl 1b
3: bl record
   lwz %r3,(4f-3b)(%r3)
   lis %r0,2
   cmpw %r0,%r3
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
   b 0f
4: .int 0x00020000

   ########################################################################
   # 2.4.1 Branch Instructions - ba, bla (first half of test)
   ########################################################################

.if TEST_BA
0: bla 0x1000028
   bl record        # Skipped.
   addi %r6,%r6,32  # Skipped.
.endif

   ########################################################################
   # 2.4.1 Branch Instructions - bc (decrement CTR and branch if zero)
   # 3.3.13 Move To/From System Register Instructions - mtctr, mfctr
   ########################################################################

   # Single iteration (CTR=1 -> 0)
   li %r3,1
   mtctr %r3
   bdz 0f
   bl record
   addi %r6,%r6,32

   # Two iterations (CTR=2 -> 1 -> 0)
0: li %r3,2
   mtctr %r3
   bl record
   bdz 1f           # Not taken.
   mfctr %r3
   bl record
   cmpwi %r3,1
   beq 2f
   stw %r3,8(%r6)
   addi %r6,%r6,32
2: bdz 0f           # Taken.
   bl record
1: addi %r6,%r6,32

   ########################################################################
   # 2.4.1 Branch Instructions - bc
   ########################################################################

   # bc 1z1zz,BI,target (branch always)
   # We set up CR0 here for the "eq" condition and leave it untouched for
   # all of these tests.
0: li %r3,0
   cmpwi %r3,0
   # Test with a false CR bit.
   bc 20,0,0f
   bl record
   addi %r6,%r6,32
.if TEST_UNDOCUMENTED
   # Check that the "z" bits in 1z1zz are ignored.
   # Some assemblers reject instructions with z != 0, so code them directly.
0: .int 0x42A0000C  # bc 21,0,0f
   bl record
   addi %r6,%r6,32
0: .int 0x42C0000C  # bc 22,0,0f
   bl record
   addi %r6,%r6,32
0: .int 0x42E0000C  # bc 23,0,0f
   bl record
   addi %r6,%r6,32
0: .int 0x4380000C  # bc 28,0,0f
   bl record
   addi %r6,%r6,32
0: .int 0x43A0000C  # bc 29,0,0f
   bl record
   addi %r6,%r6,32
0: .int 0x43C0000C  # bc 30,0,0f
   bl record
   addi %r6,%r6,32
0: .int 0x43E0000C  # bc 31,0,0f
   bl record
   addi %r6,%r6,32
.endif
   # Test with a true CR bit.
0: bc 20,2,0f
   bl record
   addi %r6,%r6,32
.if TEST_UNDOCUMENTED
0: .int 0x42A2000C  # bc 21,2,0f
   bl record
   addi %r6,%r6,32
0: .int 0x42C2000C  # bc 22,2,0f
   bl record
   addi %r6,%r6,32
0: .int 0x42E2000C  # bc 23,2,0f
   bl record
   addi %r6,%r6,32
0: .int 0x4382000C  # bc 28,2,0f
   bl record
   addi %r6,%r6,32
0: .int 0x43A2000C  # bc 29,2,0f
   bl record
   addi %r6,%r6,32
0: .int 0x43C2000C  # bc 30,2,0f
   bl record
   addi %r6,%r6,32
0: .int 0x43E2000C  # bc 31,2,0f
   bl record
   addi %r6,%r6,32
.endif

   # bc 1a01t,BI,target (decrement and branch if CTR==0)
0: li %r3,2
   mtctr %r3
   bdz 1f           # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdz 0f           # Taken.
   bl record
   addi %r6,%r6,32
0: li %r3,2
   mtctr %r3
   bdz- 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdz- 0f          # Taken.
   bl record
   addi %r6,%r6,32
0: li %r3,2
   mtctr %r3
   bdz+ 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdz+ 0f          # Taken.
   bl record
   addi %r6,%r6,32

   # bc 1a00t,BI,target (decrement and branch if CTR!=0)
0: li %r3,1
   mtctr %r3
   bdnz 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnz 0f          # Taken.
   bl record
   addi %r6,%r6,32
0: li %r3,1
   mtctr %r3
   bdnz- 1f         # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnz- 0f         # Taken.
   bl record
   addi %r6,%r6,32
0: li %r3,1
   mtctr %r3
   bdnz+ 1f         # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnz+ 0f         # Taken.
   bl record
   addi %r6,%r6,32

   # bc 011at,BI,target (branch if condition bit true)
0: bgt 1f           # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: beq 0f           # Taken.
   bl record
   addi %r6,%r6,32
0: bgt- 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: beq- 0f          # Taken.
   bl record
   addi %r6,%r6,32
0: bgt+ 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: beq+ 0f          # Taken.
   bl record
   addi %r6,%r6,32

   # bc 0101z,BI,target (decrement and branch if condition true and CTR==0)
0: li %r3,2
   mtctr %r3
   bdzt gt,1f       # Not taken (condition false and CTR!=0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdzt gt,1f       # Not taken (condition false).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,2
   mtctr %r3
   bdzt eq,1f       # Not taken (CTR!=0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdzt eq,0f       # Taken.
   bl record
   addi %r6,%r6,32
.if TEST_UNDOCUMENTED
   # Same with z=1.
0: li %r3,2
   mtctr %r3
   .int 0x41610008  # bdzt gt,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x41610008  # bdzt gt,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,2
   mtctr %r3
   .int 0x41620008  # bdzt eq,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x4162000C  # bdzt eq,0f
   bl record
   addi %r6,%r6,32
.endif

   # bc 0100z,BI,target (decrement and branch if condition true and CTR!=0)
0: li %r3,1
   mtctr %r3
   bdnzt gt,1f      # Not taken (condition false and CTR==0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnzt gt,1f      # Not taken (condition false).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,1
   mtctr %r3
   bdnzt eq,1f      # Not taken (CTR==0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnzt eq,0f      # Taken.
   bl record
   addi %r6,%r6,32
.if TEST_UNDOCUMENTED
   # Same with z=1.
0: li %r3,1
   mtctr %r3
   .int 0x41210008  # bdzt gt,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x41210008  # bdzt gt,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,1
   mtctr %r3
   .int 0x41220008  # bdzt eq,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x4122000C  # bdzt eq,0f
   bl record
   addi %r6,%r6,32
.endif

   # bc 001at,BI,target (branch if condition bit false)
0: bne 1f           # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: ble 0f           # Taken.
   bl record
   addi %r6,%r6,32
0: bne- 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: ble- 0f          # Taken.
   bl record
   addi %r6,%r6,32
0: bne+ 1f          # Not taken.
   b 0f
1: bl record2
   addi %r6,%r6,32
0: ble+ 0f          # Taken.
   bl record
   addi %r6,%r6,32

   # bc 0001z,BI,target (decrement and branch if condition false and CTR==0)
0: li %r3,2
   mtctr %r3
   bdzf eq,1f       # Not taken (condition false and CTR!=0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdzf eq,1f       # Not taken (condition false).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,2
   mtctr %r3
   bdzf gt,1f       # Not taken (CTR!=0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdzf gt,0f       # Taken.
   bl record
   addi %r6,%r6,32
.if TEST_UNDOCUMENTED
   # Same with z=1.
0: li %r3,2
   mtctr %r3
   .int 0x40620008  # bdzf eq,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x40620008  # bdzf eq,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,2
   mtctr %r3
   .int 0x40610008  # bdzf gt,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x4061000C  # bdzf gt,0f
   bl record
   addi %r6,%r6,32
.endif

   # bc 0000z,BI,target (decrement and branch if condition false and CTR!=0)
0: li %r3,1
   mtctr %r3
   bdnzf eq,1f      # Not taken (condition false and CTR==0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnzf eq,1f      # Not taken (condition false).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,1
   mtctr %r3
   bdnzf gt,1f      # Not taken (CTR==0).
   b 0f
1: bl record2
   addi %r6,%r6,32
0: bdnzf gt,0f      # Taken.
   bl record
   addi %r6,%r6,32
.if TEST_UNDOCUMENTED
   # Same with z=1.
0: li %r3,1
   mtctr %r3
   .int 0x40220008  # bdzf eq,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x40220008  # bdzf eq,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: li %r3,1
   mtctr %r3
   .int 0x40210008  # bdzf gt,1f
   b 0f
1: bl record2
   addi %r6,%r6,32
0: .int 0x4021000C  # bdzf gt,0f
   bl record
   addi %r6,%r6,32
.endif

   # bc (backward displacement)
   # We assume the displacement and link flags are processed the same way
   # for all conditions, so we only test a single condition here and below.
0: b 1f
2: b 0f
1: bc 20,0,2b
   bl record
   addi %r6,%r6,32  # Skipped.

   # bcl
0: bcl 20,0,3f
1: bl record
   lwz %r3,(2f-1b)(%r3)
   lis %r0,3
   cmpw %r0,%r3
   beq 0f
   addi %r6,%r6,32
   b 0f
2: .int 0x00030000
3: mflr %r3
   blr

   # Testing bca/bcla requires a system in which the address range
   # -0x8000...0x7FFF is mapped.  Such systems are believed to be rare if
   # not nonexistent, so we skip those instructions.

   ########################################################################
   # 2.4.1 Branch Instructions - bclr, bcctr
   ########################################################################

   # We assume bclr and bcctr use the same condition decoding logic as bc,
   # so we only use one test for each different instruction pattern.

   # bclr (with decrement)
0: b 2f
1: li %r0,2
   mtctr %r0
   bdzlr
   bdzlr
   bdzlr
   blr
2: bl record2
   bl 1b
3: mfctr %r3
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # bclr (with condition)
0: b 2f
1: li %r0,2
   cmpwi %r0,2
   beqlr
   mflr %r0
   bl record2
   mtlr %r0
   addi %r6,%r6,32
   blr
2: bl 1b

   # blrl (unconditional) -- check that this jumps to the previous value of
   # LR, not the value written by this instruction.
0: bl 4f                # Get the address of the next instruction so we can
1: addi %r3,%r3,3f-1b   # compute the address of label 3f to store in LR.
   addi %r7,%r3,2f-1b   # Also get the expected return address.
   mtlr %r3
   blrl                 # Jumps to label 3f.
2: bl record            # Fallthrough means we jumped to the new LR.
   addi %r6,%r6,32
   b 0f
3: mflr %r3
   cmpw %r3,%r7
   beq 0f
   addi %r9,%r7,-4
   lwz %r8,0(%r9)
   stw %r8,0(%r6)
   stw %r9,4(%r6)
   stw %r3,8(%r6)
   li %r8,0
   stw %r8,12(%r6)
   stw %r8,16(%r6)
   stw %r8,20(%r6)
   b 0f
4: mflr %r3
   blr

   # bcctr (unconditional)
0: bl 2f
1: addi %r3,%r3,0f-1b
   mtctr %r3
   bctr                 # Jumps to label 0f.
   bl record
   addi %r6,%r6,32
   b 0f
2: mflr %r3
   blr

   # bcctr (with condition)
0: bl 2f
1: addi %r3,%r3,0f-1b
   mtctr %r3
   li %r0,2
   cmpwi %r0,2
   beqctr
   bl record
   addi %r6,%r6,32
   b 0f
2: mflr %r3
   blr

   ########################################################################
   # 2.4.2 System Call Instruction
   ########################################################################

.if TEST_SC

0: li %r3,-1
   sc
   bl record
   cmpwi %r3,1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

.endif  # TEST_SC


   ########################################################################
   # 3.3.13 Move To/From System Register Instructions - mtcrf, mfcr
   ########################################################################

   # mtcr (mtcrf 255,RS)
0: li %r3,0
   cmpwi %r3,0
   mtcr %r3
   bl record
   bne 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mfcr
0: li %r3,0
   cmpwi %r3,0
   mfcr %r3
   bl record
   lis %r0,0x2000
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtcrf (single field)
0: lis %r3,0x4000
   mtcrf 128,%r3
   bl record
   bgt 0f
   addi %r6,%r6,32
0: lis %r3,0x0200
   mtcrf 64,%r3
   bl record
   ble 1f
   beq cr1,0f
1: addi %r6,%r6,32

   ########################################################################
   # 2.4.3 Condition Register Logical Instructions
   ########################################################################

   # crand
0: li %r3,0x0123
   mtcr %r3
   crand 17,20,24   # 0 & 0
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crand 17,20,26   # 0 & 1
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crand 17,23,24   # 1 & 0
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crand 17,23,26   # 1 & 1
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crand 31,31,31   # Check for inputs getting clobbered by output.
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # cror
0: li %r3,0x0123
   mtcr %r3
   cror 17,20,24   # 0 | 0
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   cror 17,20,26   # 0 | 1
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   cror 17,23,24   # 1 | 0
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   cror 17,23,26   # 1 | 1
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   cror 31,31,31
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # crxor
0: li %r3,0x0123
   mtcr %r3
   crxor 17,20,24   # 0 ^ 0
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crxor 17,20,26   # 0 ^ 1
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crxor 17,23,24   # 1 ^ 0
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crxor 17,23,26   # 1 ^ 1
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crxor 31,31,17
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crxor 31,17,31
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # crnand
0: li %r3,0x0123
   mtcr %r3
   crnand 17,20,24   # ~(0 & 0)
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnand 17,20,26   # ~(0 & 1)
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnand 17,23,24   # ~(1 & 0)
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnand 17,23,26   # ~(1 & 1)
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnand 17,17,17
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # crnor
0: li %r3,0x0123
   mtcr %r3
   crnor 17,20,24   # ~(0 | 0)
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnor 17,20,26   # ~(0 | 1)
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnor 17,23,24   # ~(1 | 0)
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnor 17,23,26   # ~(1 | 1)
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnor 17,17,17
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # creqv
0: li %r3,0x0123
   mtcr %r3
   creqv 17,20,24   # ~(0 ^ 0)
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   creqv 17,20,26   # ~(0 ^ 1)
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   creqv 17,23,24   # ~(1 ^ 0)
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   creqv 17,23,26   # ~(1 ^ 1)
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   creqv 17,17,17
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # crandc
0: li %r3,0x0123
   mtcr %r3
   crandc 17,20,24   # 0 & ~0
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crandc 17,20,26   # 0 & ~1
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crandc 17,23,24   # 1 & ~0
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crandc 17,23,26   # 1 & ~1
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnand 31,31,17
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crnand 17,31,17
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # crorc
0: li %r3,0x0123
   mtcr %r3
   crorc 17,20,24   # 0 | ~0
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crorc 17,20,26   # 0 | ~1
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crorc 17,23,24   # 1 | ~0
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crorc 17,23,26   # 1 | ~1
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0x0123
   mtcr %r3
   crorc 17,17,17
   bl record
   mfcr %r3
   cmpwi %r3,0x4123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 2.4.4 Condition Register Field Instruction
   ########################################################################

   # mcrf
0: li %r3,0x0123
   mtcr %r3
   mcrf 4,7
   bl record
   mfcr %r3
   cmpwi %r3,0x3123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
   # Check that the CR field is not cleared for overwrite before it is read.
0: li %r3,0x0123
   mtcr %r3
   mcrf 5,5
   bl record
   mfcr %r3
   cmpwi %r3,0x0123
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.2 Fixed-Point Load Instructions - lbz, lhz, lha, lwz (and -x, -u, -ux forms)
   ########################################################################

0: lis %r3,0xCE02
   addi %r3,%r3,0x468A
   stw %r3,0(%r4)
   lis %r3,0xDF13
   addi %r3,%r3,0x579B
   stw %r3,4(%r4)

   # lbz
0: addi %r7,%r4,2
   mr %r0,%r7
   lbz %r3,-2(%r7)
   bl record
   cmpwi %r3,0xCE
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: lbz %r3,-1(%r7)
   bl record
   cmpwi %r3,0x02
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: lbz %r3,0(%r7)
   bl record
   cmpwi %r3,0x46
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: lbz %r3,1(%r7)
   bl record
   cmpwi %r3,0x8A
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lbzx
0: addi %r7,%r4,2
   mr %r0,%r7
   li %r11,-2
   lbzx %r3,%r7,%r11
   bl record
   cmpwi %r3,0xCE
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Also check the case where RA=0 (should use 0 instead of the value of r0).
0: li %r3,0
   li %r0,1
   lbzx %r3,0,%r4
   bl record
   cmpwi %r3,0xCE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # lbzu
0: addi %r7,%r4,2
   lbzu %r3,-2(%r7)
   bl record
   cmpwi %r3,0xCE
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lbzux
0: addi %r7,%r4,2
   li %r11,-2
   lbzux %r3,%r7,%r11
   bl record
   cmpwi %r3,0xCE
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lhz
0: addi %r7,%r4,2
   mr %r0,%r7
   lhz %r3,-2(%r7)
   bl record
   li %r11,0x6E02
   addi %r11,%r11,0x6000
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: lhz %r3,0(%r7)
   bl record
   cmpwi %r3,0x468A
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lhzx
0: addi %r7,%r4,2
   mr %r0,%r7
   li %r11,-2
   lhzx %r3,%r7,%r11
   bl record
   li %r11,0x6E02
   addi %r11,%r11,0x6000
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0
   li %r0,2
   lhzx %r3,0,%r4
   bl record
   li %r11,0x6E02
   addi %r11,%r11,0x6000
   cmpw %r3,%r11
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # lhzu
0: addi %r7,%r4,2
   lhzu %r3,-2(%r7)
   bl record
   li %r11,0x6E02
   addi %r11,%r11,0x6000
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lhzux
0: addi %r7,%r4,2
   li %r11,-2
   lhzux %r3,%r7,%r11
   bl record
   li %r11,0x6E02
   addi %r11,%r11,0x6000
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lha
0: addi %r7,%r4,2
   mr %r0,%r7
   lha %r3,-2(%r7)
   bl record
   cmpwi %r3,0xCE02-0x10000
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: addi %r6,%r6,32
0: lha %r3,0(%r7)
   bl record
   cmpwi %r3,0x468A
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lhax
0: addi %r7,%r4,2
   mr %r0,%r7
   li %r11,-2
   lhax %r3,%r7,%r11
   bl record
   cmpwi %r3,0xCE02-0x10000
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0
   li %r0,2
   lhax %r3,0,%r4
   bl record
   cmpwi %r3,0xCE02-0x10000
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # lhau
0: addi %r7,%r4,2
   lhau %r3,-2(%r7)
   bl record
   cmpwi %r3,0xCE02-0x10000
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lhaux
0: addi %r7,%r4,2
   li %r11,-2
   lhaux %r3,%r7,%r11
   bl record
   cmpwi %r3,0xCE02-0x10000
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lwz was tested in the bootstrap code.

   # lwzx
0: addi %r7,%r4,4
   mr %r0,%r7
   li %r11,-4
   lwzx %r3,%r7,%r11
   bl record
   lis %r11,0x6E02
   addis %r11,%r11,0x6000
   addi %r11,%r11,0x468A
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0
   li %r0,4
   lwzx %r3,0,%r4
   bl record
   lis %r11,0x6E02
   addis %r11,%r11,0x6000
   addi %r11,%r11,0x468A
   cmpw %r3,%r11
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # lwzu
0: addi %r7,%r4,4
   lwzu %r3,-4(%r7)
   bl record
   lis %r11,0x6E02
   addis %r11,%r11,0x6000
   addi %r11,%r11,0x468A
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lwzux
0: addi %r7,%r4,4
   li %r11,-4
   lwzux %r3,%r7,%r11
   bl record
   lis %r11,0x6E02
   addis %r11,%r11,0x6000
   addi %r11,%r11,0x468A
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.11 Fixed-Point Logical Instructions - oris
   ########################################################################

   # oris
0: lis %r3,0x6000
   addis %r3,%r3,0x6000
   addi %r3,%r3,0x0006
   li %r7,6
   oris %r7,%r7,0xC000
   bl record
   cmpw %r7,%r3
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.3 Fixed-Point Store Instructions
   ########################################################################

   # stb
0: li %r3,0
   stw %r3,0(%r4)
   stw %r3,4(%r4)
   addi %r7,%r4,2
   mr %r0,%r7
   li %r3,0xCE
   stb %r3,-2(%r7)
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0xCE00-0x10000
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0x02
   stb %r3,-1(%r7)
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0xCE02-0x10000
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0x46
   stb %r3,0(%r7)
   bl record
   lha %r3,2(%r4)
   cmpwi %r3,0x4600
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0x8A
   stb %r3,1(%r7)
   bl record
   lha %r3,2(%r4)
   cmpwi %r3,0x468A
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stbx
0: addi %r7,%r4,2
   mr %r0,%r7
   li %r11,-2
   li %r3,0xDF
   stbx %r3,%r7,%r11
   bl record
   li %r3,0
   lbz %r3,0(%r4)
   cmpwi %r3,0xDF
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r0,1
   li %r3,0xDE
   stbx %r3,0,%r4
   bl record
   li %r3,0
   lbz %r3,0(%r4)
   cmpwi %r3,0xDE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # stbu
0: addi %r7,%r4,2
   li %r3,0xCE
   stbu %r3,-2(%r7)
   bl record
   li %r3,0
   lbz %r3,0(%r4)
   cmpwi %r3,0xCE
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stbux
0: addi %r7,%r4,2
   li %r11,-2
   li %r3,0xDF
   stbux %r3,%r7,%r11
   bl record
   li %r3,0
   lbz %r3,0(%r4)
   cmpwi %r3,0xDF
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # sth
0: li %r3,0
   stw %r3,0(%r4)
   stw %r3,4(%r4)
   addi %r7,%r4,2
   mr %r0,%r7
   li %r3,0x1234
   sth %r3,-2(%r7)
   bl record
   lwz %r3,0(%r4)
   lis %r11,0x1234
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r3,0x5678
   sth %r3,0(%r7)
   bl record
   lwz %r3,0(%r4)
   lis %r11,0x1234
   addi %r11,%r11,0x5678
   cmpw %r3,%r11
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # sthx
0: addi %r7,%r4,2
   mr %r0,%r7
   li %r11,-2
   li %r3,0x4321
   sthx %r3,%r7,%r11
   bl record
   li %r3,0
   lha %r3,0(%r4)
   cmpwi %r3,0x4321
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r0,2
   li %r3,0x3214
   sthx %r3,0,%r4
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0x3214
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # sthu
0: addi %r7,%r4,2
   li %r3,0x1234
   sthu %r3,-2(%r7)
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0x1234
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # sthux
0: addi %r7,%r4,2
   li %r11,-2
   li %r3,0x4321
   sthux %r3,%r7,%r11
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0x4321
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stw was tested in the bootstrap code.

   # stwx
0: li %r3,0
   stw %r3,0(%r4)
   stw %r3,4(%r4)
   addi %r7,%r4,4
   mr %r0,%r7
   li %r11,-4
   lis %r3,0x0123
   addi %r3,%r3,0x4567
   stwx %r3,%r7,%r11
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0x0123
   bne 1f
   lha %r3,2(%r4)
   cmpwi %r3,0x4567
   bne 1f
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r0,4
   lis %r3,0x7654
   addi %r3,%r3,0x3210
   stwx %r3,0,%r4
   bl record
   lha %r3,0(%r4)
   lha %r8,2(%r4)
   cmpwi %r3,0x7654
   bne 1f
   cmpwi %r8,0x3210
   beq 0f
1: sth %r3,12(%r6)
   sth %r8,14(%r6)
   addi %r6,%r6,32

   # stwu
0: addi %r7,%r4,4
   lis %r3,0x1234
   addi %r3,%r3,0x5678
   stwu %r3,-4(%r7)
   bl record
   lha %r3,0(%r4)
   lha %r8,2(%r4)
   cmpwi %r3,0x1234
   bne 1f
   cmpwi %r8,0x5678
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: sth %r3,12(%r6)
   sth %r8,14(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stwux
0: addi %r7,%r4,4
   li %r11,-4
   lis %r3,0x7654
   addi %r3,%r3,0x3210
   stwux %r3,%r7,%r11
   bl record
   lha %r3,0(%r4)
   lha %r8,2(%r4)
   cmpwi %r3,0x7654
   bne 1f
   cmpwi %r8,0x3210
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: sth %r3,12(%r6)
   sth %r8,14(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.4 Fixed-Point Load and Store with Byte Reversal Instructions
   ########################################################################

   # lhbrx
0: li %r3,0
   stw %r3,4(%r4)
   addi %r7,%r4,4
   li %r11,-4
   li %r3,0x1234
   sth %r3,0(%r4)
   lhbrx %r3,%r7,%r11
   bl record
   cmpwi %r3,0x3412
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r0,4
   lhbrx %r3,0,%r4
   bl record
   cmpwi %r3,0x3412
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # lwbrx
0: lis %r3,0x0123
   addi %r3,%r3,0x4567
   stw %r3,0(%r4)
   lwbrx %r3,%r7,%r11
   bl record
   lis %r11,0x6745
   addi %r11,%r11,0x2301
   cmpw %r3,%r11
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,0
   li %r0,4
   lwbrx %r3,0,%r4
   bl record
   lis %r11,0x6745
   addi %r11,%r11,0x2301
   cmpw %r3,%r11
   beq 0f

   # sthbrx
0: addi %r7,%r4,4
   li %r11,-4
   li %r3,0x1234
   sthbrx %r3,%r7,%r11
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0x3412
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r0,4
   li %r3,0x5678
   sthbrx %r3,0,%r4
   bl record
   lha %r3,0(%r4)
   cmpwi %r3,0x7856
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # stwbrx
0: addi %r7,%r4,4
   li %r11,-4
   lis %r3,0x0123
   addi %r3,%r3,0x4567
   stwbrx %r3,%r7,%r11
   bl record
   lwz %r3,0(%r4)
   lis %r11,0x6745
   addi %r11,%r11,0x2301
   cmpw %r3,%r11
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r0,4
   lis %r3,0x7654
   addi %r3,%r3,0x3210
   stwbrx %r3,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r11,0x1032
   addi %r11,%r11,0x5476
   cmpw %r3,%r11
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.5 Fixed-Point Load and Store Multiple Instructions
   ########################################################################

0: li %r3,-1
   stw %r3,0(%r4)
   stw %r3,4(%r4)
   stw %r3,8(%r4)
   stw %r3,12(%r4)
   stw %r3,16(%r4)
   stw %r3,20(%r4)
   stw %r3,24(%r4)
   stw %r3,28(%r4)
   stw %r3,32(%r4)
   stw %r3,252(%r4)
   stw %r3,256(%r4)
   stw %r3,260(%r4)

   # stmw
   li %r24,24
   li %r25,25
   li %r26,26
   li %r27,27
   li %r28,28
   li %r29,29
   li %r30,30
   li %r31,31
   addi %r7,%r4,4
   stmw %r24,-4(%r7)
   bl record
   lwz %r3,0(%r4)
   li %r7,24
   cmpwi %r3,24
   bne 1f
   lwz %r3,4(%r4)
   li %r7,25
   cmpwi %r3,25
   bne 1f
   lwz %r3,8(%r4)
   li %r7,26
   cmpwi %r3,26
   bne 1f
   lwz %r3,12(%r4)
   li %r7,27
   cmpwi %r3,27
   bne 1f
   lwz %r3,16(%r4)
   li %r7,28
   cmpwi %r3,28
   bne 1f
   lwz %r3,20(%r4)
   li %r7,29
   cmpwi %r3,29
   bne 1f
   lwz %r3,24(%r4)
   li %r7,30
   cmpwi %r3,30
   bne 1f
   lwz %r3,28(%r4)
   li %r7,31
   cmpwi %r3,31
   bne 1f
   lwz %r3,32(%r4)
   li %r7,32
   cmpwi %r3,-1
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   addi %r6,%r6,32

0: li %r30,300
   li %r31,310
   addi %r7,%r4,-0x7F00
   stmw %r30,0x7FFC(%r7)  # Offset should not wrap around to -0x8000.
   bl record
   lwz %r3,252(%r4)
   li %r7,30
   cmpwi %r3,300
   bne 1f
   lwz %r3,256(%r4)
   li %r7,31
   cmpwi %r3,310
   bne 1f
   lwz %r3,260(%r4)
   li %r7,32
   cmpwi %r3,-1
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   addi %r6,%r6,32

   # lmw
0: li %r24,-2
   li %r25,-2
   li %r26,-2
   li %r27,-2
   li %r28,-2
   li %r29,-2
   li %r30,-2
   li %r31,-2
   addi %r7,%r4,4
   lmw %r24,-4(%r7)
   bl record
   mr %r3,%r24
   li %r7,24
   cmpwi %r3,24
   bne 1f
   mr %r3,%r25
   li %r7,25
   cmpwi %r3,25
   bne 1f
   mr %r3,%r26
   li %r7,26
   cmpwi %r3,26
   bne 1f
   mr %r3,%r27
   li %r7,27
   cmpwi %r3,27
   bne 1f
   mr %r3,%r28
   li %r7,28
   cmpwi %r3,28
   bne 1f
   mr %r3,%r29
   li %r7,29
   cmpwi %r3,29
   bne 1f
   mr %r3,%r30
   li %r7,30
   cmpwi %r3,30
   bne 1f
   mr %r3,%r31
   li %r7,31
   cmpwi %r3,31
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   addi %r6,%r6,32

0: li %r30,-2
   li %r31,-2
   addi %r7,%r4,-0x7F00
   lmw %r30,0x7FFC(%r7)  # Offset should not wrap around to -0x8000.
   bl record
   mr %r3,%r30
   li %r7,30
   cmpwi %r3,300
   bne 1f
   mr %r3,%r31
   li %r7,31
   cmpwi %r3,310
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.6 Fixed-Point Move Assist Instructions
   # 3.3.13 Move To/From System Register Instructions - mtxer
   ########################################################################

0: li %r3,0
   li %r0,256
   mtctr %r0
1: stbx %r3,%r4,%r3
   addi %r3,%r3,1
   bdnz 1b

   # lswi (NB!=0, no register wrap)
0: addi %r7,%r4,1
   lswi %r24,%r7,9
   bl record
   lis %r3,0x0102
   addi %r3,%r3,0x0304
   cmpw %r24,%r3
   bne 1f
   lis %r3,0x0506
   addi %r3,%r3,0x0708
   cmpw %r25,%r3
   bne 1f
   lis %r3,0x0900
   cmpw %r26,%r3
   beq 0f
1: stw %r24,8(%r6)
   stw %r25,16(%r6)
   stw %r26,20(%r6)
   addi %r6,%r6,32

   # lswi (NB==0, register wrap)
0: addi %r7,%r4,18
   lswi %r25,%r7,32
   bl record
   lis %r3,0x1213
   addi %r3,%r3,0x1415
   cmpw %r25,%r3
   bne 1f
   lis %r3,0x1617
   addi %r3,%r3,0x1819
   cmpw %r26,%r3
   bne 1f
   lis %r3,0x1A1B
   addi %r3,%r3,0x1C1D
   cmpw %r27,%r3
   bne 1f
   lis %r3,0x1E1F
   addi %r3,%r3,0x2021
   cmpw %r28,%r3
   bne 1f
   lis %r3,0x2223
   addi %r3,%r3,0x2425
   cmpw %r29,%r3
   bne 1f
   lis %r3,0x2627
   addi %r3,%r3,0x2829
   cmpw %r30,%r3
   bne 1f
   lis %r3,0x2A2B
   addi %r3,%r3,0x2C2D
   cmpw %r31,%r3
   bne 1f
   lis %r3,0x2E2F
   addi %r3,%r3,0x3031
   cmpw %r0,%r3
   beq 0f
1: stw %r24,8(%r6)
   stw %r31,16(%r6)
   stw %r0,20(%r6)
   addi %r6,%r6,32

   # lswx (RA!=0, register wrap)
0: li %r3,9
   mtxer %r3
   li %r7,1
   lswx %r30,%r4,%r7
   bl record
   lis %r3,0x0102
   addi %r3,%r3,0x0304
   cmpw %r30,%r3
   bne 1f
   lis %r3,0x0506
   addi %r3,%r3,0x0708
   cmpw %r31,%r3
   bne 1f
   lis %r3,0x0900
   cmpw %r0,%r3
   beq 0f
1: stw %r30,8(%r6)
   stw %r31,16(%r6)
   stw %r0,20(%r6)
   addi %r6,%r6,32

   # lswx (RA==0, no register wrap)
0: li %r3,11
   mtxer %r3
   addi %r7,%r4,8
   li %r0,2
   lswx %r26,%r0,%r7
   bl record
   lis %r3,0x0809
   addi %r3,%r3,0x0A0B
   cmpw %r26,%r3
   bne 1f
   lis %r3,0x0C0D
   addi %r3,%r3,0x0E0F
   cmpw %r27,%r3
   bne 1f
   lis %r3,0x1011
   addi %r3,%r3,0x1200
   cmpw %r28,%r3
   beq 0f
1: stw %r26,8(%r6)
   stw %r27,16(%r6)
   stw %r28,20(%r6)
   addi %r6,%r6,32

   # lswx (count==0)
0: li %r25,-1
   li %r3,0
   mtxer %r3
   lswx %r24,%r0,%r4
   bl record
   cmpwi %r25,-1
   beq 0f
   stw %r25,8(%r6)
   addi %r6,%r6,32

0: lis %r24,0x5051
   addi %r24,%r24,0x5253
   lis %r25,0x5455
   addi %r25,%r25,0x5657
   lis %r26,0x5859
   addi %r26,%r26,0x5A5B
   lis %r27,0x5C5D
   addi %r27,%r27,0x5E5F
   lis %r28,0x6061
   addi %r28,%r28,0x6263
   lis %r29,0x6465
   addi %r29,%r29,0x6667
   lis %r30,0x6869
   addi %r30,%r30,0x6A6B
   lis %r31,0x6C6D
   addi %r31,%r31,0x6E6F
   lis %r3,0x7071
   addi %r3,%r3,0x7273
   mr %r0,%r3

   # stswi (NB!=0, no register wrap)
0: addi %r7,%r4,1
   stswi %r24,%r7,9
   bl record
   lbz %r3,1(%r4)
   lhz %r8,2(%r4)
   lhz %r9,4(%r4)
   lhz %r10,6(%r4)
   lhz %r11,8(%r4)
   cmpwi %r3,0x50
   bne 1f
   cmpwi %r8,0x5152
   bne 1f
   cmpwi %r9,0x5354
   bne 1f
   cmpwi %r10,0x5556
   bne 1f
   cmpwi %r11,0x5758
   beq 0f
1: sth %r3,8(%r6)
   sth %r8,10(%r6)
   sth %r9,12(%r6)
   sth %r10,14(%r6)
   sth %r11,16(%r6)
   addi %r6,%r6,32

   # stswi (NB==0, register wrap)
0: addi %r7,%r4,18
   stswi %r25,%r7,32
   bl record
   lhz %r3,18(%r4)
   cmpwi %r3,0x5455
   bne 1f
   lhz %r3,20(%r4)
   cmpwi %r3,0x5657
   bne 1f
   lhz %r3,22(%r4)
   cmpwi %r3,0x5859
   bne 1f
   lhz %r3,24(%r4)
   cmpwi %r3,0x5A5B
   bne 1f
   lhz %r3,26(%r4)
   cmpwi %r3,0x5C5D
   bne 1f
   lhz %r3,28(%r4)
   cmpwi %r3,0x5E5F
   bne 1f
   lhz %r3,30(%r4)
   cmpwi %r3,0x6061
   bne 1f
   lhz %r3,32(%r4)
   cmpwi %r3,0x6263
   bne 1f
   lhz %r3,34(%r4)
   cmpwi %r3,0x6465
   bne 1f
   lhz %r3,36(%r4)
   cmpwi %r3,0x6667
   bne 1f
   lhz %r3,38(%r4)
   cmpwi %r3,0x6869
   bne 1f
   lhz %r3,40(%r4)
   cmpwi %r3,0x6A6B
   bne 1f
   lhz %r3,42(%r4)
   cmpwi %r3,0x6C6D
   bne 1f
   lhz %r3,44(%r4)
   cmpwi %r3,0x6E6F
   bne 1f
   lhz %r3,46(%r4)
   cmpwi %r3,0x7071
   bne 1f
   lhz %r3,48(%r4)
   cmpwi %r3,0x7273
   beq 0f
1: stw %r3,8(%r6)
   addi %r6,%r6,32

   # stswx (RA!=0, register wrap)
0: li %r3,9
   mtxer %r3
   li %r7,1
   stswx %r30,%r4,%r7
   bl record
   lbz %r3,1(%r4)
   lhz %r8,2(%r4)
   lhz %r9,4(%r4)
   lhz %r10,6(%r4)
   lhz %r11,8(%r4)
   cmpwi %r3,0x68
   bne 1f
   cmpwi %r8,0x696A
   bne 1f
   cmpwi %r9,0x6B6C
   bne 1f
   cmpwi %r10,0x6D6E
   bne 1f
   cmpwi %r11,0x6F70
   beq 0f
1: sth %r3,8(%r6)
   sth %r8,10(%r6)
   sth %r9,12(%r6)
   sth %r10,14(%r6)
   sth %r11,16(%r6)
   addi %r6,%r6,32

   # stswx (RA==0, no register wrap)
0: li %r3,11
   mtxer %r3
   addi %r7,%r4,8
   li %r0,2
   stswx %r26,%r0,%r7
   bl record
   lhz %r3,8(%r4)
   cmpwi %r3,0x5859
   bne 1f
   lhz %r3,10(%r4)
   cmpwi %r3,0x5A5B
   bne 1f
   lhz %r3,12(%r4)
   cmpwi %r3,0x5C5D
   bne 1f
   lhz %r3,14(%r4)
   cmpwi %r3,0x5E5F
   bne 1f
   lhz %r3,16(%r4)
   cmpwi %r3,0x6061
   bne 1f
   lbz %r3,18(%r4)
   cmpwi %r3,0x62
   beq 0f
1: stw %r3,8(%r6)
   addi %r6,%r6,32

   # stswx (count==0)
0: li %r24,-1
   li %r3,0
   mtxer %r3
   li %r3,-2
   stb %r3,0(%r4)
   stswx %r24,%r0,%r4
   bl record
   lbz %r3,0(%r4)
   cmpwi %r3,0xFE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.13 Move To/From System Register Instructions - mfxer, mtxer
   ########################################################################

   # mfxer
0: li %r3,0x12
   mtxer %r3
   li %r3,-1
   addic. %r3,%r3,1
   mfxer %r3
   bl record
   lis %r7,0x2000
   addi %r7,%r7,0x12
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtxer (check that it overwrites the CA bit)
0: li %r3,0x34
   mtxer %r3
   bl record
   li %r3,0
   mfxer %r3
   cmpwi %r3,0x34
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - addi, addis, add, subf
   ########################################################################

0: li %r3,0
   mtxer %r3
   mtcr %r3

   # addi and addis were tested in the bootstrap code.

   # add
0: li %r8,-4
   li %r9,9
   add %r3,%r8,%r9
   bl record
   li %r7,5
   bl check_alu

   # add.
0: li %r8,-4
   li %r9,3
   add. %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r8,1
   add. %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,1
   add. %r3,%r3,%r9
   bl record
   li %r7,1
   bl check_alu_gt

   # addo
0: lis %r8,0x4000
   lis %r9,0x3000
   addo %r3,%r8,%r9
   bl record
   lis %r7,0x7000
   bl check_alu
0: lis %r8,0x3000
   addo %r3,%r3,%r8
   bl record
   lis %r7,0xA000
   bl check_alu_ov
   # Also check that SO is preserved even when OV is cleared.
0: lis %r8,0x4000
   lis %r9,0x3000
   addo %r3,%r8,%r9
   addo %r3,%r9,%r3
   addo %r3,%r9,%r3
   bl record
   lis %r7,0xD000
   bl check_alu_so

   # addo.
0: lis %r8,0x4000
   lis %r9,0x3000
   addo. %r3,%r8,%r9
   bl record
   lis %r7,0x7000
   bl check_alu_gt
0: lis %r8,0x3000
   addo. %r3,%r3,%r8
   bl record
   lis %r7,0xA000
   bl check_alu_ov_lt
0: lis %r9,0x6000
   addo. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r8,0x4000
   lis %r9,0x3000
   addo. %r3,%r8,%r9
   addo. %r3,%r9,%r3
   addo. %r3,%r9,%r3
   bl record
   lis %r7,0xD000
   bl check_alu_so_lt

   # subf
0: li %r8,4
   li %r9,9
   subf %r3,%r8,%r9
   bl record
   li %r7,5
   bl check_alu

   # subf.
0: li %r8,4
   li %r9,3
   subf. %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r8,-1
   subf. %r3,%r8,%r3
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-1
   subf. %r3,%r9,%r3
   bl record
   li %r7,1
   bl check_alu_gt

   # subfo
0: lis %r8,0xC000
   lis %r9,0x3000
   subfo %r3,%r9,%r8
   bl record
   lis %r7,0x9000
   bl check_alu
0: lis %r8,0x3000
   subfo %r3,%r8,%r3
   bl record
   lis %r7,0x6000
   bl check_alu_ov
0: lis %r8,0xC000
   lis %r9,0x3000
   subfo %r3,%r9,%r8
   subfo %r3,%r9,%r3
   subfo %r3,%r9,%r3
   bl record
   lis %r7,0x3000
   bl check_alu_so
   # Check that overflow from the +1 in ~(RA)+(RB)+1 is detected.
0: lis %r8,0xC000
   lis %r9,0x4000
   subfo %r7,%r8,%r9
   bl record
   mr %r3,%r7
   lis %r7,0x8000
   bl check_alu_ov

   # subfo.
0: lis %r8,0xC000
   lis %r9,0x3000
   subfo. %r3,%r9,%r8
   bl record
   lis %r7,0x9000
   bl check_alu_lt
0: lis %r8,0x3000
   subfo. %r3,%r8,%r3
   bl record
   lis %r7,0x6000
   bl check_alu_ov_gt
0: lis %r9,0x6000
   subfo. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r8,0xC000
   lis %r9,0x3000
   subfo. %r3,%r9,%r8
   subfo. %r3,%r9,%r3
   subfo. %r3,%r9,%r3
   bl record
   lis %r7,0x3000
   bl check_alu_so_gt
0: lis %r8,0xC000
   lis %r9,0x4000
   subfo. %r7,%r8,%r9
   bl record
   mr %r3,%r7
   lis %r7,0x8000
   bl check_alu_ov_lt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - addic, addic., subfic, addc, subfc
   ########################################################################

   # addic
   # Use r0 here to verify that it doesn't get treated as zero like addi/addis.
0: li %r0,1
   addic %r3,%r0,2
   bl record
   li %r7,3
   bl check_alu
0: li %r0,-1
   addic %r3,%r0,3
   bl record
   li %r7,2
   bl check_alu_ca
   # XER[CA] should not be an input to the add.
0: li %r0,-1
   addic %r3,%r0,3
   addic %r3,%r3,0
   bl record
   li %r7,2
   bl check_alu

   # addic.
0: li %r0,-3
   addic. %r3,%r0,1
   bl record
   li %r7,-2
   bl check_alu_lt
0: addic. %r3,%r3,2
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r0,-1
   addic. %r3,%r0,3
   addic. %r3,%r3,0
   bl record
   li %r7,2
   bl check_alu_gt

   # subfic
0: li %r0,2
   subfic %r3,%r0,1
   bl record
   li %r7,-1
   bl check_alu
0: li %r0,1
   subfic %r3,%r0,1
   bl record
   li %r7,0
   bl check_alu_ca

   # addc
0: li %r8,-4
   li %r9,3
   addc %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu
0: li %r8,2
   addc %r3,%r3,%r8
   bl record
   li %r7,1
   bl check_alu_ca

   # addc.
0: li %r8,-4
   li %r9,3
   addc. %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r8,1
   addc. %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r9,1
   addc. %r3,%r3,%r9
   bl record
   li %r7,1
   bl check_alu_gt

   # addco
0: lis %r8,0x4000
   lis %r9,0x3000
   addco %r3,%r8,%r9
   bl record
   lis %r7,0x7000
   bl check_alu
0: lis %r8,0x3000
   addco %r3,%r3,%r8
   bl record
   lis %r7,0xA000
   bl check_alu_ov
0: lis %r9,0x7000
   addco %r3,%r3,%r9
   bl record
   lis %r7,0x1000
   bl check_alu_ca
0: lis %r8,0x4000
   lis %r9,0x6000
   addco %r3,%r8,%r9
   addco %r3,%r9,%r3
   bl record
   li %r7,0
   bl check_alu_ca_so

   # addco.
0: lis %r8,0x4000
   lis %r9,0x3000
   addco. %r3,%r8,%r9
   bl record
   lis %r7,0x7000
   bl check_alu_gt
0: lis %r8,0x3000
   addco. %r3,%r3,%r8
   bl record
   lis %r7,0xA000
   bl check_alu_ov_lt
0: lis %r9,0x6000
   addco. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: lis %r8,0x5000
   lis %r9,0x6000
   addco. %r3,%r8,%r9
   addco. %r3,%r9,%r3
   bl record
   lis %r7,0x1000
   bl check_alu_ca_so_gt

   # subfc
0: li %r8,4
   li %r9,9
   subfc %r3,%r8,%r9
   bl record
   li %r7,5
   bl check_alu_ca
0: li %r8,4
   li %r9,9
   subfc %r3,%r9,%r8
   bl record
   li %r7,-5
   bl check_alu

   # subfc.
0: li %r8,4
   li %r9,3
   subfc. %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r8,-1
   subfc. %r3,%r8,%r3
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r9,-1
   subfc. %r3,%r9,%r3
   bl record
   li %r7,1
   bl check_alu_gt
0: li %r9,3
   subfc. %r3,%r3,%r9
   bl record
   li %r7,2
   bl check_alu_ca_gt

   # subfco
0: lis %r8,0xC000
   lis %r9,0x3000
   subfco %r3,%r9,%r8
   bl record
   lis %r7,0x9000
   bl check_alu_ca
0: lis %r8,0x3000
   subfco %r3,%r8,%r3
   bl record
   lis %r7,0x6000
   bl check_alu_ca_ov
0: lis %r8,0xC000
   lis %r9,0x3000
   subfco %r3,%r9,%r8
   subfco %r3,%r9,%r3
   subfco %r3,%r9,%r3
   bl record
   lis %r7,0x3000
   bl check_alu_ca_so
0: lis %r8,0xC000
   lis %r9,0x4000
   subfco %r7,%r8,%r9
   bl record
   mr %r3,%r7
   lis %r7,0x8000
   bl check_alu_ov

   # subfco.
0: lis %r8,0xC000
   lis %r9,0x3000
   subfco. %r3,%r9,%r8
   bl record
   lis %r7,0x9000
   bl check_alu_ca_lt
0: lis %r8,0x3000
   subfco. %r3,%r8,%r3
   bl record
   lis %r7,0x6000
   bl check_alu_ca_ov_gt
0: lis %r9,0x6000
   subfco. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: lis %r8,0xC000
   lis %r9,0x3000
   subfco. %r3,%r9,%r8
   subfco. %r3,%r9,%r3
   subfco. %r3,%r9,%r3
   bl record
   lis %r7,0x3000
   bl check_alu_ca_so_gt
0: lis %r8,0xC000
   lis %r9,0x4000
   subfco. %r7,%r8,%r9
   bl record
   mr %r3,%r7
   lis %r7,0x8000
   bl check_alu_ov_lt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - adde, subfe
   ########################################################################

   # adde
0: li %r8,-4
   li %r9,3
   adde %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu
0: li %r8,2
   adde %r3,%r3,%r8
   bl record
   li %r7,1
   bl check_alu_ca
   # Check that the carry bit is added and carry resulting from adding the
   # carry bit is detected.
0: li %r3,-1
   li %r8,2
   adde %r3,%r8,%r3
   li %r9,-2
   adde %r3,%r9,%r3
   bl record
   li %r7,0
   bl check_alu_ca

   # adde.
0: li %r8,-4
   li %r9,3
   adde. %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r8,1
   adde. %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r9,1
   adde. %r3,%r3,%r9
   bl record
   li %r7,1
   bl check_alu_gt
0: li %r3,-1
   li %r8,2
   adde. %r3,%r8,%r3
   li %r9,-2
   adde. %r3,%r9,%r3
   bl record
   li %r7,0
   bl check_alu_ca_eq

   # addeo
0: lis %r8,0x4000
   lis %r9,0x3000
   addeo %r3,%r8,%r9
   bl record
   lis %r7,0x7000
   bl check_alu
0: lis %r8,0x3000
   addeo %r3,%r3,%r8
   bl record
   lis %r7,0xA000
   bl check_alu_ov
0: lis %r9,0x7000
   addeo %r3,%r3,%r9
   bl record
   lis %r7,0x1000
   bl check_alu_ca
0: lis %r8,0xC000
   lis %r9,0xA000
   addeo %r3,%r8,%r9
   addi %r9,%r9,-1
   addeo %r3,%r9,%r3
   bl record
   li %r7,0
   bl check_alu_ca_so

   # addeo.
0: lis %r8,0x4000
   lis %r9,0x3000
   addeo. %r3,%r8,%r9
   bl record
   lis %r7,0x7000
   bl check_alu_gt
0: lis %r8,0x3000
   addeo. %r3,%r3,%r8
   bl record
   lis %r7,0xA000
   bl check_alu_ov_lt
0: lis %r9,0x6000
   addeo. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: lis %r8,0xC000
   lis %r9,0xA000
   addeo. %r3,%r8,%r9
   addi %r9,%r9,-1
   addeo. %r3,%r9,%r3
   bl record
   li %r7,0
   bl check_alu_ca_so_eq

   # subfe
0: li %r8,4
   li %r9,9
   subfe %r3,%r8,%r9
   bl record
   li %r7,4
   bl check_alu_ca
0: li %r8,4
   li %r9,9
   subfe %r3,%r9,%r8
   bl record
   li %r7,-6
   bl check_alu
0: li %r8,4
   li %r9,9
   subfe %r3,%r8,%r9
   subfe %r3,%r8,%r3
   bl record
   li %r7,0
   bl check_alu_ca

   # subfe.
0: li %r8,3
   li %r9,3
   subfe. %r3,%r8,%r9
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r8,-2
   subfe. %r3,%r8,%r3
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r9,-2
   subfe. %r3,%r9,%r3
   bl record
   li %r7,1
   bl check_alu_gt
0: li %r9,4
   subfe. %r3,%r3,%r9
   bl record
   li %r7,2
   bl check_alu_ca_gt
0: li %r9,4
   subfe. %r3,%r3,%r9
   li %r8,1
   subfe. %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu_ca_eq

   # subfeo
0: lis %r8,0xC000
   lis %r9,0x3000
   subfeo %r3,%r9,%r8
   bl record
   lis %r7,0x9000
   addi %r7,%r7,-1
   bl check_alu_ca
0: lis %r8,0x3000
   subfeo %r3,%r8,%r3
   bl record
   lis %r7,0x6000
   addi %r7,%r7,-2
   bl check_alu_ca_ov
0: lis %r8,0xC000
   lis %r9,0x3000
   subfeo %r3,%r9,%r8
   subfeo %r3,%r9,%r3
   subfeo %r3,%r9,%r3
   bl record
   lis %r7,0x3000
   addi %r7,%r7,-1
   bl check_alu_ca_so
0: lis %r8,0xC000
   lis %r9,0x8000
   addi %r9,%r9,-1
   subfeo %r9,%r9,%r8
   subfeo %r7,%r8,%r9
   bl record
   mr %r3,%r7
   lis %r7,0x8000
   bl check_alu_ov

   # subfeo.
0: lis %r8,0xC000
   lis %r9,0x3000
   subfeo. %r3,%r9,%r8
   bl record
   lis %r7,0x9000
   addi %r7,%r7,-1
   bl check_alu_ca_lt
0: lis %r8,0x3000
   subfeo. %r3,%r8,%r3
   bl record
   lis %r7,0x6000
   addi %r7,%r7,-2
   bl check_alu_ca_ov_gt
0: lis %r9,0x6000
   addi %r9,%r9,-1
   subfeo. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: lis %r8,0xC000
   lis %r9,0x3000
   subfeo. %r3,%r9,%r8
   subfeo. %r3,%r9,%r3
   subfeo. %r3,%r9,%r3
   bl record
   lis %r7,0x3000
   addi %r7,%r7,-1
   bl check_alu_ca_so_gt
0: lis %r8,0xC000
   lis %r9,0x8000
   addi %r9,%r9,-1
   subfeo %r9,%r9,%r8
   subfeo. %r7,%r8,%r9
   bl record
   mr %r3,%r7
   lis %r7,0x8000
   bl check_alu_ov_lt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - addme, subfme
   ########################################################################

   # addme
0: li %r8,0
   addme %r3,%r8
   bl record
   li %r7,-1
   bl check_alu
0: li %r9,1
   addme %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca
0: li %r9,1
   addme %r3,%r9
   addme %r3,%r3
   bl record
   li %r7,0
   bl check_alu_ca

   # addme.
0: li %r8,0
   addme. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,1
   addme. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r7,2
   addme. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_ca_gt

   # addmeo
0: li %r8,0
   addmeo %r3,%r8
   bl record
   li %r7,-1
   bl check_alu
0: li %r9,1
   addmeo %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca
0: lis %r7,0x8000
   addmeo %r3,%r7
   bl record
   lis %r7,0x8000
   addi %r7,%r7,-1
   bl check_alu_ca_ov

   # addmeo.
0: li %r8,0
   addmeo. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,1
   addmeo. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r7,2
   addmeo. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_ca_gt
0: lis %r3,0x8000
   addmeo. %r3,%r3
   bl record
   lis %r7,0x8000
   addi %r7,%r7,-1
   bl check_alu_ca_ov_gt

   # subfme
0: li %r8,-1
   subfme %r3,%r8
   bl record
   li %r7,-1
   bl check_alu
0: li %r9,-2
   subfme %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca
0: li %r9,-2
   subfme %r3,%r9
   li %r3,-1
   subfme %r3,%r3
   bl record
   li %r7,0
   bl check_alu_ca

   # subfme.
0: li %r8,-1
   subfme. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,-2
   subfme. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r7,-3
   subfme. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_ca_gt

   # subfmeo
0: li %r8,-1
   subfmeo %r3,%r8
   bl record
   li %r7,-1
   bl check_alu
0: li %r9,-2
   subfmeo %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca
0: lis %r7,0x8000
   addi %r7,%r7,-1
   subfmeo %r3,%r7
   bl record
   lis %r7,0x8000
   addi %r7,%r7,-1
   bl check_alu_ca_ov

   # subfmeo.
0: li %r8,-1
   subfmeo. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,-2
   subfmeo. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: li %r7,-3
   subfmeo. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_ca_gt
0: lis %r3,0x8000
   addi %r3,%r3,-1
   subfmeo. %r3,%r3
   bl record
   lis %r7,0x8000
   addi %r7,%r7,-1
   bl check_alu_ca_ov_gt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - addze, subfze
   ########################################################################

   # addze
0: li %r8,1
   addze %r3,%r8
   bl record
   li %r7,1
   bl check_alu
   # addze and subfze can never cause a carry without an incoming carry bit.
0: lis %r0,0x2000
   mtxer %r0
   li %r9,-1
   addze %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca

   # addze.
0: li %r8,-1
   addze. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,0
   addze. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r7,1
   addze. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_gt
0: lis %r0,0x2000
   mtxer %r0
   li %r9,-1
   addze. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq

   # addzeo
0: li %r8,1
   addzeo %r3,%r8
   bl record
   li %r7,1
   bl check_alu
0: lis %r0,0x2000
   mtxer %r0
   li %r9,-1
   addze %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca
0: lis %r0,0x2000
   mtxer %r0
   lis %r7,0x8000
   addi %r7,%r7,-1
   addzeo %r3,%r7
   bl record
   lis %r7,0x8000
   bl check_alu_ov

   # addzeo.
0: li %r8,-1
   addzeo. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,0
   addzeo. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r7,1
   addzeo. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_gt
0: lis %r0,0x2000
   mtxer %r0
   li %r0,-1
   addze. %r3,%r0
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: lis %r0,0x2000
   mtxer %r0
   lis %r3,0x8000
   addi %r3,%r3,-1
   addzeo. %r3,%r3
   bl record
   lis %r7,0x8000
   bl check_alu_ov_lt

   # subfze
0: li %r8,-2
   subfze %r3,%r8
   bl record
   li %r7,1
   bl check_alu
0: lis %r0,0x2000
   mtxer %r0
   li %r9,0
   subfze %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca

   # subfze.
0: li %r8,0
   subfze. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,-1
   subfze. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r7,-2
   subfze. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_gt
0: lis %r0,0x2000
   mtxer %r0
   li %r9,0
   subfze. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca_eq

   # subfzeo
0: li %r8,-2
   subfzeo %r3,%r8
   bl record
   li %r7,1
   bl check_alu
0: lis %r0,0x2000
   mtxer %r0
   li %r9,0
   subfze %r3,%r9
   bl record
   li %r7,0
   bl check_alu_ca
0: lis %r0,0x2000
   mtxer %r0
   lis %r7,0x8000
   subfzeo %r3,%r7
   bl record
   lis %r7,0x8000
   bl check_alu_ov

   # subfzeo.
0: li %r8,0
   subfzeo. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,-1
   subfzeo. %r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r7,-2
   subfzeo. %r3,%r7
   bl record
   li %r7,1
   bl check_alu_gt
0: lis %r0,0x2000
   mtxer %r0
   li %r0,0
   subfze. %r3,%r0
   bl record
   li %r7,0
   bl check_alu_ca_eq
0: lis %r0,0x2000
   mtxer %r0
   lis %r3,0x8000
   subfzeo. %r3,%r3
   bl record
   lis %r7,0x8000
   bl check_alu_ov_lt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - neg
   ########################################################################

   # neg
0: li %r8,1
   neg %r3,%r8
   bl record
   li %r7,-1
   bl check_alu
0: li %r9,-2
   neg %r3,%r9
   bl record
   li %r7,2
   bl check_alu
0: li %r7,0
   neg %r3,%r7
   bl record
   li %r7,0
   bl check_alu
0: lis %r3,0x8000
   neg %r3,%r3
   bl record
   lis %r7,0x8000
   bl check_alu

   # neg.
0: li %r8,1
   neg. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,-2
   neg. %r3,%r9
   bl record
   li %r7,2
   bl check_alu_gt
0: li %r7,0
   neg. %r3,%r7
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r3,0x8000
   neg. %r3,%r3
   bl record
   lis %r7,0x8000
   bl check_alu_lt

   # nego
0: li %r8,1
   nego %r3,%r8
   bl record
   li %r7,-1
   bl check_alu
0: li %r9,-2
   nego %r3,%r9
   bl record
   li %r7,2
   bl check_alu
0: li %r7,0
   nego %r3,%r7
   bl record
   li %r7,0
   bl check_alu
0: lis %r3,0x8000
   nego %r3,%r3
   bl record
   lis %r7,0x8000
   bl check_alu_ov

   # nego.
0: li %r8,1
   nego. %r3,%r8
   bl record
   li %r7,-1
   bl check_alu_lt
0: li %r9,-2
   nego. %r3,%r9
   bl record
   li %r7,2
   bl check_alu_gt
0: li %r7,0
   nego. %r3,%r7
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r3,0x8000
   nego. %r3,%r3
   bl record
   lis %r7,0x8000
   bl check_alu_ov_lt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - mulli
   ########################################################################

   # mulli
0: li %r8,3
   mulli %r3,%r8,5
   bl record
   li %r7,15
   bl check_alu
0: mulli %r3,%r3,-7
   bl record
   li %r7,-105
   bl check_alu
0: lis %r8,0x10
   addi %r8,%r8,1
   mulli %r3,%r8,0x1111
   bl record
   lis %r7,0x1110
   addi %r7,%r7,0x1111
   bl check_alu

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - mullw
   ########################################################################

   # mullw
0: li %r8,3
   li %r9,5
   mullw %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu
0: li %r8,-7
   mullw %r3,%r3,%r8
   bl record
   li %r7,-105
   bl check_alu
0: lis %r8,0x10
   addi %r8,%r8,1
   li %r7,0x1111
   mullw %r3,%r8,%r7
   bl record
   lis %r7,0x1110
   addi %r7,%r7,0x1111
   bl check_alu

   # mullw.
0: li %r8,3
   li %r9,5
   mullw. %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu_gt
0: li %r8,-7
   mullw. %r3,%r3,%r8
   bl record
   li %r7,-105
   bl check_alu_lt
0: li %r9,0
   mullw. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq

   # mullwo
0: li %r8,3
   li %r9,5
   mullwo %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu
0: li %r8,-7
   mullwo %r3,%r3,%r8
   bl record
   li %r7,-105
   bl check_alu
0: lis %r8,0x10
   addi %r8,%r8,1
   li %r7,0x1111
   mullwo %r3,%r8,%r7
   bl record
   lis %r7,0x1110
   addi %r7,%r7,0x1111
   bl check_alu_ov
0: lis %r8,0x10
   addi %r8,%r8,1
   li %r7,-0x1111
   mullwo %r3,%r7,%r8
   bl record
   lis %r7,-0x1110
   addi %r7,%r7,-0x1111
   bl check_alu_ov

   # mullwo.
0: li %r8,3
   li %r9,5
   mullwo. %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu_gt
0: li %r8,-7
   mullwo. %r3,%r3,%r8
   bl record
   li %r7,-105
   bl check_alu_lt
0: li %r9,0
   mullwo. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r8,0x10
   addi %r8,%r8,1
   li %r7,0x1111
   mullwo. %r3,%r8,%r7
   bl record
   lis %r7,0x1110
   addi %r7,%r7,0x1111
   bl check_alu_ov_gt
0: lis %r8,0x10
   addi %r8,%r8,1
   li %r7,-0x1111
   mullwo. %r3,%r7,%r8
   bl record
   lis %r7,-0x1110
   addi %r7,%r7,-0x1111
   bl check_alu_ov_lt

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - mulhw
   ########################################################################

   # mulhw
0: lis %r8,3
   lis %r9,5
   mulhw %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu
0: lis %r3,15
   lis %r8,-7
   mulhw %r3,%r3,%r8
   bl record
   li %r7,-105
   bl check_alu
0: lis %r8,0x1
   addi %r8,%r8,0x10
   lis %r7,0x1111
   mulhw %r3,%r8,%r7
   bl record
   li %r7,0x1112
   bl check_alu
.if TEST_UNDOCUMENTED
   # Check that the version of mulhw with the OE bit set doesn't affect XER
   # (OE is a reserved field in the mulh* instructions, but the CPU accepts
   # this encoding and treats it as if OE=0).
0: lis %r3,0x6000
   addis %r3,%r3,0x6000
   mtxer %r3
   lis %r8,3
   lis %r9,5
   .int 0x7C694496  # mulhw %r3,%r9,%r8
   bl record
   li %r7,15
   bl check_alu_ov
.endif

   # mulhw.
0: lis %r8,3
   lis %r9,5
   mulhw. %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu_undef
0: lis %r3,0x4000
   addis %r3,%r3,0x4000
   mtxer %r3
   lis %r3,15
   lis %r8,-7
   mulhw. %r3,%r3,%r8
   bl record
   li %r7,-105
   bl check_alu_so_undef
0: lis %r8,0x1
   addi %r8,%r8,0x10
   lis %r7,0x1111
   mulhw. %r3,%r8,%r7
   bl record
   li %r7,0x1112
   bl check_alu_undef
.if TEST_UNDOCUMENTED
0: lis %r3,0x6000
   addis %r3,%r3,0x6000
   mtxer %r3
   lis %r8,3
   lis %r9,5
   .int 0x7C694497  # mulhw. %r3,%r9,%r8
   bl record
   li %r7,15
   bl check_alu_ov_undef
.endif

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - mulhwu
   ########################################################################

   # mulhwu
0: lis %r8,3
   lis %r9,5
   mulhwu %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu
0: lis %r3,15
   lis %r8,-7
   mulhwu %r3,%r3,%r8
   bl record
   lis %r7,15
   addi %r7,%r7,-105
   bl check_alu
0: lis %r8,0x1
   addi %r8,%r8,0x10
   lis %r7,0x1111
   mulhwu %r3,%r8,%r7
   bl record
   li %r7,0x1112
   bl check_alu
.if TEST_UNDOCUMENTED
0: lis %r3,0x6000
   addis %r3,%r3,0x6000
   mtxer %r3
   lis %r8,3
   lis %r9,5
   .int 0x7C694416  # mulhwu %r3,%r9,%r8
   bl record
   li %r7,15
   bl check_alu_ov
.endif

   # mulhwu.
0: lis %r8,3
   lis %r9,5
   mulhwu. %r3,%r8,%r9
   bl record
   li %r7,15
   bl check_alu_undef
0: lis %r3,0x4000
   addis %r3,%r3,0x4000
   mtxer %r3
   lis %r3,15
   lis %r8,-7
   mulhwu. %r3,%r3,%r8
   bl record
   lis %r7,15
   addi %r7,%r7,-105
   bl check_alu_so_undef
0: lis %r8,0x1
   addi %r8,%r8,0x10
   lis %r7,0x1111
   mulhwu. %r3,%r8,%r7
   bl record
   li %r7,0x1112
   bl check_alu_undef
.if TEST_UNDOCUMENTED
0: lis %r3,0x6000
   addis %r3,%r3,0x6000
   mtxer %r3
   lis %r8,3
   lis %r9,5
   .int 0x7C694417  # mulhwu. %r3,%r9,%r8
   bl record
   li %r7,15
   bl check_alu_ov_undef
.endif

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - divw
   ########################################################################

   # divw
0: lis %r8,0x1000
   li %r9,12
   divw %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu
0: li %r9,-105
   li %r8,7
   divw %r3,%r9,%r8
   bl record
   li %r7,-15
   bl check_alu
0: li %r7,105
   li %r9,-7
   divw %r3,%r7,%r9
   bl record
   li %r7,-15
   bl check_alu
0: li %r7,3
   li %r8,-7
   divw %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu
0: li %r9,-105
   li %r7,7
   divw %r3,%r9,%r7
   bl record
   li %r7,-15
   bl check_alu
0: li %r9,0
   divw %r3,%r9,%r9
   # This gives no result; just check that it doesn't crash.

   # divw.
0: lis %r8,0x1000
   li %r9,12
   divw. %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu_undef
0: lis %r3,0x4000
   addis %r3,%r3,0x4000
   mtxer %r3
   li %r9,-105
   li %r8,7
   divw. %r3,%r9,%r8
   bl record
   li %r7,-15
   bl check_alu_so_undef
0: li %r7,3
   li %r8,-7
   divw. %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu_undef

   # divwo
0: lis %r8,0x1000
   li %r9,12
   divwo %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu
0: li %r9,-105
   li %r8,7
   divwo %r3,%r9,%r8
   bl record
   li %r7,-15
   bl check_alu
0: li %r7,3
   li %r8,-7
   divwo %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu
0: lis %r7,0x8000
   li %r9,-1
   divwo %r3,%r7,%r9
   bl record
   li %r3,0
   li %r7,0
   bl check_alu_ov
0: lis %r7,0x8000
   li %r11,1
   divwo %r3,%r7,%r11
   bl record
   bl check_alu
0: li %r8,1
   li %r7,0
   divwo %r3,%r8,%r7
   bl record
   li %r3,0
   bl check_alu_ov
0: li %r9,0
   divwo %r3,%r9,%r7
   bl record
   li %r3,0
   bl check_alu_ov

   # divwo.
0: lis %r8,0x1000
   li %r9,12
   divwo. %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu_undef
0: lis %r3,0x4000
   addis %r3,%r3,0x4000
   mtxer %r3
   li %r9,-105
   li %r8,7
   divwo. %r3,%r9,%r8
   bl record
   li %r7,-15
   bl check_alu_so_undef
0: li %r7,3
   li %r8,-7
   divwo. %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu_undef
0: lis %r7,0x8000
   li %r9,-1
   divwo. %r3,%r7,%r9
   bl record
   li %r3,0
   li %r7,0
   bl check_alu_ov_undef
0: lis %r7,0x8000
   li %r11,1
   divwo. %r3,%r7,%r11
   bl record
   bl check_alu_undef
0: li %r8,1
   li %r7,0
   divwo. %r3,%r8,%r7
   bl record
   li %r3,0
   bl check_alu_ov_undef
0: li %r9,0
   divwo. %r3,%r9,%r7
   bl record
   li %r3,0
   bl check_alu_ov_undef

   ########################################################################
   # 3.3.8 Fixed-Point Arithmetic Instructions - divwu
   ########################################################################

   # divwu
0: lis %r8,0x1000
   li %r9,12
   divwu %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu
0: li %r9,-105
   li %r8,7
   divwu %r3,%r9,%r8
   bl record
   lis %r7,0x2492
   addi %r7,%r7,0x4915
   bl check_alu
0: li %r7,105
   li %r9,-7
   divwu %r3,%r7,%r9
   bl record
   li %r7,0
   bl check_alu
0: li %r7,105
   li %r8,-7
   divwu %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu
0: li %r9,-105
   li %r7,7
   divwu %r3,%r9,%r7
   bl record
   lis %r7,0x2492
   addi %r7,%r7,0x4915
   bl check_alu

   # divwu.
0: lis %r8,0x1000
   li %r9,12
   divwu. %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu_undef
0: lis %r3,0x4000
   addis %r3,%r3,0x4000
   mtxer %r3
   li %r9,-105
   li %r8,1
   divwu. %r3,%r9,%r8
   bl record
   li %r7,-105
   bl check_alu_so_undef
0: li %r7,105
   li %r8,-7
   divwu. %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu_undef

   # divwuo
0: lis %r8,0x1000
   li %r9,12
   divwuo %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu
0: li %r9,-105
   li %r8,7
   divwuo %r3,%r9,%r8
   bl record
   lis %r7,0x2492
   addi %r7,%r7,0x4915
   bl check_alu
0: li %r7,105
   li %r8,-7
   divwuo %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu
0: lis %r7,0x8000
   li %r9,-1
   divwuo %r3,%r7,%r9
   bl record
   li %r7,0
   bl check_alu
0: li %r8,1
   divwuo %r3,%r8,%r7
   bl record
   li %r3,0
   bl check_alu_ov
0: li %r9,0
   divwuo %r3,%r9,%r7
   bl record
   li %r3,0
   bl check_alu_ov

   # divwuo.
0: lis %r8,0x1000
   li %r9,12
   divwuo. %r3,%r8,%r9
   bl record
   lis %r7,0x155
   addi %r7,%r7,0x5555
   bl check_alu_undef
0: lis %r3,0x4000
   addis %r3,%r3,0x4000
   mtxer %r3
   li %r9,-105
   li %r8,1
   divwuo. %r3,%r9,%r8
   bl record
   li %r7,-105
   bl check_alu_so_undef
0: li %r7,105
   li %r8,-7
   divwuo. %r3,%r7,%r8
   bl record
   li %r7,0
   bl check_alu_undef
0: lis %r7,0x8000
   li %r9,-1
   divwuo. %r3,%r7,%r9
   bl record
   li %r7,0
   bl check_alu_undef
0: li %r8,1
   divwuo. %r3,%r8,%r7
   bl record
   li %r3,0
   bl check_alu_ov_undef
0: li %r9,0
   divwuo. %r3,%r9,%r7
   bl record
   li %r3,0
   bl check_alu_ov_undef

   ########################################################################
   # 3.3.9 Fixed-Point Compare Instructions
   ########################################################################

   # These instructions don't produce any results, but we use the check_alu
   # functions anyway for brevity, so set up parameters to avoid spurious
   # failures.
0: li %r3,0
   li %r7,0

   # cmpwi
0: li %r11,3
   cmpwi %r11,4
   bl record
   bl check_alu_lt
0: lis %r8,0x6000
   addis %r8,%r8,0x6000
   mtxer %r8
   cmpwi %r11,3
   bl record
   bl check_alu_ov_eq
0: cmpwi %r11,2
   bl record
   bl check_alu_gt
0: cmpwi %r11,-1
   bl record
   bl check_alu_gt
0: li %r11,0
   cmpwi %r11,1
   bl record
   bl check_alu_lt
0: cmpwi %r11,0
   bl record
   bl check_alu_eq
0: cmpwi %r11,-1
   bl record
   bl check_alu_gt
0: li %r11,-3
   cmpwi %r11,0
   bl record
   bl check_alu_lt
0: cmpwi %r11,-2
   bl record
   bl check_alu_lt
0: cmpwi %r11,-3
   bl record
   bl check_alu_eq
0: cmpwi %r11,-4
   bl record
   bl check_alu_gt
0: li %r11,0
   cmpwi cr6,%r11,0
   bl record
   mfcr %r8
   cmpwi %r8,0x20
   beq 0f
   stw %r8,8(%r6)
   addi %r6,%r6,32
0: mtcr %r7
   li %r11,-3
   cmpwi %r11,-3
   bl record
   bl check_alu_eq

   # cmpw
0: li %r11,3
   li %r9,4
   cmpw %r11,%r9
   bl record
   bl check_alu_lt
0: lis %r8,0x6000
   addis %r8,%r8,0x6000
   mtxer %r8
   li %r9,3
   cmpw %r11,%r9
   bl record
   bl check_alu_ov_eq
0: li %r9,2
   cmpw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r9,-1
   cmpw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r11,0
   li %r9,1
   cmpw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r9,0
   cmpw %r11,%r9
   bl record
   bl check_alu_eq
0: li %r9,-1
   cmpw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r11,-3
   li %r9,0
   cmpw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r9,-2
   cmpw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r9,-3
   cmpw %r11,%r9
   bl record
   bl check_alu_eq
0: li %r9,-4
   cmpw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r11,0
   li %r9,0
   cmpw cr6,%r11,%r9
   bl record
   mfcr %r8
   cmpwi %r8,0x20
   beq 0f
   stw %r8,8(%r6)
   addi %r6,%r6,32
0: mtcr %r7
   li %r11,-3
   li %r9,-3
   cmpw %r11,%r9
   bl record
   bl check_alu_eq

   # cmplwi
0: li %r11,3
   cmplwi %r11,4
   bl record
   bl check_alu_lt
0: lis %r8,0x6000
   addis %r8,%r8,0x6000
   mtxer %r8
   cmplwi %r11,3
   bl record
   bl check_alu_ov_eq
0: cmplwi %r11,2
   bl record
   bl check_alu_gt
0: cmplwi %r11,65536-1
   bl record
   bl check_alu_lt
0: li %r11,0
   cmplwi %r11,1
   bl record
   bl check_alu_lt
0: cmplwi %r11,0
   bl record
   bl check_alu_eq
0: cmplwi %r11,65536-1
   bl record
   bl check_alu_lt
0: li %r11,-3
   cmplwi %r11,0
   bl record
   bl check_alu_gt
0: cmplwi %r11,65536-2
   bl record
   bl check_alu_gt
0: cmplwi %r11,65536-3
   bl record
   bl check_alu_gt
0: cmplwi %r11,65536-4
   bl record
   bl check_alu_gt
0: li %r11,0
   cmplwi cr6,%r11,0
   bl record
   mfcr %r8
   cmplwi %r8,0x20
   beq 0f
   stw %r8,8(%r6)
   addi %r6,%r6,32
0: mtcr %r7
   li %r11,-3
   cmplwi %r11,65536-3
   bl record
   bl check_alu_gt

   # cmplw
0: li %r11,3
   li %r9,4
   cmplw %r11,%r9
   bl record
   bl check_alu_lt
0: lis %r8,0x6000
   addis %r8,%r8,0x6000
   mtxer %r8
   li %r9,3
   cmplw %r11,%r9
   bl record
   bl check_alu_ov_eq
0: li %r9,2
   cmplw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r9,-1
   cmplw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r11,0
   li %r9,1
   cmplw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r9,0
   cmplw %r11,%r9
   bl record
   bl check_alu_eq
0: li %r9,-1
   cmplw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r11,-3
   li %r9,0
   cmplw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r9,-2
   cmplw %r11,%r9
   bl record
   bl check_alu_lt
0: li %r9,-3
   cmplw %r11,%r9
   bl record
   bl check_alu_eq
0: li %r9,-4
   cmplw %r11,%r9
   bl record
   bl check_alu_gt
0: li %r11,0
   li %r9,0
   cmplw cr6,%r11,%r9
   bl record
   mfcr %r8
   cmplwi %r8,0x20
   beq 0f
   stw %r8,8(%r6)
   addi %r6,%r6,32
0: mtcr %r7
   li %r11,-3
   li %r9,-3
   cmplw %r11,%r9
   bl record
   bl check_alu_eq

   ########################################################################
   # 3.3.10 Fixed-Point Trap Instructions
   ########################################################################

.if TEST_TRAP

   # twi
0: li %r7,10
   li %r3,-1
   twi 16,%r7,11
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 2,%r7,11
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 13,%r7,11
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 31,%r7,11
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 8,%r7,9
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 1,%r7,9
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 22,%r7,9
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 31,%r7,9
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 4,%r7,10
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 27,%r7,10
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 31,%r7,10
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 8,%r7,-10
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 2,%r7,-10
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 21,%r7,-10
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   twi 31,%r7,-10
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # tw
0: li %r7,10
   li %r24,11
   li %r3,-1
   tw 16,%r7,%r24
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 2,%r7,%r24
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 13,%r7,%r24
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 31,%r7,%r24
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r25,9
   li %r3,-1
   tw 8,%r7,%r25
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 1,%r7,%r25
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 22,%r7,%r25
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 31,%r7,%r25
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r26,10
   li %r3,-1
   tw 4,%r7,%r26
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 27,%r7,%r26
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 31,%r7,%r26
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r27,-10
   li %r3,-1
   tw 8,%r7,%r27
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 2,%r7,%r27
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 21,%r7,%r27
   bl record
   cmpwi %r3,-1
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,-1
   tw 31,%r7,%r27
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

.endif  # TEST_TRAP

   ########################################################################
   # 3.3.11 Fixed-Point Logical Instructions - second operand immediate
   ########################################################################

0: lis %r24,0x55AA
   addi %r24,%r24,0x55AA
   # We want to load 0xAA55AA55, but addi ...,0xAA55 will subtract from the
   # high halfword, so we use 0xAA56 (split into lis+addis as a holdover
   # from 64-bit architectures, where it's done to keep the upper 32 bits
   # clear).  Normally we'd use ori instead of addi, but we haven't yet
   # confirmed that ori functions properly.
   lis %r25,0x5A56
   addis %r25,%r25,0x5000
   addi %r25,%r25,0xAA55-0x10000

   # andi.
0: andi. %r3,%r24,0x1248
   bl record
   li %r7,0x1008
   bl check_alu_gt
0: andi. %r3,%r24,0xFFFF
   bl record
   li %r7,0x55AA
   bl check_alu_gt
0: andi. %r3,%r24,0
   bl record
   li %r7,0
   bl check_alu_eq

   # andis.
0: andis. %r3,%r24,0x1248
   bl record
   lis %r7,0x1008
   bl check_alu_gt
0: andis. %r3,%r25,0xFFFF
   bl record
   lis %r7,0x5A55
   addis %r7,%r7,0x5000
   bl check_alu_lt
0: andis. %r3,%r24,0
   bl record
   li %r7,0
   bl check_alu_eq

   # ori
0: ori %r3,%r24,0x1248
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x57EA
   bl check_alu

   # oris
0: oris %r3,%r24,0x1248
   bl record
   lis %r7,0x57EA
   addi %r7,%r7,0x55AA
   bl check_alu

   # xori
0: xori %r3,%r24,0x1248
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x47E2
   bl check_alu

   # xoris
0: xoris %r3,%r24,0x1248
   bl record
   lis %r7,0x47E2
   addi %r7,%r7,0x55AA
   bl check_alu

   ########################################################################
   # 3.3.11 Fixed-Point Logical Instructions - second operand in register
   ########################################################################

   # and
0: li %r3,0x1248
   and %r3,%r24,%r3
   bl record
   li %r7,0x1008
   bl check_alu

   # and.
0: li %r11,-6
   li %r3,3
   and. %r3,%r11,%r3
   bl record
   li %r7,2
   bl check_alu_gt
0: li %r8,5
   and. %r3,%r11,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-3
   and. %r3,%r11,%r9
   bl record
   li %r7,-8
   bl check_alu_lt

   # or
0: li %r3,0x1248
   or %r3,%r24,%r3
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x57EA
   bl check_alu

   # or.
0: li %r11,6
   li %r3,3
   or. %r3,%r11,%r3
   bl record
   li %r7,7
   bl check_alu_gt
0: li %r8,0
   or. %r3,%r8,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-6
   or. %r3,%r11,%r9
   bl record
   li %r7,-2
   bl check_alu_lt

   # xor
0: li %r3,0x1248
   xor %r3,%r24,%r3
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x47E2
   bl check_alu

   # xor.
0: li %r11,-6
   li %r3,3
   xor. %r3,%r11,%r3
   bl record
   li %r7,-7
   bl check_alu_lt
0: li %r8,-6
   xor. %r3,%r11,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-3
   xor. %r3,%r11,%r9
   bl record
   li %r7,7
   bl check_alu_gt

   # nand
0: li %r3,0x1248
   nand %r3,%r24,%r3
   bl record
   li %r7,~0x1008
   bl check_alu

   # nand.
0: li %r11,-6
   li %r3,3
   nand. %r3,%r11,%r3
   bl record
   li %r7,-3
   bl check_alu_lt
0: li %r8,-1
   nand. %r3,%r8,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-3
   nand. %r3,%r11,%r9
   bl record
   li %r7,7
   bl check_alu_gt

   # nor
0: li %r3,0x1248
   nor %r3,%r24,%r3
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x57EA
   nand %r7,%r7,%r7 # "not" is converted to "nor" so we can't use it yet.
   bl check_alu

   # nor.
0: li %r11,6
   li %r3,3
   nor. %r3,%r11,%r3
   bl record
   li %r7,-8
   bl check_alu_lt
0: li %r8,-1
   nor. %r3,%r11,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-6
   nor. %r3,%r11,%r9
   bl record
   li %r7,1
   bl check_alu_gt

   # eqv
0: li %r3,0x1248
   eqv %r3,%r24,%r3
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x47E2
   not %r7,%r7
   bl check_alu

   # eqv.
0: li %r11,-6
   li %r3,3
   eqv. %r3,%r11,%r3
   bl record
   li %r7,6
   bl check_alu_gt
0: li %r8,5
   eqv. %r3,%r11,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-3
   eqv. %r3,%r11,%r9
   bl record
   li %r7,-8
   bl check_alu_lt

   # andc
0: li %r3,0x1248
   andc %r3,%r3,%r25
   bl record
   li %r7,0x1008
   bl check_alu

   # andc.
0: li %r11,5
   li %r3,3
   andc. %r3,%r3,%r11
   bl record
   li %r7,2
   bl check_alu_gt
0: li %r8,5
   andc. %r3,%r8,%r11
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-3
   andc. %r3,%r9,%r11
   bl record
   li %r7,-8
   bl check_alu_lt

   # orc
0: li %r3,0x1248
   orc %r3,%r3,%r25
   bl record
   lis %r7,0x55AA
   addi %r7,%r7,0x57EA
   bl check_alu

   # orc.
0: li %r11,-7
   li %r3,3
   orc. %r3,%r3,%r11
   bl record
   li %r7,7
   bl check_alu_gt
0: li %r8,-1
   li %r9,0
   orc. %r3,%r9,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,-6
   orc. %r3,%r9,%r11
   bl record
   li %r7,-2
   bl check_alu_lt

   ########################################################################
   # 3.3.11 Fixed-Point Logical Instructions - extsb, extsh
   ########################################################################

   # extsb
0: li %r3,0x17F
   extsb %r3,%r3
   bl record
   li %r7,0x7F
   bl check_alu
0: li %r3,0x180
   extsb %r3,%r3
   bl record
   li %r7,-0x80
   bl check_alu

   # extsb.
0: li %r3,0x17F
   extsb. %r3,%r3
   bl record
   li %r7,0x7F
   bl check_alu_gt
0: li %r3,0x180
   extsb. %r3,%r3
   bl record
   li %r7,-0x80
   bl check_alu_lt
0: li %r3,0x100
   extsb. %r3,%r3
   bl record
   li %r7,0
   bl check_alu_eq

   # extsh
0: lis %r3,0x1
   ori %r3,%r3,0x7F00
   extsh %r3,%r3
   bl record
   li %r7,0x7F00
   bl check_alu
0: lis %r3,0x1
   ori %r3,%r3,0x8000
   extsh %r3,%r3
   bl record
   li %r7,-0x8000
   bl check_alu

   # extsh.
0: lis %r3,0x1
   ori %r3,%r3,0x7F00
   extsh. %r3,%r3
   bl record
   li %r7,0x7F00
   bl check_alu_gt
0: lis %r3,0x1
   ori %r3,%r3,0x8000
   extsh. %r3,%r3
   bl record
   li %r7,-0x8000
   bl check_alu_lt
0: lis %r3,0x1
   extsh. %r3,%r3
   bl record
   li %r7,0
   bl check_alu_eq

   ########################################################################
   # 3.3.11 Fixed-Point Logical Instructions - cntlzw
   ########################################################################

   # cntlzw
0: li %r3,1
   cntlzw %r3,%r3
   bl record
   li %r7,31
   bl check_alu
0: li %r8,-1
   cntlzw %r3,%r8
   bl record
   li %r7,0
   bl check_alu
0: li %r9,0
   cntlzw %r3,%r9
   bl record
   li %r7,32
   bl check_alu

   # cntlzw.
0: li %r3,1
   cntlzw. %r3,%r3
   bl record
   li %r7,31
   bl check_alu_gt
0: li %r8,-1
   cntlzw. %r3,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r9,0
   cntlzw. %r3,%r9
   bl record
   li %r7,32
   bl check_alu_gt

   ########################################################################
   # 3.3.12.1 Fixed-Point Rotate and Shift Instructions - rlwinm, rlwnm
   ########################################################################

0: lis %r24,0x89AB
   ori %r24,%r24,0xCDEF

   # rlwinm
0: rlwinm %r3,%r24,5,0,31   # rotlwi %r3,%r24,5
   bl record
   lis %r7,0x3579
   ori %r7,%r7,0xBDF1
   bl check_alu
0: rlwinm %r3,%r24,30,31,1
   bl record
   lis %r7,0xC000
   ori %r7,%r7,0x0001
   bl check_alu

   # rlwinm.
0: rlwinm. %r3,%r24,5,0,31  # rotlwi. %r3,%r24,5
   bl record
   lis %r7,0x3579
   ori %r7,%r7,0xBDF1
   bl check_alu_gt
0: rlwinm. %r3,%r24,30,31,1
   bl record
   lis %r7,0xC000
   ori %r7,%r7,0x0001
   bl check_alu_lt
0: rlwinm. %r3,%r24,5,28,30
   bl record
   li %r7,0
   bl check_alu_eq

   # rlwnm
0: li %r0,5
   rlwnm %r3,%r24,%r0,0,31  # rotlw %r3,%r24,%r0
   bl record
   lis %r7,0x3579
   ori %r7,%r7,0xBDF1
   bl check_alu
0: li %r0,30
   rlwnm %r3,%r24,%r0,31,1
   bl record
   lis %r7,0xC000
   ori %r7,%r7,0x0001
   bl check_alu

   # rlwnm.
0: li %r0,5
   rlwnm. %r3,%r24,%r0,0,31 # rotlw. %r3,%r24,%r0
   bl record
   lis %r7,0x3579
   ori %r7,%r7,0xBDF1
   bl check_alu_gt
0: li %r0,30
   rlwnm. %r3,%r24,%r0,31,1
   bl record
   lis %r7,0xC000
   ori %r7,%r7,0x0001
   bl check_alu_lt
0: li %r0,5
   rlwnm. %r3,%r24,%r0,28,30
   bl record
   li %r7,0
   bl check_alu_eq

   ########################################################################
   # 3.3.12.1 Fixed-Point Rotate and Shift Instructions - rlwimi
   ########################################################################

   # rlwimi
0: li %r3,-1
   rlwimi %r3,%r24,5,0,31
   bl record
   lis %r7,0x3579
   ori %r7,%r7,0xBDF1
   bl check_alu
0: lis %r3,0x1000
   ori %r3,%r3,0x0004
   rlwimi %r3,%r24,30,31,1
   bl record
   lis %r7,0xD000
   ori %r7,%r7,0x0005
   bl check_alu

   # rlwimi.
0: lis %r3,0x4002
   rlwimi. %r3,%r24,5,0,31
   bl record
   lis %r7,0x3579
   ori %r7,%r7,0xBDF1
   bl check_alu_gt
0: lis %r3,0x1000
   ori %r3,%r3,0x0004
   rlwimi. %r3,%r24,30,31,1
   bl record
   lis %r7,0xD000
   ori %r7,%r7,0x0005
   bl check_alu_lt
0: li %r3,14
   rlwimi. %r3,%r24,5,28,30
   bl record
   li %r7,0
   bl check_alu_eq

   ########################################################################
   # 3.3.12.2 Fixed-Point Rotate and Shift Instructions - slw
   ########################################################################

   # slw
0: li %r3,1
   li %r0,0
   slw %r3,%r3,%r0
   bl record
   li %r7,1
   bl check_alu
0: li %r3,3
   li %r7,31
   slw %r3,%r3,%r7
   bl record
   lis %r7,0x8000
   bl check_alu
0: li %r3,1
   li %r8,32
   slw %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu
0: li %r3,1
   li %r9,63
   slw %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu
0: li %r3,1
   li %r11,64
   slw %r3,%r3,%r11
   bl record
   li %r7,1
   bl check_alu

   # slw.
0: li %r3,1
   li %r0,0
   slw. %r3,%r3,%r0
   bl record
   li %r7,1
   bl check_alu_gt
0: li %r3,3
   li %r7,31
   slw. %r3,%r3,%r7
   bl record
   lis %r7,0x8000
   bl check_alu_lt
0: li %r3,1
   li %r8,32
   slw. %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r3,1
   li %r9,63
   slw. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: li %r3,1
   li %r11,64
   slw. %r3,%r3,%r11
   bl record
   li %r7,1
   bl check_alu_gt

   ########################################################################
   # 3.3.12.2 Fixed-Point Rotate and Shift Instructions - srw
   ########################################################################

   # srw
0: lis %r3,0x8000
   li %r0,0
   srw %r3,%r3,%r0
   bl record
   lis %r7,0x8000
   bl check_alu
0: lis %r3,0x8000
   li %r7,31
   srw %r3,%r3,%r7
   bl record
   li %r7,1
   bl check_alu
0: lis %r3,0x8000
   li %r8,32
   srw %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu
0: lis %r3,0x8000
   li %r9,63
   srw %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu
0: lis %r3,0x8000
   li %r11,64
   srw %r3,%r3,%r11
   bl record
   lis %r7,0x8000
   bl check_alu

   # srw.
0: lis %r3,0x8000
   li %r0,0
   srw. %r3,%r3,%r0
   bl record
   lis %r7,0x8000
   bl check_alu_lt
0: lis %r3,0x8000
   li %r7,31
   srw. %r3,%r3,%r7
   bl record
   li %r7,1
   bl check_alu_gt
0: lis %r3,0x8000
   li %r8,32
   srw. %r3,%r3,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r3,0x8000
   li %r9,63
   srw. %r3,%r3,%r9
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r3,0x8000
   li %r11,64
   srw. %r3,%r3,%r11
   bl record
   lis %r7,0x8000
   bl check_alu_lt

   ########################################################################
   # 3.3.12.2 Fixed-Point Rotate and Shift Instructions - srawi
   ########################################################################

   # srawi
0: li %r11,10
   srawi %r3,%r11,0
   bl record
   li %r7,10
   bl check_alu
0: li %r11,10
   srawi %r3,%r11,2
   bl record
   li %r7,2
   bl check_alu
0: li %r11,10
   srawi %r3,%r11,31
   bl record
   li %r7,0
   bl check_alu
0: lis %r3,0xA000
   srawi %r3,%r3,0
   bl record
   lis %r7,0xA000
   bl check_alu
0: lis %r3,0xA000
   srawi %r3,%r3,28
   bl record
   li %r7,-6
   bl check_alu
0: lis %r3,0xA000
   srawi %r3,%r3,30
   bl record
   li %r7,-2
   bl check_alu_ca
0: lis %r3,0xA000
   srawi %r3,%r3,31
   bl record
   li %r7,-1
   bl check_alu_ca

   # srawi.
0: li %r11,10
   srawi. %r3,%r11,0
   bl record
   li %r7,10
   bl check_alu_gt
0: li %r11,10
   srawi. %r3,%r11,2
   bl record
   li %r7,2
   bl check_alu_gt
0: li %r11,10
   srawi. %r3,%r11,31
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r3,0xA000
   srawi. %r3,%r3,0
   bl record
   lis %r7,0xA000
   bl check_alu_lt
0: lis %r3,0xA000
   srawi. %r3,%r3,28
   bl record
   li %r7,-6
   bl check_alu_lt
0: lis %r3,0xA000
   srawi. %r3,%r3,30
   bl record
   li %r7,-2
   bl check_alu_ca_lt
0: lis %r3,0xA000
   srawi. %r3,%r3,31
   bl record
   li %r7,-1
   bl check_alu_ca_lt

   ########################################################################
   # 3.3.12.2 Fixed-Point Rotate and Shift Instructions - sraw
   ########################################################################

   # sraw
0: li %r11,10
   li %r0,0
   sraw %r3,%r11,%r0
   bl record
   li %r7,10
   bl check_alu
0: li %r11,10
   li %r7,2
   sraw %r3,%r11,%r7
   bl record
   li %r7,2
   bl check_alu
0: li %r11,10
   li %r8,32
   sraw %r3,%r11,%r8
   bl record
   li %r7,0
   bl check_alu
0: lis %r3,0xA000
   li %r0,0
   sraw %r3,%r3,%r0
   bl record
   lis %r7,0xA000
   bl check_alu
0: lis %r3,0xA000
   li %r7,28
   sraw %r3,%r3,%r7
   bl record
   li %r7,-6
   bl check_alu
0: lis %r7,0xA000
   li %r8,30
   sraw %r3,%r7,%r8
   bl record
   li %r7,-2
   bl check_alu_ca
0: lis %r3,0xA000
   li %r8,32
   sraw %r3,%r3,%r8
   bl record
   li %r7,-1
   bl check_alu_ca
0: lis %r3,0xA000
   li %r9,63
   sraw %r3,%r3,%r9
   bl record
   li %r7,-1
   bl check_alu_ca
0: lis %r3,0xA000
   li %r11,64
   sraw %r3,%r3,%r11
   bl record
   lis %r7,0xA000
   bl check_alu

   # sraw.
0: li %r11,10
   li %r0,0
   sraw. %r3,%r11,%r0
   bl record
   li %r7,10
   bl check_alu_gt
0: li %r11,10
   li %r7,2
   sraw. %r3,%r11,%r7
   bl record
   li %r7,2
   bl check_alu_gt
0: li %r11,10
   li %r8,32
   sraw. %r3,%r11,%r8
   bl record
   li %r7,0
   bl check_alu_eq
0: lis %r3,0xA000
   li %r0,0
   sraw. %r3,%r3,%r0
   bl record
   lis %r7,0xA000
   bl check_alu_lt
0: lis %r3,0xA000
   li %r7,28
   sraw. %r3,%r3,%r7
   bl record
   li %r7,-6
   bl check_alu_lt
0: lis %r7,0xA000
   li %r8,30
   sraw. %r3,%r7,%r8
   bl record
   li %r7,-2
   bl check_alu_ca_lt
0: lis %r3,0xA000
   li %r8,32
   sraw. %r3,%r3,%r8
   bl record
   li %r7,-1
   bl check_alu_ca_lt
0: lis %r3,0xA000
   li %r9,63
   sraw. %r3,%r3,%r9
   bl record
   li %r7,-1
   bl check_alu_ca_lt
0: lis %r3,0xA000
   li %r11,64
   sraw. %r3,%r3,%r11
   bl record
   lis %r7,0xA000
   bl check_alu_lt

   ########################################################################
   # 3.3.13 Move To/From System Register Instructions
   ########################################################################

   # These instructions have all been tested at earlier points.

   ########################################################################
   # 4.6 Floating-Point Processor Instructions
   ########################################################################

   # Set up %r30 and %r31 with pointers to single- resp. double-precision
   # floating-point constant tables, since we can't use immediate values in
   # floating-point instructions or move directly between general-purpose
   # registers and floating-point registers.

0: bl 1f
1: mflr %r3
   addi %r30,%r3,2f-1b
   addi %r31,%r3,3f-1b
   b 0f
2: .int 0x00000000              #   0(%r30): 0.0f
   .int 0x3F800000              #   4(%r30): 1.0f
   .int 0x3FB504F3              #   8(%r30): sqrtf(2.0f)
   .int 0x00800000              #  12(%r30): minimum single-precision normal
   .int 0x3FD55555              #  16(%r30): 1.666...f
   .int 0x7FD00000              #  20(%r30): QNaN
   .int 0xFFA00000              #  24(%r30): SNaN
   .int 0x3FAAAAAB              #  28(%r30): 1.333...f (rounded toward nearest)
   .int 0x3FC00000              #  32(%r30): 1.5f
   .int 0x40000001              #  36(%r30): 2.0f + 1ulp
   .int 0xB3800000              #  40(%r30): 2.0f - (1.333...f * 1.5f)
   .int 0x00000001              #  44(%r30): minimum single-precision denormal
   .int 0x80000001              #  48(%r30): same, with sign bit set
   .int 0x00FFFFFF              #  52(%r30): minimum single normal * 2 - 1ulp
   .int 0x001FFE00              #  56(%r30): 1/HUGE_VALF*4095/4096 (fres bound)
   .int 0x00200200              #  60(%r30): 1/HUGE_VALF*4097/4096 (fres bound)
   .balign 8
3: .int 0x00000000,0x00000000   #   0(%r31): 0.0
   .int 0x3FF00000,0x00000000   #   8(%r31): 1.0
   .int 0x7FF00000,0x00000000   #  16(%r31): inf
   .int 0xFFF00000,0x00000000   #  24(%r31): -inf
   .int 0x7FFA0000,0x00000000   #  32(%r31): QNaN
   .int 0xFFF40000,0x00000000   #  40(%r31): SNaN
   .int 0xFFFC0000,0x00000000   #  48(%r31): SNaN converted to QNaN
   .int 0x3FD00000,0x00000000   #  56(%r31): 0.25
   .int 0x3FE00000,0x00000000   #  64(%r31): 0.5
   .int 0x40000000,0x00000000   #  72(%r31): 2.0
   .int 0x41500000,0x00000000   #  80(%r31): 4194304.0
   .int 0x41500000,0x08000000   #  88(%r31): 4194304.125
   .int 0x41500000,0x10000000   #  96(%r31): 4194304.25
   .int 0x41500000,0x18000000   # 104(%r31): 4194304.375
   .int 0x41500000,0x20000000   # 112(%r31): 4194304.5
   .int 0x41500000,0x30000000   # 120(%r31): 4194304.75
   .int 0x41500000,0x40000000   # 128(%r31): 4194305.0
   .int 0x36A00000,0x00000000   # 136(%r31): minimum single-precision denormal
   .int 0x36800000,0x00000000   # 144(%r31): minimum single-prec. denormal / 4
   .int 0x47EFFFFF,0xE0000000   # 152(%r31): HUGE_VALF (maximum normal float)
   .int 0x47EFFFFF,0xE8000000   # 160(%r31): HUGE_VALF + epsilon/4
   .int 0x7FF80000,0x00000000   # 168(%r31): QNaN from invalid operations
   .int 0x40A00000,0x00000000   # 176(%r31): 2048.0
   .int 0x41400000,0x00000000   # 184(%r31): 2097152.0
   .int 0x41400000,0x08000000   # 192(%r31): 2097152.0625
   .int 0x414FFFFF,0xE0000000   # 200(%r31): 4194303.75 (exact in single-prec)
   .int 0x41E00000,0x00000000   # 208(%r31): (double) 2^31
   .int 0xC1E00000,0x00200000   # 216(%r31): (double) -(2^31+1)
   .int 0x43E00000,0x00000000   # 224(%r31): (double) 2^63
   .int 0xC3E00000,0x00000001   # 232(%r31): (double) -(2^63+2^11)
   .int 0x00100000,0x00000000   # 240(%r31): minimum double-precision normal
   .int 0x00080000,0x00000000   # 248(%r31): minimum double-prec. normal / 2
   .int 0x3FDFFE00,0x00000000   # 256(%r31): 0.5 * 4095/4096 (fres bound)
   .int 0x3FE00100,0x00000000   # 264(%r31): 0.5 * 4097/4096 (fres bound)
   .int 0x3FFFFE00,0x00000000   # 272(%r31): 2.0 * 4095/4096 (frsqrte bound)
   .int 0x40000100,0x00000000   # 280(%r31): 2.0 * 4097/4096 (frsqrte bound)
   .int 0x3FF55555,0x55555555   # 288(%r31): 1.333...
   .int 0x3FF80000,0x00000000   # 296(%r31): 1.5
   .int 0x3FFFFFFF,0xFFFFFFFF   # 304(%r31): 2.0 - 1ulp
   .int 0x3CA00000,0x00000000   # 312(%r31): 2.0 - (1.333... * 1.5)
   .int 0x7FEFFFFF,0xFFFFFFFF   # 320(%r31): HUGE_VAL (maximum normal double)
   .int 0x47E00000,0x00000000   # 328(%r31): (double)2^127
   .int 0x47F00000,0x00000000   # 336(%r31): (double)2^128 (out of float range)
   .int 0x46700000,0x00000000   # 344(%r31): (double)2^128 - HUGE_VALF
   .int 0x3CB00000,0x00000000   # 352(%r31): 2.0 - (2.0 - 1ulp)
   .int 0x3FFAAAAA,0xAAAAAAAB   # 360(%r31): 1.666...
   .int 0x3FF00000,0x00000001   # 368(%r31): 1.0 + 1ulp
   .int 0x00000000,0x00000001   # 376(%r31): minimum double-precision denormal
   .int 0x3FEFFFFF,0xF8000000   # 384(%r31): (double)(1.0f - 0.25ulp)
   .int 0x001FFFFF,0xFFFFFFFF   # 392(%r31): minimum double normal * 2 - 1ulp

   ########################################################################
   # 4.6.2 Floating-Point Load Instructions
   ########################################################################

   # lfs
0: lfs %f0,4(%r30)
   bl record
   fcmpu cr0,%f0,%f1
   beq 0f
   stfd %f0,8(%r6)
   addi %r6,%r6,32
   # Verify that we can load a different value, in case lfs is broken but
   # F0 happened to contain 1.0 on entry.
0: lfs %f0,0(%r30)
   bl record
   fcmpu cr0,%f0,%f1
   bne 0f
   stfd %f0,8(%r6)
   addi %r6,%r6,32

   # lfsx
0: addi %r7,%r30,8
   li %r8,-4
   lfs %f0,0(%r30)
   lfsx %f0,%r7,%r8
   bl record
   fcmpu cr0,%f0,%f1
   bne 1f
   addi %r8,%r30,8
   cmpw %r7,%r8
   beq 0f
1: stfd %f0,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: addi %r7,%r30,4
   li %r0,4
   lfs %f0,0(%r30)
   lfsx %f0,0,%r7
   bl record
   fcmpu cr0,%f0,%f1
   beq 0f
   stfd %f0,8(%r6)
   addi %r6,%r6,32

   # lfsu
0: addi %r7,%r30,8
   lfsu %f0,-4(%r7)
   bl record
   fcmpu cr0,%f0,%f1
   bne 1f
   addi %r0,%r30,4
   cmpw %r7,%r0
   beq 0f
1: stfd %f0,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lfsux
0: addi %r7,%r30,8
   li %r0,-4
   lfs %f0,0(%r30)
   lfsux %f0,%r7,%r0
   bl record
   fcmpu cr0,%f0,%f1
   bne 1f
   addi %r0,%r30,4
   cmpw %r7,%r0
   beq 0f
1: stfd %f0,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lfd
0: lfd %f0,0(%r31)
   bl record
   fcmpu cr0,%f0,%f1
   bne 0f
   stfd %f0,8(%r6)
   addi %r6,%r6,32
0: lfd %f0,8(%r31)
   bl record
   fcmpu cr0,%f0,%f1
   beq 0f
   stfd %f0,8(%r6)
   addi %r6,%r6,32

   # lfdx
0: addi %r7,%r31,16
   li %r8,-8
   lfd %f0,0(%r31)
   lfdx %f0,%r7,%r8
   bl record
   fcmpu cr0,%f0,%f1
   bne 1f
   addi %r8,%r31,16
   cmpw %r7,%r8
   beq 0f
1: stfd %f0,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: addi %r7,%r31,8
   li %r0,8
   lfd %f0,0(%r31)
   lfdx %f0,0,%r7
   bl record
   fcmpu cr0,%f0,%f1
   beq 0f
   stfd %f0,8(%r6)
   addi %r6,%r6,32

   # lfdu
0: addi %r7,%r31,16
   lfd %f0,0(%r31)
   lfdu %f0,-8(%r7)
   bl record
   fcmpu cr0,%f0,%f1
   bne 1f
   addi %r0,%r31,8
   cmpw %r7,%r0
   beq 0f
1: stfd %f0,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # lfdux
0: addi %r7,%r31,16
   li %r0,-8
   lfd %f0,0(%r31)
   lfdux %f0,%r7,%r0
   bl record
   fcmpu cr0,%f0,%f1
   bne 1f
   addi %r0,%r31,8
   cmpw %r7,%r0
   beq 0f
1: stfd %f0,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 4.6.3 Floating-Point Store Instructions
   ########################################################################

0: li %r0,0
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stw %r0,8(%r4)
   stw %r0,12(%r4)

   # stfs
0: addi %r7,%r4,4
   stw %r0,0(%r4)
   stfs %f1,-4(%r7)
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3F80
   cmpw %r3,%r8
   bne 1f
   addi %r8,%r4,4
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stfsx
0: li %r8,-4
   stw %r0,0(%r4)
   stfsx %f1,%r7,%r8
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3F80
   cmpw %r3,%r8
   bne 1f
   addi %r8,%r4,4
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r0,4
   stw %r0,0(%r4)
   stfsx %f1,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3F80
   cmpw %r3,%r8
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # stfsu
0: stw %r0,0(%r4)
   stfsu %f1,-4(%r7)
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3F80
   cmpw %r3,%r8
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stfsux
0: addi %r7,%r4,4
   li %r8,-4
   stw %r0,0(%r4)
   stfsux %f1,%r7,%r8
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3F80
   cmpw %r3,%r8
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stfd
0: addi %r7,%r4,8
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f1,-8(%r7)
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3FF0
   cmpw %r3,%r8
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   addi %r8,%r4,8
   cmpw %r7,%r8
   beq 0f
1: stw %r7,16(%r6)
   addi %r6,%r6,32

   # stfdx
0: li %r8,-8
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfdx %f1,%r7,%r8
   bl record
   lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lis %r8,0x3FF0
   cmpw %r3,%r8
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   addi %r8,%r4,8
   cmpw %r7,%r8
   beq 0f
1: stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r0,8
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfdx %f1,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3FF0
   cmpw %r3,%r8
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   beq 0f
1: addi %r6,%r6,32

   # stfdu
0: stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfdu %f1,-8(%r7)
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3FF0
   cmpw %r3,%r8
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stfdux
0: addi %r7,%r4,8
   li %r8,-8
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfdux %f1,%r7,%r8
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x3FF0
   cmpw %r3,%r8
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # stfiwx
0: lfd %f0,104(%r31)  # 4194304.375
   addi %r7,%r4,4
   li %r8,-4
   stw %r0,0(%r4)
   stfiwx %f0,%r7,%r8
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x1800
   cmpw %r3,%r8
   bne 1f
   addi %r8,%r4,4
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: li %r0,4
   stw %r0,0(%r4)
   stfiwx %f0,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r8,0x1800
   cmpw %r3,%r8
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # Loads and stores should not change the sign of zero.
0: lis %r7,0x8000
   stw %r7,0(%r4)
   li %r0,0
   stw %r0,4(%r4)
   lfs %f7,0(%r4)
   stw %r0,8(%r4)
   stfs %f7,8(%r4)
   bl record
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: lfd %f8,0(%r4)
   stw %r0,8(%r4)
   stw %r7,12(%r4)
   stfd %f8,8(%r4)
   bl record
   lwz %r3,8(%r4)
   lwz %r8,12(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpwi %r8,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 4.6.8 Floating-Point Status and Control Register Instructions
   ########################################################################

   # mffs/mtfsf
0: lfd %f0,0(%r31)  # 0.0
   mtfsf 255,%f0
   lfd %f0,88(%r31)  # 4194304.125 (arbitary value with nonzero low word)
   mffs %f0
   bl record
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r0,~0x0800  # Don't try to set bit 20 (reserved).
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf 255,%f0
   bl record
   lfd %f0,88(%r31)
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   cmpwi %r3,~0x0800
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r0,0
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf 8,%f0
   bl record
   lfd %f0,88(%r31)
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0xFFFF
   ori %r0,%r0,0x07FF
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
   # mtfsf should not alter bits 1-2 of FPSCR.
0: lis %r0,0x9000
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf 128,%f0
   bl record
   lfd %f0,88(%r31)
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0xFFFF
   ori %r0,%r0,0x07FF
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: lis %r0,0x6000
   stw %r0,4(%r4)
   lfd %f2,0(%r4)
   mtfsf 255,%f2
   bl record
   lfd %f0,88(%r31)
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mffs.
0: li %r0,0
   mtcr %r0
   lis %r0,0x1234
   ori %r0,%r0,0x5678
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf 255,%f0
   mffs. %f0
   bl record
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r8
   lis %r9,0x0700   # FPSCR[1:2] were automatically set by mtfsf.
   cmpw %r8,%r9
   bne 1f
   lis %r0,0x7234
   ori %r0,%r0,0x5678
   cmpw %r3,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32

   # mcrfs
0: li %r0,0
   mtcr %r0
   lis %r0,0x123C
   ori %r0,%r0,0x5678
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf 255,%f0
   mcrfs cr7,0
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r8
   cmpwi %r8,7
   bne 1f
   lis %r0,0x623C
   ori %r0,%r0,0x5678
   cmpw %r3,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   mcrfs cr7,3
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r8
   cmpwi %r8,0xC
   bne 1f
   lis %r0,0x6234   # Non-exception bits are not cleared.
   ori %r0,%r0,0x5678
   cmpw %r3,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   mcrfs cr5,1
   mcrfs cr6,2
   mcrfs cr7,5
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r8
   cmpwi %r8,0x236
   bne 1f
   lis %r0,0x0004   # Clearing exception bits should clear FEX and VX too.
   ori %r0,%r0,0x5078
   cmpw %r3,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32

   # mtfsfi/mtfsfi.
0: li %r0,0
   mtcr %r0
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf 255,%f0
   mtfsfi. 0,1
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r7
   lis %r0,0x1000   # FPSCR[0] should not have been set.
   cmpw %r3,%r0
   bne 1f
   lis %r0,0x0100
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsfi 2,8
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0x3080   # FPSCR[2] should have been set along with FPSCR[8].
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtfsf.
0: li %r0,0
   mtcr %r0
   lis %r0,0x1234
   ori %r0,%r0,0x5678
   stw %r0,4(%r4)
   lfd %f0,0(%r4)
   mtfsf. 255,%f0
   bl record
   mfcr %r0
   lis %r3,0x0700
   cmpw %r0,%r3
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtfsb0
0: mtfsb0 28
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0x7234
   ori %r0,%r0,0x5670
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: mtfsb0 1         # Should have no effect.
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0x7234
   ori %r0,%r0,0x5670
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtfsb0.
0: li %r0,0
   mtcr %r0
   mtfsb0. 3        # Also clears bit 1.
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r7
   lis %r0,0x2234
   ori %r0,%r0,0x5670
   cmpw %r3,%r0
   bne 1f
   lis %r0,0x0200
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # mtfsb1
0: mtfsb1 16
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0x2234
   ori %r0,%r0,0xD670
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: mtfsb1 1         # Should have no effect.
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   lis %r0,0x2234
   ori %r0,%r0,0xD670
   cmpw %r3,%r0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtfsb1.
0: li %r0,0
   mtcr %r0
   mtfsb1. 3        # Also sets bits 0 and 1.
   bl record
   mffs %f0
   stfd %f0,0(%r4)
   lwz %r3,4(%r4)
   mfcr %r7
   lis %r0,0xF234
   ori %r0,%r0,0xD670
   cmpw %r3,%r0
   bne 1f
   lis %r0,0x0F00
   cmpw %r7,%r0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 4.6.4 Floating-Point Move Instructions
   ########################################################################

0: lfd %f0,0(%r31)  # 0.0
   lfd %f5,32(%r31) # QNaN
   lfd %f6,40(%r31) # SNaN
   mtfsf 255,%f0
   mtfsfi 0,8       # Ensure that Rc=1 insns have a nonzero value to write.
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r11,4(%r4)
   li %r0,0

   # fmr
0: mtcr %r0
   fmr %f3,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3         # Also check that FPSCR is unmodified.
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0      # Check that CR1 is unmodified (Rc=0).
   beq 0f
1: stw %r8,16(%r6)
   addi %r6,%r6,32
   # Check that NaNs don't cause exceptions with these instructions.
0: fmr %f3,%f5
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FFA
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fmr %f3,%f6
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFF4
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fmr.
0: mtcr %r0
   fmr. %f3,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fneg
0: mtcr %r0
   fneg %f2,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f2,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fneg %f3,%f5
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFFA
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fneg %f3,%f6
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FF4
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
   # fneg should negate the sign of zero as well.
0: fneg %f3,%f0
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x8000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fneg %f3,%f0
   fneg %f3,%f3
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fneg.
0: mtcr %r0
   fneg. %f2,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f2,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fabs
0: mtcr %r0
   fmr %f3,%f0
   fabs %f3,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fmr %f3,%f0
   fabs %f3,%f2
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fabs %f3,%f5
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FFA
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fabs %f3,%f6
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FF4
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fabs.
0: mtcr %r0
   fmr %f3,%f0
   fabs. %f3,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fnabs
0: mtcr %r0
   fmr %f3,%f0
   fnabs %f3,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fmr %f3,%f0
   fnabs %f3,%f2
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fnabs %f3,%f5
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFFA
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: fnabs %f3,%f6
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFF4
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # fnabs.
0: mtcr %r0
   fmr %f3,%f0
   fnabs. %f3,%f1
   bl record
   stw %r0,0(%r4)
   stw %r0,4(%r4)
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 4.6.7 Floating-Point Compare Instructions
   ########################################################################

0: li %r0,0
   mtcr %r0

   # fcmpu
0: mtfsf 255,%f0
   fcmpu cr0,%f0,%f0    # 0.0 == 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: fcmpu cr1,%f1,%f0    # 1.0 > 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2400
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   fcmpu cr7,%f2,%f0    # -1.0 < 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   fcmpu cr7,%f5,%f0    # QNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,0x1000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Clear FPSCR here to verify that FPSCR[19] is set again and not just
   # carried over from the last test.
0: mtfsf 255,%f0
   mtcr %r0
   fcmpu cr7,%f0,%f5    # 0.0 ? QNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,0x1000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   fcmpu cr7,%f6,%f0    # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Clear FPSCR to verify that FPSCR[19] is set and also to ensure that
   # the exception flag is newly raised.
0: mtfsf 255,%f0
   mtcr %r0
   fcmpu cr7,%f0,%f6    # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Test again with FPSCR[FX] cleared to ensure that it's not set again
   # when the exception flag does not change.
0: mtfsb0 0
   mtcr %r0
   fcmpu cr5,%f0,%f6    # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x100
   bne 1f
   lis %r8,0x2100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Check that a set exception flag is not cleared if no exception occurs.
0: mtcr %r0
   fcmpu cr5,%f0,%f0    # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x200
   bne 1f
   lis %r8,0x2100
   ori %r8,%r8,0x2000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # fcmpo (VE=0)
0: mtcr %r0
   mtfsf 255,%f0
   fcmpo cr0,%f0,%f0    # 0.0 == 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: fcmpo cr1,%f1,%f0    # 1.0 > 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2400
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   fcmpo cr7,%f2,%f0    # -1.0 < 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   fcmpo cr7,%f5,%f0    # QNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA008
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   fcmpo cr7,%f0,%f5    # 0.0 ? QNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA008
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   fcmpo cr7,%f6,%f0    # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   fcmpo cr7,%f0,%f6    # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   fcmpo cr5,%f0,%f6    # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x100
   bne 1f
   lis %r8,0x2108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   fcmpo cr5,%f0,%f0    # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x200
   bne 1f
   lis %r8,0x2108
   ori %r8,%r8,0x2000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # fcmpo (VE=1)
0: mtfsf 255,%f0
   mtfsb1 24
   mtcr %r0
   fcmpo cr6,%f6,%f0    # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x10
   bne 1f
   lis %r8,0xE100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtfsb1 24
   mtcr %r0
   fcmpo cr6,%f0,%f6    # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x10
   bne 1f
   lis %r8,0xE100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   fcmpo cr4,%f0,%f6    # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x1000
   bne 1f
   lis %r8,0x6100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   fcmpo cr4,%f0,%f0    # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x2000
   bne 1f
   lis %r8,0x6100
   ori %r8,%r8,0x2080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 4.6.3 Floating-Point Store Instructions - float/double conversion
   ########################################################################

   # Check for two possible bugs in software implementations:
   # - Conversion between single and double precision causes an exception
   #   which terminates the program.
   # - Conversion leaves a stale exception flag which is picked up by a
   #   subsequent floating-point operation.
   # We assume here that frsp(0.0) works and use it as a method of
   # catching stale exceptions; frsp is fully tested below.

0: li %r0,0
   mtcr %r0
   mtfsf 255,%f0

   # lfs (denormal)
0: li %r3,1
   stw %r3,0(%r4)
   lfs %f3,0(%r4)
   bl record
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x36A0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0  # Make sure any stale exceptions don't break the next test.
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0  # Round-to-single-precision of 0.0 should cause no exceptions.
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfs (infinity)
0: lis %r3,0x7F80
   stw %r3,0(%r4)
   lfs %f6,0(%r4)
   bl record
   stfd %f6,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x7FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfs (QNaN)
0: lis %r3,0x7FD0
   stw %r3,0(%r4)
   lfs %f7,0(%r4)
   bl record
   stfd %f7,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x7FFA
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfs (SNaN)
0: lis %r3,0x7FA0
   stw %r3,0(%r4)
   lfs %f8,0(%r4)
   bl record
   stfd %f8,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x7FF4
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfsx (denormal)
0: li %r3,1
   stw %r3,0(%r4)
   lfsx %f3,0,%r4
   bl record
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x36A0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfsx (infinity)
0: lis %r3,0x7F80
   stw %r3,0(%r4)
   lfsx %f6,0,%r4
   bl record
   stfd %f6,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x7FF0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfsx (QNaN)
0: lis %r3,0x7FD0
   stw %r3,0(%r4)
   lfsx %f7,0,%r4
   bl record
   stfd %f7,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x7FFA
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # lfsx (SNaN)
0: lis %r3,0x7FA0
   stw %r3,0(%r4)
   lfsx %f8,0,%r4
   bl record
   stfd %f8,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x7FF4
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 2f
   cmpwi %r3,0
   beq 1f
2: li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (denormal)
0: lis %r3,0x36A0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f3,0(%r4)
   stfs %f3,0(%r4)
   bl record
   lwz %r3,0(%r4)
   li %r7,1
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (infinity)
0: lis %r3,0x7FF0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f6,0(%r4)
   stfs %f6,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lis %r7,0x7F80
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (QNaN)
0: lis %r3,0x7FFA
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f7,0(%r4)
   stfs %f7,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lis %r7,0x7FD0
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (QNaN)
0: lis %r3,0x7FF4
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f8,0(%r4)
   stfs %f8,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lis %r7,0x7FA0
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (denormal)
0: lis %r3,0x36A0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f3,0(%r4)
   stfsx %f3,0,%r4
   bl record
   lwz %r3,0(%r4)
   li %r7,1
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (infinity)
0: lis %r3,0x7FF0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f6,0(%r4)
   stfsx %f6,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r7,0x7F80
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (QNaN)
0: lis %r3,0x7FFA
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f7,0(%r4)
   stfsx %f7,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r7,0x7FD0
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   # stfs (QNaN)
0: lis %r3,0x7FF4
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f8,0(%r4)
   stfsx %f8,0,%r4
   bl record
   lwz %r3,0(%r4)
   lis %r7,0x7FA0
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,16(%r6)
   addi %r6,%r6,32
   frsp %f3,%f0
   mtfsf 255,%f0
   b 0f
1: frsp %f3,%f0
   fmr %f4,%f0
   bl check_fpu_pzero

   ########################################################################
   # 4.6.6 Floating-Point Rounding and Conversion Instructions - frsp
   # (and general rounding tests)
   ########################################################################

   # Preload floating-point constants for convenience in subsequent tests.
0: lfd %f24,80(%r31)   # 4194304.0
   lfd %f25,88(%r31)   # 4194304.125
   lfd %f26,96(%r31)   # 4194304.25
   lfd %f27,104(%r31)  # 4194304.375
   lfd %f28,112(%r31)  # 4194304.5
   lfd %f29,120(%r31)  # 4194304.75
   lfd %f30,128(%r31)  # 4194305.0
   lfd %f5,136(%r31)   # Single-precision minimum denormal
   lfd %f6,144(%r31)   # Single-precision minimum denormal / 4
   lfd %f7,152(%r31)   # HUGE_VALF (maximum normal single-precision value)
   lfd %f8,160(%r31)   # HUGE_VALF + epsilon/4
   lfd %f9,16(%r31)    # Positive infinity
   lfd %f10,32(%r31)   # QNaN
   lfd %f11,40(%r31)   # SNaN
   lfd %f12,48(%r31)   # SNaN converted to QNaN

   # frsp (RN = round to nearest)
0: frsp %f3,%f24    # 4194304.0 (exact)
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: frsp %f3,%f25    # 4194304.125 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f26    # 4194304.25 (round to even)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f27    # 4194304.375 (round up)
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm_inex
0: frsp %f3,%f28    # 4194304.5 (exact)
   bl record
   bl check_fpu_pnorm
0: frsp %f3,%f29    # 4194304.75 (round to even)
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm_inex
0: frsp %f3,%f30    # 4194305.0 (exact)
   bl record
   bl check_fpu_pnorm
0: frsp %f3,%f5     # Single-precision minimum denormal (exact)
   bl record
   fmr %f4,%f5
   bl check_fpu_pdenorm
0: frsp %f3,%f6     # Single-precision minimum denormal / 4 (round down)
   bl record
   fmr %f4,%f0
   bl add_fpscr_ux
   bl check_fpu_pzero_inex
0: fneg %f3,%f6     # -(Single-precision minimum denormal / 4) (round up)
   frsp %f3,%f3
   bl record
   bl add_fpscr_ux
   bl check_fpu_nzero_inex
0: frsp %f3,%f7     # HUGE_VALF (exact)
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: frsp %f3,%f8     # HUGE_VALF + epsilon/4 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: fneg %f4,%f8     # -(HUGE_VALF + epsilon/4) (round up)
   frsp %f3,%f4
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm_inex
0: frsp %f3,%f9     # Positive infinity (exact)
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: frsp %f3,%f10    # QNaN
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: frsp %f3,%f11    # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # frsp (RN = round toward zero)
0: mtfsfi 7,1
   frsp %f3,%f24    # 4194304.0 (exact)
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: frsp %f3,%f25    # 4194304.125 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f26    # 4194304.25 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f27    # 4194304.375 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f28    # 4194304.5 (exact)
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm
0: frsp %f3,%f29    # 4194304.75 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f30    # 4194305.0 (exact)
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm
0: frsp %f3,%f5     # Single-precision minimum denormal (exact)
   bl record
   fmr %f4,%f5
   bl check_fpu_pdenorm
0: frsp %f3,%f6     # Single-precision minimum denormal / 4 (round down)
   bl record
   fmr %f4,%f0
   bl add_fpscr_ux
   bl check_fpu_pzero_inex
0: fneg %f3,%f6     # -(Single-precision minimum denormal / 4) (round up)
   frsp %f3,%f3
   bl record
   bl add_fpscr_ux
   bl check_fpu_nzero_inex
0: frsp %f3,%f7     # HUGE_VALF (exact)
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: frsp %f3,%f8     # HUGE_VALF + epsilon/4 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: fneg %f4,%f8     # -(HUGE_VALF + epsilon/4) (round up)
   frsp %f3,%f4
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm_inex
0: frsp %f3,%f9     # Positive infinity (exact)
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: frsp %f3,%f10    # QNaN
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: frsp %f3,%f11    # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # frsp (RN = round toward +infinity)
0: mtfsfi 7,2
   frsp %f3,%f24    # 4194304.0 (exact)
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: frsp %f3,%f25    # 4194304.125 (round up)
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm_inex
0: frsp %f3,%f26    # 4194304.25 (round up)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f27    # 4194304.375 (round up)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f28    # 4194304.5 (exact)
   bl record
   bl check_fpu_pnorm
0: frsp %f3,%f29    # 4194304.75 (round up)
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm_inex
0: frsp %f3,%f30    # 4194305.0 (exact)
   bl record
   bl check_fpu_pnorm
0: frsp %f3,%f5     # Single-precision minimum denormal (exact)
   bl record
   fmr %f4,%f5
   bl check_fpu_pdenorm
0: frsp %f3,%f6     # Single-precision minimum denormal / 4 (round up)
   bl record
   bl add_fpscr_ux
   bl check_fpu_pdenorm_inex
0: fneg %f3,%f6     # -(Single-precision minimum denormal / 4) (round up)
   frsp %f3,%f3
   bl record
   bl add_fpscr_ux
   fmr %f4,%f0
   bl check_fpu_nzero_inex
0: frsp %f3,%f7     # HUGE_VALF (exact)
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: frsp %f3,%f8     # HUGE_VALF + epsilon/4 (round up)
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: fneg %f4,%f8     # -(HUGE_VALF + epsilon/4) (round up)
   frsp %f3,%f4
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm_inex
0: frsp %f3,%f9     # Positive infinity (exact)
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: frsp %f3,%f10    # QNaN
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: frsp %f3,%f11    # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # frsp (RN = round toward -infinity)
0: mtfsfi 7,3
   frsp %f3,%f24    # 4194304.0 (exact)
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: frsp %f3,%f25    # 4194304.125 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f26    # 4194304.25 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f27    # 4194304.375 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f28    # 4194304.5 (exact)
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm
0: frsp %f3,%f29    # 4194304.75 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: frsp %f3,%f30    # 4194305.0 (exact)
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm
0: frsp %f3,%f5     # Single-precision minimum denormal (exact)
   bl record
   fmr %f4,%f5
   bl check_fpu_pdenorm
0: frsp %f3,%f6     # Single-precision minimum denormal / 4 (round down)
   bl record
   fmr %f4,%f0
   bl add_fpscr_ux
   bl check_fpu_pzero_inex
0: fneg %f3,%f6     # -(Single-precision minimum denormal / 4) (round down)
   frsp %f3,%f3
   bl record
   fneg %f4,%f5
   bl add_fpscr_ux
   bl check_fpu_ndenorm_inex
0: frsp %f3,%f7     # HUGE_VALF (exact)
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: frsp %f3,%f8     # HUGE_VALF + epsilon/4 (round down)
   bl record
   bl check_fpu_pnorm_inex
0: fneg %f4,%f8     # -(HUGE_VALF + epsilon/4) (round down)
   frsp %f3,%f4
   bl record
   fneg %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_ninf_inex
0: frsp %f3,%f9     # Positive infinity (exact)
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: frsp %f3,%f10    # QNaN
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: frsp %f3,%f11    # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # frsp (SNaN and VE=1)
0: mtfsf 255,%f0
   mtfsb1 24
   fmr %f4,%f24
   frsp %f4,%f11  # Should not change the target register's contents.
   bl record
   fmr %f3,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # frsp (FPSCR[FX] handling)
0: frsp %f4,%f11
   mtfsb0 0
   frsp %f4,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # frsp (exception persistence)
0: frsp %f4,%f11
   frsp %f4,%f24
   bl record
   fmr %f3,%f4
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

.if TEST_UNDOCUMENTED
   # frsp (nonzero frA -- see notes at fadd)
0: .int 0xFC6BC018  # frsp %f3,%f24,frA=%f11
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
.endif

   # frsp.
   # We assume frsp and frsp. share the same implementation and just check
   # that CR1 is updated properly (and likewise in tests below).
0: frsp. %f3,%f11   # SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0    # SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   frsp. %f3,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11     # Register contents should be unchanged.
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   ########################################################################
   # 4.6.5.1 Floating-Point Elementary Arithmetic Instructions
   ########################################################################

   # Swap out some constants.
0: lfd %f2,72(%r31)    # 2.0
   lfd %f28,184(%r31)  # 2097152.0
   lfd %f29,192(%r31)  # 2097152.0625

   # fadd
0: fadd %f3,%f1,%f24    # normal + normal
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm
0: fadd %f3,%f9,%f1     # +infinity + normal
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fadd %f3,%f9,%f9     # +infinity + +infinity
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f13,%f9        # +infinity + -infinity
   fadd %f3,%f9,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: fadd %f3,%f10,%f1    # QNaN + normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fadd %f3,%f1,%f11    # normal + SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
   # Also test NaN selection order (should be frA -> frB -> frC).
0: fadd %f3,%f10,%f11   # QNaN + SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fadd %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # -infinity + +infinity (exception enabled)
   mtfsb1 24
   fneg %f13,%f9
   fmr %f3,%f24
   fadd %f3,%f13,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_fpu_noresult
   mtfsb0 24

   # fadd (FPSCR[FX] handling)
0: fadd %f4,%f1,%f11
   mtfsb0 0
   fadd %f4,%f1,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fadd (exception persistence)
0: fadd %f4,%f1,%f11
   fadd %f4,%f1,%f24
   bl record
   fmr %f3,%f4
   fmr %f4,%f30
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

.if TEST_UNDOCUMENTED
   # fadd (nonzero frC)
   # The PowerPC spec requires frC to be zero, but the 750CL just ignores
   # the frC field.  Likewise for the unused fields in the other 1- and
   # 2-operand floating-point instructions, though frC must be zero for
   # frsp and fctiw[z], suggesting that the processor reads the full
   # 10-bit XO field for OPCD 0x3F with XO bit 0x10 clear.  We use a SNaN
   # in the ignored fields to verify that the operand is in fact ignored.
   .int 0xFC61C2EA      # fadd %f3,%f1,%f24,frC=%f11
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm
.endif

   # fadd.
0: fadd. %f3,%f1,%f11   # normal + SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # normal + SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fadd. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fadds
0: fadds %f3,%f1,%f25   # normal + normal
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm_inex
0: fadds %f3,%f7,%f7    # normal + normal (result overflows)
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
   # Also check that the computation is performed in double precision
   # and only rounded to single precision at the end, rather than the
   # input values being rounded to single precision before performing the
   # computation.  (The PowerPC v2.01 spec says [Book I, 4.3.5.3] that
   # "[a]ll input values must be representable in single format; if they
   # are not, the result placed into the target FPR, and the setting of
   # status bits in the FPSCR and in the Condition Register (if Rc=1),
   # are undefined", but we test for the actual behavior of the 750CL.)
0: lfd %f4,336(%r31)    # 2^128 + -HUGE_VALF
   fneg %f13,%f7
   fadds %f3,%f4,%f13
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
0: lfd %f4,336(%r31)    # -HUGE_VALF + 2^128
   fneg %f13,%f7
   fadds %f3,%f13,%f4
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
   # Check rounding of input values.
0: mtfsfi 7,1           # 1.999...9 + 0.000...1 (round down)
   lfd %f3,304(%r31)
   lfd %f4,352(%r31)
   fadds %f3,%f3,%f4    # Input values are not truncated.
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1           # 0.000...1 + 1.999...9 (round down)
   lfd %f3,304(%r31)
   lfd %f4,352(%r31)
   fadds %f3,%f4,%f3
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0

.if TEST_UNDOCUMENTED
   # fadds (nonzero frC)
   .int 0xEC61CAEA      # fadds %f3,%f1,%f25,frC=%f11
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm_inex
.endif

   # fadds.
0: fadds. %f3,%f1,%f11  # normal + SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # fsub
0: fsub %f3,%f30,%f1    # normal - normal
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: fsub %f3,%f9,%f1     # +infinity - normal
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fsub %f3,%f9,%f9     # +infinity - +infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: fneg %f13,%f9        # +infinity - -infinity
   fsub %f3,%f9,%f13
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fsub %f3,%f10,%f1    # QNaN - normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fsub %f3,%f1,%f11    # normal - SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fsub %f3,%f10,%f11   # QNaN - SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fsub %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # -infinity - -infinity (exception enabled)
   mtfsb1 24
   fneg %f13,%f9
   fmr %f3,%f24
   fsub %f3,%f13,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_fpu_noresult
   mtfsb0 24

   # fsub (FPSCR[FX] handling)
0: fsub %f4,%f1,%f11
   mtfsb0 0
   fsub %f4,%f1,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fsub (exception persistence)
0: fsub %f4,%f1,%f11
   fsub %f4,%f30,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

.if TEST_UNDOCUMENTED
   # fsub (nonzero frC)
   .int 0xFC7E0AE8      # fsub %f3,%f30,%f1,frC=%f11
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
.endif

   # fsub.
0: fsub. %f3,%f1,%f11   # normal - SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # normal - SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fsub. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fsubs
0: lfd %f13,56(%r31)    # normal - normal
   fsubs %f3,%f27,%f13
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm_inex
0: fneg %f4,%f7         # -normal - normal (result overflows)
   fsubs %f3,%f4,%f7
   bl record
   fneg %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_ninf_inex
0: lfd %f4,336(%r31)    # 2^128 - HUGE_VALF
   fsubs %f3,%f4,%f7
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
0: lfd %f4,336(%r31)    # HUGE_VALF - 2^128
   fsubs %f3,%f7,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: mtfsfi 7,1           # 1.999...9 - -0.000...1 (round down)
   lfd %f3,304(%r31)
   lfd %f4,352(%r31)
   fneg %f4,%f4
   fsubs %f3,%f3,%f4
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1           # 0.000...1 - -1.999...9 (round down)
   lfd %f3,304(%r31)
   fneg %f3,%f3
   lfd %f4,352(%r31)
   fsubs %f3,%f4,%f3
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0

.if TEST_UNDOCUMENTED
   # fsubs (nonzero frC)
0: lfd %f13,56(%r31)
   .int 0xEC7B6AE8      # fsubs %f3,%f27,%f13,frC=%f11
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm_inex
.endif

   # fsubs.
0: fsubs. %f3,%f1,%f11  # normal - SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # fmul
0: fmul %f3,%f2,%f28    # normal * normal
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: fmul %f3,%f0,%f28    # 0 * normal
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: fneg %f4,%f0         # -0 * normal
   fmul %f3,%f4,%f28
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fneg %f4,%f28        # 0 * -normal
   fmul %f3,%f0,%f4
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fmul %f3,%f9,%f2     # +infinity * normal
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fmul %f3,%f9,%f9     # +infinity * +infinity
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f13,%f9        # +infinity * -infinity
   fmul %f3,%f9,%f13
   bl record
   fmr %f4,%f13
   bl check_fpu_ninf
0: fmul %f3,%f9,%f0     # +infinity * 0
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f13,%f9        # 0 * -infinity
   fmul %f3,%f0,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fmul %f3,%f10,%f1    # QNaN * normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fmul %f3,%f1,%f11    # normal * SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmul %f3,%f10,%f11   # QNaN * SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN * normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmul %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # 0 * +infinity (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmul %f3,%f0,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   bl check_fpu_noresult
   mtfsb0 24
   # frC rounding (see under fmuls below) should not occur for
   # double-precision operations.
0: mtfsfi 7,1           # 1.5 * 1.333...4 (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   addi %r3,%r3,1
   stw %r3,4(%r4)
   lfd %f3,0(%r4)
   lfd %f13,296(%r31)
   fmul %f3,%f13,%f3
   bl record
   lis %r3,0x4000
   stw %r3,8(%r4)
   stw %r0,12(%r4)
   lfd %f4,8(%r4)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0

   # fmul (FPSCR[FX] handling)
0: fmul %f4,%f1,%f11
   mtfsb0 0
   fmul %f4,%f1,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fmul (exception persistence)
0: fmul %f4,%f1,%f11
   fmul %f4,%f2,%f28
   bl record
   fmr %f3,%f4
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

.if TEST_UNDOCUMENTED
   # fmul (nonzero frB)
0: .int 0xFC625F32      # fmul %f3,%f2,%f28,frB=%f11
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
.endif

   # fmul.
0: fmul. %f3,%f1,%f11   # normal * SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # normal * SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fmul. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fmuls
0: fmuls %f3,%f2,%f29   # normal * normal
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm_inex
0: lfd %f4,328(%r31)    # normal * normal (result overflows)
   fmuls %f3,%f4,%f2
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: fneg %f4,%f0         # -0 * normal
   fmuls %f3,%f4,%f28
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fneg %f4,%f28        # 0 * -normal
   fmuls %f3,%f0,%f4
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: lfd %f4,336(%r31)    # 2^128 * 0.5
   lfd %f13,64(%r31)
   fmuls %f4,%f4,%f13
   bl record
   fmr %f3,%f4
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: lfd %f4,336(%r31)    # 0.5 * 2^128
   fmuls %f13,%f13,%f4
   bl record
   fmr %f3,%f13
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: mtfsfi 7,2           # 1.333... * 1.5 (round up)
   lfd %f3,288(%r31)
   lfd %f4,296(%r31)
   fmuls %f3,%f3,%f4
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
0: mtfsfi 7,1           # 1.333...4 * 1.5 (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   addi %r3,%r3,1
   stw %r3,4(%r4)
   lfd %f3,0(%r4)
   lfd %f13,296(%r31)
   fmuls %f3,%f3,%f13
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
0: mtfsfi 7,2           # 1.5 * 1.333... (round up)
   lfd %f3,288(%r31)
   lfd %f4,296(%r31)
   fmuls %f3,%f4,%f3
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
   # The second input (frC) to the single-precision multiplier gets its
   # mantissa rounded to 24 bits, using round-to-nearest (more accurately,
   # rounding up if the 25th bit is a 1 and down if it is a 0) regardless
   # of the current rounding mode in FPSCR.
   # This test will fail if multiplication is implemented using full precision.
0: mtfsfi 7,1           # 1.5 * 1.333...4 (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   addi %r3,%r3,1
   stw %r3,4(%r4)
   lfd %f3,0(%r4)
   lfd %f13,296(%r31)
   fmuls %f3,%f13,%f3
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,8(%r4)
   lfs %f4,8(%r4)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation rounds to 25 or more bits.
0: mtfsfi 7,2           # 1.5 * 1.333... @ 26 bits (round up)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   andis. %r3,%r3,0xFC00
   stw %r3,4(%r4)
   lfd %f13,0(%r4)
   lfd %f3,296(%r31)
   fmuls %f4,%f3,%f13
   bl record
   fmr %f3,%f4
   fmr %f4,%f2
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation rounds to 23 or fewer bits.
0: mtfsfi 7,1           # 1.5 * 1.333... @ 25 bits (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   andis. %r3,%r3,0xF800
   stw %r3,4(%r4)
   lfd %f3,0(%r4)
   lfd %f13,296(%r31)
   fmuls %f4,%f13,%f3
   bl record
   fmr %f3,%f4
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation follows FPSCR[RN] instead
   # of always rounding to nearest.
0: mtfsfi 7,1           # 1.5 * 1.333...4 @ 25 bits (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   andis. %r3,%r3,0xF800
   addis %r3,%r3,0x0800
   stw %r3,4(%r4)
   lfd %f4,0(%r4)
   lfd %f13,296(%r31)
   fmuls %f4,%f13,%f4
   bl record
   fmr %f3,%f4
   fmr %f4,%f2
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation does not set FPSCR[OX]
   # after rounding frC to infinity.
0: lfd %f4,320(%r31)    # HUGE_VAL * HUGE_VAL
   fmuls %f13,%f4,%f4
   bl record
   fmr %f3,%f13
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
   # This test will fail if the implementation rounds a large frC to
   # infinity when the result would not overflow.
0: lfd %f4,376(%r31)    # minimum_denormal * HUGE_VAL
   lfd %f13,320(%r31)
   fmuls %f13,%f4,%f13
   bl record
   fmr %f3,%f13
   lis %r3,0x3CD0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f4,0(%r4)
   bl check_fpu_pnorm
   # This test will fail if the implementation shifts out the sign bit of
   # frA when avoiding round-to-infinity of frC.  (It will also fail if
   # the previous test failed.)
0: lfd %f4,376(%r31)    # -minimum_denormal * HUGE_VAL
   fneg %f4,%f4
   lfd %f13,320(%r31)
   fmuls %f3,%f4,%f13
   bl record
   lis %r3,0xBCD0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f4,0(%r4)
   bl check_fpu_nnorm
   # This test will fail if the implementation sets FPSCR[FI] or FPSCR[XX]
   # based on the result of rounding frC.
0: lfd %f3,384(%r31)    # 1 * (1.0f - 0.25ulp)
   fmuls %f13,%f1,%f3
   bl record
   fmr %f3,%f13
   fmr %f4,%f1
   bl check_fpu_pnorm
   # One or both of these two tests will fail if the implementation does
   # not normalize denormals before rounding.
0: mtfsfi 7,1           # 2^1023 * (2^-1048 - 1ulp)
   lis %r3,0x7FE0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   stw %r0,8(%r4)
   lis %r3,0x3FF
   ori %r3,%r3,0xFFFF
   stw %r3,12(%r4)
   lfd %f3,0(%r4)
   lfd %f4,8(%r4)
   fmuls %f4,%f3,%f4
   bl record
   fmr %f3,%f4
   lis %r3,0x3E60
   stw %r3,16(%r4)
   stw %r0,20(%r4)
   lfd %f4,16(%r4)
   bl check_fpu_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1           # 2^1023 * (2^-1049 - 1ulp)
   lis %r3,0x7FE0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   stw %r0,8(%r4)
   lis %r3,0x1FF
   ori %r3,%r3,0xFFFF
   stw %r3,12(%r4)
   lfd %f4,0(%r4)
   lfd %f3,8(%r4)
   fmuls %f4,%f4,%f3
   bl record
   fmr %f3,%f4
   lis %r3,0x3E4F
   ori %r3,%r3,0xFFFF
   stw %r3,16(%r4)
   lis %r3,0xE000
   stw %r3,20(%r4)
   lfd %f4,16(%r4)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
   # This test will fail if normalizing frC shifts the sign bit out.
0: mtfsfi 7,1           # 2^1023 * -(2^-1048 - 1ulp)
   lis %r3,0x7FE0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lis %r3,0x8000
   stw %r3,8(%r4)
   lis %r3,0x3FF
   ori %r3,%r3,0xFFFF
   stw %r3,12(%r4)
   lfd %f3,0(%r4)
   lfd %f4,8(%r4)
   fmuls %f13,%f3,%f4
   bl record
   fmr %f3,%f13
   lis %r3,0xBE60
   stw %r3,16(%r4)
   stw %r0,20(%r4)
   lfd %f4,16(%r4)
   bl check_fpu_nnorm
   mtfsfi 7,0
   # This test will fail if the implementation fails to raise an inexact
   # or underflow exception when normalization of a second-operand denormal
   # forces the first operand to zero.
0: lfd %f4,240(%r31)   # minimum_normal * minimum_denormal
   lfd %f3,376(%r31)
   fmuls %f13,%f4,%f3
   bl record
   fmr %f3,%f13
   fmr %f4,%f0
   bl add_fpscr_ux
   bl check_fpu_pzero_inex
   # This test will fail if the implementation changes the sign of the
   # result during underflow.
0: lfd %f13,376(%r31)   # minimum_denormal * -minimum_denormal
   fneg %f3,%f13
   fmuls %f13,%f13,%f3
   bl record
   fmr %f3,%f13
   fneg %f4,%f0
   bl add_fpscr_ux
   bl check_fpu_nzero_inex
   # This test will fail if the implementation raises an inexact exception
   # when multiplying a rounded value by zero.
0: lfd %f13,288(%r31)   # 0 * 1.333...
   fmuls %f13,%f0,%f13
   bl record
   fmr %f3,%f13
   fmr %f4,%f0
   bl check_fpu_pzero
   # This test will fail if the implementation raises a 0*inf exception
   # when multiplying a value that gets rounded up to infinity.
0: lfd %f4,320(%r31)    # 0 * HUGE_VAL
   fmuls %f3,%f0,%f4
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
   # This test will fail if the implementation raises a 0*inf exception
   # when multiplying infinity by a value that gets rounded down to zero.
0: lfd %f4,376(%r31)    # infinity * minimum_denormal
   fmuls %f3,%f9,%f4
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
   # This test will fail (with a result of zero) if the implementation
   # blindly rounds up the mantissa without checking for NaNness.
0: li %r3,-1            # 1 * SNaN(-1)
   stw %r3,0(%r4)
   stw %r3,4(%r4)
   lfd %f13,0(%r4)
   fmuls %f3,%f1,%f13
   bl record
   lis %r3,0xE000
   stw %r3,4(%r4)
   lfd %f4,0(%r4)
   bl check_fpu_nan

.if TEST_UNDOCUMENTED
   # fmuls (nonzero frB)
0: .int 0xEC625F72      # fmuls %f3,%f2,%f29,frB=%f11
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm_inex
.endif

   # fmuls.
0: fmuls. %f3,%f1,%f11  # normal * SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # fdiv
0: fdiv %f3,%f24,%f2    # normal / normal
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm
0: fdiv %f3,%f9,%f2     # +infinity / normal
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fdiv %f3,%f9,%f9     # +infinity / +infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxidi
   bl check_fpu_nan
0: fdiv %f3,%f2,%f0     # normal / 0
   bl record
   fmr %f4,%f9
   bl add_fpscr_zx
   bl check_fpu_pinf
0: fneg %f3,%f0         # normal / -0
   fdiv %f3,%f2,%f3
   bl record
   fneg %f4,%f9
   bl add_fpscr_zx
   bl check_fpu_ninf
0: fdiv %f3,%f0,%f0     # 0 / 0
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxzdz
   bl check_fpu_nan
0: fdiv %f3,%f10,%f1    # QNaN / normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fdiv %f3,%f1,%f11    # normal / SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fdiv %f3,%f10,%f11   # QNaN / SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN / normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fdiv %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # +infinity / -infinity (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fneg %f13,%f9
   fdiv %f3,%f9,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxidi
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # 0 / 0 (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmr %f4,%f0
   fdiv %f3,%f4,%f4
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxzdz
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsb1 27            # normal / 0 (exception enabled)
   fmr %f3,%f24
   fdiv %f3,%f1,%f0
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_zx
   bl check_fpu_noresult
   mtfsb0 27

   # fdiv (FPSCR[FX] handling)
0: fdiv %f4,%f1,%f11
   mtfsb0 0
   fdiv %f4,%f1,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fdiv (exception persistence)
0: fdiv %f4,%f1,%f11
   fdiv %f4,%f24,%f2
   bl record
   fmr %f3,%f4
   fmr %f4,%f28
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

.if TEST_UNDOCUMENTED
   # fdiv (nonzero frC)
0: .int 0xFC7812E4      # fdiv %f3,%f24,%f2,frC=%f11
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm
.endif

   # fdiv.
0: fdiv. %f3,%f1,%f11   # normal / SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # normal / SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fdiv. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fdivs
0: fdivs %f3,%f25,%f2  # normal / normal
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm_inex
0: lfd %f3,328(%r31)    # normal / normal (result overflows)
   lfd %f4,64(%r31)
   fdivs %f3,%f3,%f4
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: lfd %f4,336(%r31)    # 2^128 / 2.0
   fdivs %f3,%f4,%f2
   bl record
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: lfd %f4,336(%r31)    # 2^128 / 0.5
   lfd %f3,64(%r31)
   fdivs %f3,%f4,%f3
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: lis %r3,0x3FFA       # 1.666666716337204 / 1.25 = 1.3333333730697632 (exact)
   ori %r3,%r3,0xAAAA
   stw %r3,0(%r4)
   lis %r3,0xB800
   stw %r3,4(%r4)
   lfd %f13,0(%r4)
   lis %r3,0x3FA0
   stw %r3,8(%r4)
   lfs %f4,8(%r4)
   fdivs %f3,%f13,%f4
   bl record
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAB
   stw %r3,12(%r4)
   lfs %f4,12(%r4)
   bl check_fpu_pnorm
0: lfd %f4,0(%r4)       # 1.666666716337204 / 1.3333333730697632 = 1.25 (exact)
   lfs %f13,12(%r4)
   fdivs %f3,%f4,%f13
   bl record
   lfs %f4,8(%r4)
   bl check_fpu_pnorm

.if TEST_UNDOCUMENTED
   # fdivs (nonzero frC)
0: .int 0xEC7912E4      # fdivs %f3,%f25,%f2,frC=%f11
   bl record
   fmr %f4,%f28
   bl check_fpu_pnorm_inex
.endif

   # fdivs.
0: fdivs. %f3,%f1,%f11  # normal / SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # Check detection of underflow conditions.  In the PowerPC architecture,
   # the underflow exception detects tininess before rounding, so a result
   # which rounds up to the minimum normal value should set UX=1.  Due to
   # the nature of the operations, it's not possible to cause underflow on
   # double-precision fadd and fsub, but we can trigger it with fadds and
   # fsubs by taking advantage of the fact that the 750CL allows double-
   # precision operands to single-precision instructions.

0: lfs %f3,12(%r30)
   lfd %f13,240(%r31)
   fneg %f13,%f13
   fadds %f3,%f3,%f13
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfs %f3,12(%r30)
   lfd %f13,240(%r31)
   fsubs %f3,%f3,%f13
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfd %f3,392(%r31)
   lfd %f13,64(%r31)
   fmul %f3,%f3,%f13
   bl record
   lfd %f4,240(%r31)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfs %f3,52(%r30)
   lfd %f13,64(%r31)
   fmuls %f3,%f3,%f13
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfd %f3,392(%r31)
   fdiv %f3,%f3,%f2
   bl record
   lfd %f4,240(%r31)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfs %f3,52(%r30)
   fdivs %f3,%f3,%f2
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex

   # This is a convenient place to check FPRF values for double-precision
   # denormals since we don't generate them anywhere else.

0: lfd %f13,240(%r31)
   fdiv %f3,%f13,%f2
   bl record
   lfd %f4,248(%r31)
   bl check_fpu_pdenorm
0: fneg %f3,%f2
   fdiv %f3,%f13,%f3
   bl record
   fneg %f4,%f4
   bl check_fpu_ndenorm

   ########################################################################
   # 4.6.5.2 Floating-Point Multiply-Add Instructions
   ########################################################################

   # fmadd
0: fmadd %f3,%f2,%f28,%f1   # normal * normal + normal
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm
0: fmadd %f3,%f0,%f1,%f2    # 0 * normal + normal
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
0: fneg %f4,%f0             # -0 * normal + 0
   fmadd %f3,%f4,%f1,%f0
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: fneg %f4,%f1             # 0 * -normal + 0
   fmadd %f3,%f0,%f4,%f0
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: fneg %f4,%f0             # 0 * normal + -0
   fmadd %f3,%f0,%f1,%f4
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: lfd %f3,240(%r31)        # minimum_normal * minimum_normal + -minimum_normal
   fneg %f4,%f3
   fmadd %f3,%f3,%f3,%f4
   bl record
   lfd %f4,240(%r31)
   fneg %f4,%f4
   bl add_fpscr_ux          # Result is tiny and inexact.
   bl check_fpu_nnorm_inex
0: fmadd %f3,%f9,%f2,%f1    # +infinity * normal + normal
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fmadd %f3,%f9,%f2,%f9    # +infinity * normal + +infinity
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f13,%f9            # +infinity * -normal + -infinity
   fneg %f2,%f2
   fmadd %f3,%f9,%f2,%f13
   bl record
   fneg %f2,%f2
   fneg %f4,%f9
   bl check_fpu_ninf
0: fneg %f13,%f9            # -infinity * normal + +infinity
   fmadd %f3,%f13,%f2,%f9
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: lfd %f13,320(%r31)       # huge * -huge + +infinity
   fneg %f4,%f13
   fmadd %f3,%f13,%f4,%f9
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fmadd %f3,%f9,%f0,%f1    # +infinity * 0 + normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f4,%f9             # +infinity * 0 + -infinity
   fmadd %f3,%f9,%f0,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f4,%f9             # +infinity * +normal + -infinity
   fmadd %f3,%f9,%f1,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: fneg %f4,%f9             # 0 * +infinity + -infinity
   fmadd %f3,%f0,%f9,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f4,%f9             # +normal * +infinity + -infinity
   fmadd %f3,%f1,%f9,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: fneg %f13,%f9            # 0 * -infinity + SNaN
   fmadd %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmadd %f3,%f10,%f1,%f2   # QNaN * normal + normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fmadd %f3,%f1,%f11,%f2   # normal * SNaN + normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmadd %f3,%f1,%f2,%f11   # normal * normal + SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fneg %f4,%f9             # +infinity * QNaN + -infinity
   fmadd %f3,%f9,%f10,%f4
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fmadd %f3,%f10,%f11,%f0  # QNaN * SNaN + 0
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmadd %f3,%f10,%f0,%f11  # QNaN * 0 + SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmadd %f3,%f0,%f11,%f10  # 0 * SNaN + QNaN
   bl record
   fmr %f4,%f10             # The addend is frB, so it gets precedence.
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fneg %f3,%f12            # QNaN * SNaN + QNaN
   fmadd %f3,%f10,%f11,%f3
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # SNaN * normal + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmadd %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # 0 * +infinity + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmadd %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # +infinity * normal + -infinity (exception enabled)
   mtfsb1 24
   fneg %f13,%f9
   fmr %f3,%f24
   fmadd %f3,%f9,%f2,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_fpu_noresult
   mtfsb0 24
   # Check that the product is not rounded.
0: mtfsfi 7,1               # 1.333... * 1.5 + 0 (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   fmadd %f3,%f3,%f13,%f0   # Should truncate to (2.0 - 1ulp).
   bl record
   lfd %f4,304(%r31)
   bl check_fpu_pnorm_inex
   lfd %f4,312(%r31)
   fadd %f3,%f3,%r4         # Adding 1ulp to the result should not change it.
   lfd %f4,304(%r31)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.333... * 1.5 + 1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fmadd %f3,%f3,%f13,%f4   # Should be exactly 2.0.
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... + 1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fmadd %f3,%f13,%f3,%f4   # Should be exactly 2.0 (no frC rounding).
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0

   # fmadd (FPSCR[FX] handling)
0: fmadd %f4,%f1,%f11,%f2
   mtfsb0 0
   fmadd %f4,%f1,%f11,%f2
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fmadd (exception persistence)
0: fmadd %f4,%f1,%f11,%f2
   fmadd %f4,%f2,%f28,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f30
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

   # fmadd.
0: fmadd. %f3,%f1,%f11,%f2  # normal * SNaN + normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # normal * SNaN + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fmadd. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fmadds
0: fmadds %f3,%f2,%f29,%f1  # normal * normal + normal
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm_inex
0: fneg %f13,%f1            # normal * normal + -normal
   lfd %f4,120(%r31)
   fmadds %f3,%f1,%f4,%f13
   bl record
   lfd %f4,200(%r31)        # The multiply result should not be rounded.
   bl check_fpu_pnorm
0: lfd %f3,328(%r31)        # huge * normal + -normal
   fneg %f4,%f7
   fmadds %f3,%f3,%f2,%f4
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
0: lfs %f3,12(%r30)         # minimum_normal * minimum_normal + -minimum_normal
   fneg %f4,%f3
   fmadds %f3,%f3,%f3,%f4
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   bl check_fpu_nnorm_inex
0: mtfsfi 7,1               # 1.333... * 1.5 + 1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fmadds %f3,%f3,%f13,%f4  # Should be exactly 2.0 even for single precision.
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... + 1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fmadds %f3,%f13,%f3,%f4  # Should round down due to frC rounding.
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
0: fneg %f13,%f7            # HUGE_VALF * 2 + -HUGE_VALF
   fmadds %f3,%f7,%f2,%f13
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: fneg %f13,%f7            # 2 * HUGE_VALF + -HUGE_VALF
   fmadds %f3,%f2,%f7,%f13
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
   # The following cases are technically undefined behavior, but we test
   # for the actual behavior of the 750CL.
0: lfd %f13,336(%r31)       # 2^128 * 0.5 + 0
   lfd %f4,64(%r31)
   fmadds %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: lfd %f13,336(%r31)       # 0.5 * 2^128 + 0
   lfd %f4,64(%r31)
   fmadds %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: lfd %f4,336(%r31)        # 1 * -HUGE_VALF + 2^128
   fneg %f13,%f7
   fmadds %f3,%f1,%f13,%f4
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 1 + -HUGE_VAL
   fneg %f13,%f4
   fmadds %f4,%f4,%f1,%f13
   bl record
   fmr %f3,%f4
   fmr %f4,%f0
   bl check_fpu_pzero
0: lfd %f4,320(%r31)        # 1 * HUGE_VAL + -HUGE_VAL
   fneg %f13,%f4
   fmadds %f4,%f1,%f4,%f13
   bl record
   fmr %f3,%f4
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: lfd %f3,64(%r31)         # 0.5 * HUGE_VAL + -HUGE_VAL
   lfd %f4,320(%r31)
   fneg %f13,%f4
   fmadds %f4,%f3,%f4,%f13
   bl record
   fmr %f3,%f4
   fneg %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_ninf_inex
0: lfd %f13,288(%r31)       # 0 * 1.333... + 1
   fmadds %f13,%f0,%f13,%f1
   bl record
   fmr %f3,%f13
   fmr %f4,%f1
   bl check_fpu_pnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 0 + 1
   fmadds %f4,%f4,%f0,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f1
   bl check_fpu_pnorm
0: lfd %f4,320(%r31)        # 0 * HUGE_VAL + 1
   fmadds %f4,%f0,%f4,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f1
   bl check_fpu_pnorm
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + -min_normal_f
   lfs %f3,12(%r30)
   fneg %f3,%f3
   fmadds %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   bl check_fpu_nnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + 1
   fmadds %f3,%f4,%f4,%f1
   bl record
   fmr %f4,%f1
   bl check_fpu_pnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + inf
   fmadds %f3,%f4,%f4,%f9
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + QNaN
   fmadds %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   bl check_fpu_nan

   # fmadds.
0: fmadds. %f3,%f1,%f11,%f2 # normal * SNaN + normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # fmsub
0: fmsub %f3,%f1,%f30,%f1   # normal * normal - normal
   bl record
   fmr %f4,%f24
   bl check_fpu_pnorm
0: fmsub %f3,%f0,%f1,%f2    # 0 * normal - normal
   bl record
   fneg %f4,%f2
   bl check_fpu_nnorm
0: fneg %f4,%f0             # -0 * normal - 0
   fmsub %f3,%f4,%f1,%f0
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fneg %f4,%f1             # 0 * -normal - 0
   fmsub %f3,%f0,%f4,%f0
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fneg %f4,%f0             # 0 * normal - -0
   fmsub %f3,%f0,%f1,%f4
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: lfd %f3,240(%r31)        # minimum_normal * minimum_normal - minimum_normal
   fmsub %f3,%f3,%f3,%f3
   bl record
   lfd %f4,240(%r31)
   fneg %f4,%f4
   bl add_fpscr_ux
   bl check_fpu_nnorm_inex
0: fmsub %f3,%f9,%f2,%f1    # +infinity * normal - normal
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f13,%f9            # +infinity * normal - -infinity
   fmsub %f3,%f9,%f2,%f13
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f2,%f2             # +infinity * -normal - +infinity
   fmsub %f3,%f9,%f2,%f9
   bl record
   fneg %f2,%f2
   fneg %f4,%f9
   bl check_fpu_ninf
0: fneg %f13,%f9            # -infinity * normal - -infinity
   fmsub %f3,%f13,%f2,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: lfd %f13,320(%r31)       # huge * -huge - -infinity
   fneg %f4,%f13
   fneg %f3,%f9
   fmsub %f3,%f13,%f4,%f3
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: fmsub %f3,%f9,%f0,%f1    # +infinity * 0 - normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fmsub %f3,%f9,%f0,%f9    # +infinity * 0 - infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f13,%f9            # 0 * -infinity - SNaN
   fmsub %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmsub %f3,%f10,%f1,%f2   # QNaN * normal - normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fmsub %f3,%f1,%f11,%f2   # normal * SNaN - normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmsub %f3,%f1,%f2,%f11   # normal * normal - SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmsub %f3,%f9,%f10,%f9   # +infinity * QNaN - infinity
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fmsub %f3,%f10,%f11,%f0  # QNaN * SNaN - 0
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmsub %f3,%f10,%f0,%f11  # QNaN * 0 - SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fmsub %f3,%f0,%f11,%f10  # 0 * SNaN - QNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fneg %f3,%f12            # QNaN * SNaN - QNaN
   fmsub %f3,%f10,%f11,%f3
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # SNaN * normal - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmsub %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # 0 * +infinity - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmsub %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # +infinity * normal - +infinity (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fmsub %f3,%f9,%f2,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_fpu_noresult
   mtfsb0 24

   # fmsub (FPSCR[FX] handling)
0: fmsub %f4,%f1,%f11,%f2
   mtfsb0 0
   fmsub %f4,%f1,%f11,%f2
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fmsub (exception persistence)
0: fmsub %f4,%f1,%f11,%f2
   fmsub %f4,%f1,%f30,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   bl check_fpu_pnorm

   # fmsub.
0: fmsub. %f3,%f1,%f11,%f2  # normal * SNaN - normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # normal * SNaN - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fmsub. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fmsubs
0: fneg %f13,%f1            # normal * normal - -normal
   fmsubs %f3,%f2,%f29,%f13
   bl record
   fmr %f4,%f30
   bl check_fpu_pnorm_inex
0: lfd %f4,120(%r31)        # normal * normal - normal
   fmsubs %f3,%f1,%f4,%f1
   bl record
   lfd %f4,200(%r31)
   bl check_fpu_pnorm
0: lfd %f3,328(%r31)        # huge * normal - normal
   fmsubs %f3,%f3,%f2,%f7
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
0: lfs %f3,12(%r30)         # minimum_normal * minimum_normal - minimum_normal
   fmsubs %f3,%f3,%f3,%f3
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   bl check_fpu_nnorm_inex
0: mtfsfi 7,1               # 1.333... * 1.5 - -1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fneg %f4,%f4
   fmsubs %f3,%f3,%f13,%f4
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... - -1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fneg %f4,%f4
   fmsubs %f3,%f13,%f3,%f4
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   bl check_fpu_pnorm_inex
   mtfsfi 7,0
0: fmsubs %f3,%f7,%f2,%f7   # HUGE_VALF * 2 - HUGE_VALF
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: fmsubs %f3,%f2,%f7,%f7   # 2 * HUGE_VALF - HUGE_VALF
   bl record
   fmr %f4,%f7
   bl check_fpu_pnorm
0: lfd %f13,336(%r31)       # 2^128 * 0.5 - 0
   lfd %f4,64(%r31)
   fmsubs %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: lfd %f13,336(%r31)       # 0.5 * 2^128 - 0
   lfd %f4,64(%r31)
   fmsubs %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   bl check_fpu_pnorm
0: lfd %f4,336(%r31)        # 1 * HUGE_VALF - 2^128
   fmsubs %f3,%f1,%f7,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 1 - HUGE_VAL
   fmsubs %f4,%f4,%f1,%f4
   bl record
   fmr %f3,%f4
   fmr %f4,%f0
   bl check_fpu_pzero
0: lfd %f4,320(%r31)        # 1 * HUGE_VAL - HUGE_VAL
   fmsubs %f4,%f1,%f4,%f4
   bl record
   fmr %f3,%f4
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: lfd %f3,64(%r31)         # 0.5 * HUGE_VAL - HUGE_VAL
   lfd %f4,320(%r31)
   fmsubs %f4,%f1,%f3,%f4
   bl record
   fmr %f3,%f4
   fneg %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_ninf_inex
0: lfd %f13,288(%r31)       # 0 * 1.333... - 1
   fmsubs %f13,%f0,%f13,%f1
   bl record
   fmr %f3,%f13
   fneg %f4,%f1
   bl check_fpu_nnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 0 - 1
   fmsubs %f4,%f4,%f0,%f1
   bl record
   fmr %f3,%f4
   fneg %f4,%f1
   bl check_fpu_nnorm
0: lfd %f4,320(%r31)        # 0 * HUGE_VAL - 1
   fmsubs %f4,%f0,%f4,%f1
   bl record
   fmr %f3,%f4
   fneg %f4,%f1
   bl check_fpu_nnorm
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - min_normal_f
   lfs %f3,12(%r30)
   fmsubs %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   bl check_fpu_nnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - 1
   fmsubs %f3,%f4,%f4,%f1
   bl record
   fneg %f4,%f1
   bl check_fpu_nnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - inf
   fmsubs %f3,%f4,%f4,%f9
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - QNaN
   fmsubs %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   bl check_fpu_nan

   # fmsubs.
0: fmsubs. %f3,%f1,%f11,%f2 # normal * SNaN - normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # fnmadd
0: fnmadd %f3,%f2,%f28,%f1  # normal * normal + normal
   bl record
   fneg %f4,%f30
   bl check_fpu_nnorm
0: fnmadd %f3,%f0,%f1,%f2   # 0 * normal + normal
   bl record
   fneg %f4,%f2
   bl check_fpu_nnorm
0: fneg %f4,%f0             # -0 * normal + 0
   fnmadd %f3,%f4,%f1,%f0
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fneg %f4,%f1             # 0 * -normal + 0
   fnmadd %f3,%f0,%f4,%f0
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fneg %f4,%f0             # 0 * normal + -0
   fnmadd %f3,%f0,%f1,%f4
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: lfd %f3,240(%r31)        # minimum_normal * minimum_normal + -minimum_normal
   fneg %f4,%f3
   fnmadd %f3,%f3,%f3,%f4
   bl record
   lfd %f4,240(%r31)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: fnmadd %f3,%f9,%f2,%f1   # +infinity * normal + normal
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: fnmadd %f3,%f9,%f2,%f9   # +infinity * normal + +infinity
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: fneg %f13,%f9            # +infinity * -normal + -infinity
   fneg %f2,%f2
   fnmadd %f3,%f9,%f2,%f13
   bl record
   fneg %f2,%f2
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f13,%f9            # -infinity * normal + +infinity
   fnmadd %f3,%f13,%f2,%f9
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: lfd %f13,320(%r31)       # huge * -huge + +infinity
   fneg %f4,%f13
   fnmadd %f3,%f13,%f4,%f9
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: fnmadd %f3,%f9,%f0,%f1   # +infinity * 0 + normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f4,%f9             # +infinity * 0 + -infinity
   fnmadd %f3,%f9,%f0,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f13,%f9            # 0 * -infinity + SNaN
   fnmadd %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmadd %f3,%f10,%f1,%f2  # QNaN * normal + normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fnmadd %f3,%f1,%f11,%f2  # normal * SNaN + normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmadd %f3,%f1,%f2,%f11  # normal * normal + SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fneg %f4,%f9             # +infinity * QNaN + -infinity
   fnmadd %f3,%f9,%f10,%f4
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fnmadd %f3,%f10,%f11,%f0  # QNaN * SNaN + 0
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmadd %f3,%f10,%f0,%f11  # QNaN * 0 + SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmadd %f3,%f0,%f11,%f10  # 0 * SNaN + QNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fneg %f3,%f10            # QNaN * SNaN + QNaN
   fnmadd %f3,%f10,%f11,%f3
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # SNaN * normal + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fnmadd %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # 0 * +infinity + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fnmadd %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # +infinity * normal + -infinity (exception enabled)
   mtfsb1 24
   fneg %f13,%f9
   fmr %f3,%f24
   fnmadd %f3,%f9,%f2,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_fpu_noresult
   mtfsb0 24

   # fnmadd (FPSCR[FX] handling)
0: fnmadd %f4,%f1,%f11,%f2
   mtfsb0 0
   fnmadd %f4,%f1,%f11,%f2
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fnmadd (exception persistence)
0: fnmadd %f4,%f1,%f11,%f2
   fnmadd %f4,%f2,%f28,%f1
   bl record
   fmr %f3,%f4
   fneg %f4,%f30
   bl add_fpscr_vxsnan
   bl check_fpu_nnorm

   # fnmadd.
0: fnmadd. %f3,%f1,%f11,%f2  # normal * SNaN + normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # normal * SNaN + normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fnmadd. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fnmadds
0: fnmadds %f3,%f2,%f29,%f1 # normal * normal + normal
   bl record
   fneg %f4,%f30
   bl check_fpu_nnorm_inex
0: fneg %f13,%f1            # normal * normal + -normal
   lfd %f4,120(%r31)
   fnmadds %f3,%f1,%f4,%f13
   bl record
   lfd %f4,200(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f3,328(%r31)        # huge * normal + -normal
   fneg %f4,%f7
   fnmadds %f3,%f3,%f2,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfs %f3,12(%r30)         # minimum_normal * minimum_normal + -minimum_normal
   fneg %f4,%f3
   fnmadds %f3,%f3,%f3,%f4
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: mtfsfi 7,1               # 1.333... * 1.5 + 1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fnmadds %f3,%f3,%f13,%f4
   bl record
   fneg %f4,%f2
   bl check_fpu_nnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... + 1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fnmadds %f3,%f13,%f3,%f4
   bl record
   lis %r3,0xC000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   bl check_fpu_nnorm_inex
   mtfsfi 7,0
0: fneg %f13,%f7            # HUGE_VALF * 2 + -HUGE_VALF
   fnmadds %f3,%f7,%f2,%f13
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm
0: fneg %f13,%f7            # 2 * HUGE_VALF + -HUGE_VALF
   fnmadds %f3,%f2,%f7,%f13
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm
0: lfd %f13,336(%r31)       # 2^128 * 0.5 + 0
   lfd %f4,64(%r31)
   fnmadds %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f13,336(%r31)       # 0.5 * 2^128 + 0
   lfd %f4,64(%r31)
   fnmadds %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f4,336(%r31)        # 1 * -HUGE_VALF + 2^128
   fneg %f13,%f7
   fnmadds %f3,%f1,%f13,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 1 + -HUGE_VAL
   fneg %f13,%f4
   fnmadds %f4,%f4,%f1,%f13
   bl record
   fmr %f3,%f4
   fneg %f4,%f0
   bl check_fpu_nzero
0: lfd %f4,320(%r31)        # 1 * HUGE_VAL + -HUGE_VAL
   fneg %f13,%f4
   fnmadds %f4,%f1,%f4,%f13
   bl record
   fmr %f3,%f4
   fneg %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_ninf_inex
0: lfd %f3,64(%r31)         # 0.5 * HUGE_VAL + -HUGE_VAL
   lfd %f4,320(%r31)
   fneg %f13,%f4
   fnmadds %f4,%f1,%f3,%f13
   bl record
   fmr %f3,%f4
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: lfd %f13,288(%r31)       # 0 * 1.333... + 1
   fnmadds %f13,%f0,%f13,%f1
   bl record
   fmr %f3,%f13
   fneg %f4,%f1
   bl check_fpu_nnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 0 + 1
   fnmadds %f4,%f4,%f0,%f1
   bl record
   fmr %f3,%f4
   fneg %f4,%f1
   bl check_fpu_nnorm
0: lfd %f4,320(%r31)        # 0 * HUGE_VAL + 1
   fnmadds %f4,%f0,%f4,%f1
   bl record
   fmr %f3,%f4
   fneg %f4,%f1
   bl check_fpu_nnorm
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + -min_normal_f
   lfs %f3,12(%r30)
   fneg %f3,%f3
   fnmadds %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + 1
   fnmadds %f3,%f4,%f4,%f1
   bl record
   fneg %f4,%f1
   bl check_fpu_nnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + inf
   fnmadds %f3,%f4,%f4,%f9
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d + QNaN
   fnmadds %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   bl check_fpu_nan

   # fnmadds.
0: fnmadds. %f3,%f1,%f11,%f2 # normal * SNaN + normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   # fnmsub
0: fnmsub %f3,%f1,%f30,%f1  # normal * normal - normal
   bl record
   fneg %f4,%f24
   bl check_fpu_nnorm
0: fnmsub %f3,%f0,%f1,%f2   # 0 * normal - normal
   bl record
   fmr %f4,%f2
   bl check_fpu_pnorm
0: fneg %f4,%f0             # -0 * normal - 0
   fnmsub %f3,%f4,%f1,%f0
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: fneg %f4,%f1             # 0 * -normal - 0
   fnmsub %f3,%f0,%f4,%f0
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: fneg %f4,%f0             # 0 * normal - -0
   fnmsub %f3,%f0,%f1,%f4
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: lfd %f3,240(%r31)        # minimum_normal * minimum_normal - minimum_normal
   fnmsub %f3,%f3,%f3,%f3
   bl record
   lfd %f4,240(%r31)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: fnmsub %f3,%f9,%f2,%f1   # +infinity * normal - normal
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: fneg %f13,%f9            # +infinity * normal - -infinity
   fnmsub %f3,%f9,%f2,%f13
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: fneg %f2,%f2             # +infinity * -normal - +infinity
   fnmsub %f3,%f9,%f2,%f9
   bl record
   fneg %f2,%f2
   fmr %f4,%f9
   bl check_fpu_pinf
0: fneg %f13,%f9            # -infinity * normal - -infinity
   fnmsub %f3,%f13,%f2,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   bl check_fpu_nan
0: lfd %f13,320(%r31)       # huge * -huge - -infinity
   fneg %f4,%f13
   fneg %f3,%f9
   fnmsub %f3,%f13,%f4,%f3
   bl record
   fneg %f4,%f9
   bl check_fpu_ninf
0: fnmsub %f3,%f9,%f0,%f1   # +infinity * 0 - normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fnmsub %f3,%f9,%f0,%f9   # +infinity * 0 - infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   bl check_fpu_nan
0: fneg %f13,%f9            # 0 * -infinity - SNaN
   fnmsub %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmsub %f3,%f10,%f1,%f2  # QNaN * normal - normal
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fnmsub %f3,%f1,%f11,%f2  # normal * SNaN - normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmsub %f3,%f1,%f2,%f11  # normal * normal - SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmsub %f3,%f9,%f10,%f9  # +infinity * QNaN - infinity
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fnmsub %f3,%f10,%f11,%f0  # QNaN * SNaN - 0
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmsub %f3,%f10,%f0,%f11  # QNaN * 0 - SNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fnmsub %f3,%f0,%f11,%f10  # 0 * SNaN - QNaN
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: fneg %f3,%f10            # QNaN * SNaN - QNaN
   fnmsub %f3,%f10,%f11,%f3
   bl record
   fmr %f4,%f10
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # SNaN * normal - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fnmsub %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # 0 * +infinity - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fnmsub %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # +infinity * normal - +infinity (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   fnmsub %f3,%f9,%f2,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_fpu_noresult
   mtfsb0 24

   # fnmsub (FPSCR[FX] handling)
0: fnmsub %f4,%f1,%f11,%f2
   mtfsb0 0
   fnmsub %f4,%f1,%f11,%f2
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fnmsub (exception persistence)
0: fnmsub %f4,%f1,%f11,%f2
   fnmsub %f4,%f1,%f30,%f1
   bl record
   fmr %f3,%f4
   fneg %f4,%f24
   bl add_fpscr_vxsnan
   bl check_fpu_nnorm

   # fnmsub.
0: fnmsub. %f3,%f1,%f11,%f2  # normal * SNaN - normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0            # normal * SNaN - normal (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fnmsub. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

   # fnmsubs
0: fneg %f13,%f1            # normal * normal - -normal
   fnmsubs %f3,%f2,%f29,%f13
   bl record
   fneg %f4,%f30
   bl check_fpu_nnorm_inex
0: lfd %f4,120(%r31)        # normal * normal - normal
   fnmsubs %f3,%f1,%f4,%f1
   bl record
   lfd %f4,200(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f3,328(%r31)        # huge * normal - normal
   fnmsubs %f3,%f3,%f2,%f7
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfs %f3,12(%r30)         # minimum_normal * minimum_normal - minimum_normal
   fnmsubs %f3,%f3,%f3,%f3
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: mtfsfi 7,1               # 1.333... * 1.5 - -1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fneg %f4,%f4
   fnmsubs %f3,%f3,%f13,%f4
   bl record
   fneg %f4,%f2
   bl check_fpu_nnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... - -1ulp (round toward zero)
   lfd %f3,288(%r31)
   lfd %f13,296(%r31)
   lfd %f4,312(%r31)
   fneg %f4,%f4
   fnmsubs %f3,%f13,%f3,%f4
   bl record
   lis %r3,0xC000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   bl check_fpu_nnorm_inex
   mtfsfi 7,0
0: fnmsubs %f3,%f7,%f2,%f7   # HUGE_VALF * 2 - HUGE_VALF
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm
0: fnmsubs %f3,%f2,%f7,%f7   # 2 * HUGE_VALF - HUGE_VALF
   bl record
   fneg %f4,%f7
   bl check_fpu_nnorm
0: lfd %f13,336(%r31)       # 2^128 * 0.5 - 0
   lfd %f4,64(%r31)
   fnmsubs %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f13,336(%r31)       # 0.5 * 2^128 - 0
   lfd %f4,64(%r31)
   fnmsubs %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   bl check_fpu_nnorm
0: lfd %f4,336(%r31)        # 1 * HUGE_VALF - 2^128
   fnmsubs %f3,%f1,%f7,%f4
   bl record
   lfd %f4,344(%r31)
   bl check_fpu_pnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 1 - HUGE_VAL
   fnmsubs %f4,%f4,%f1,%f4
   bl record
   fmr %f3,%f4
   fneg %f4,%f0
   bl check_fpu_nzero
0: lfd %f4,320(%r31)        # 1 * HUGE_VAL - HUGE_VAL
   fnmsubs %f4,%f1,%f4,%f4
   bl record
   fmr %f3,%f4
   fneg %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_ninf_inex
0: lfd %f3,64(%r31)         # 0.5 * HUGE_VAL + -HUGE_VAL
   lfd %f4,320(%r31)
   fnmsubs %f4,%f1,%f3,%f4
   bl record
   fmr %f3,%f4
   fmr %f4,%f9
   bl add_fpscr_ox
   bl check_fpu_pinf_inex
0: lfd %f13,288(%r31)       # 0 * 1.333... - 1
   fnmsubs %f13,%f0,%f13,%f1
   bl record
   fmr %f3,%f13
   fmr %f4,%f1
   bl check_fpu_pnorm
0: lfd %f4,320(%r31)        # HUGE_VAL * 0 - 1
   fnmsubs %f4,%f4,%f0,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f1
   bl check_fpu_pnorm
0: lfd %f4,320(%r31)        # 0 * HUGE_VAL - 1
   fnmsubs %f4,%f0,%f4,%f1
   bl record
   fmr %f3,%f4
   fmr %f4,%f1
   bl check_fpu_pnorm
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - min_normal_f
   lfs %f3,12(%r30)
   fnmsubs %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   bl check_fpu_pnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - 1
   fnmsubs %f3,%f4,%f4,%f1
   bl record
   fmr %f4,%f1
   bl check_fpu_pnorm_inex
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - inf
   fnmsubs %f3,%f4,%f4,%f9
   bl record
   fmr %f4,%f9
   bl check_fpu_pinf
0: lfd %f4,376(%r31)        # min_denormal_d * min_denormal_d - QNaN
   fnmsubs %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   bl check_fpu_nan

   # fnmsubs.
0: fnmsubs. %f3,%f1,%f11,%f2 # normal * SNaN - normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan

   ########################################################################
   # 4.6.6 Floating-Point Rounding and Conversion Instructions - fcti*
   ########################################################################

   # Swap out some constants.
0: lfd %f24,120(%r31)  # 4194304.75
   fneg %f25,%f24      # -4194304.75
   fmr %f26,%f9        # +infinity
   fneg %f27,%f9       # -infinity
   lfd %f28,208(%r31)  # (double) 2^31
   lfd %f29,216(%r31)  # (double) -(2^31+1)
   lfd %f7,64(%r31)    # 0.5
   fadd %f8,%f7,%f1    # 1.5
   lis %r24,0x40       # 4194304
   addi %r25,%r24,1    # 4194305
   neg %r26,%r24       # -4194304
   neg %r27,%r25       # -4194305
   lis %r29,-0x8000    # -2^31
   not %r28,%r29       # 2^31-1

   # fctiw (RN = round to nearest)
0: fctiw %f3,%f0
   bl record
   li %r7,0
   bl check_fctiw
0: fctiw %f3,%f1
   bl record
   li %r7,1
   bl check_fctiw
0: fneg %f3,%f1
   fctiw %f3,%f3
   bl record
   li %r7,-1
   bl check_fctiw
0: fctiw %f3,%f7
   bl record
   li %r7,0
   bl check_fctiw_inex
0: fctiw %f3,%f8
   bl record
   li %r7,2
   bl check_fctiw_inex
0: fctiw %f3,%f24
   bl record
   mr %r7,%r25
   bl check_fctiw_inex
0: fctiw %f3,%f25
   bl record
   mr %r7,%r27
   bl check_fctiw_inex
0: fctiw %f3,%f26
   bl record
   mr %r7,%r28
   bl add_fpscr_vxcvi
   bl check_fctiw
0: fctiw %f3,%f27
   bl record
   mr %r7,%r29
   bl add_fpscr_vxcvi
   bl check_fctiw
0: fctiw %f3,%f28
   bl record
   mr %r7,%r28
   bl add_fpscr_vxcvi
   bl check_fctiw
0: fneg %f4,%f28
   fctiw %f3,%f4
   bl record
   mr %r7,%r29
   bl check_fctiw
0: fctiw %f3,%f29
   bl record
   mr %r7,%r29
   bl add_fpscr_vxcvi
   bl check_fctiw
0: fctiw %f3,%f10
   bl record
   mr %r7,%r29
   bl add_fpscr_vxcvi
   bl check_fctiw
0: fctiw %f3,%f11
   bl record
   mr %r7,%r29
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl check_fctiw

   # fctiw (RN = round toward zero)
0: mtfsfi 7,1
   fctiw %f3,%f24
   bl record
   mr %r7,%r24
   bl check_fctiw_inex
0: fctiw %f3,%f25
   bl record
   mr %r7,%r26
   bl check_fctiw_inex

   # fctiw (RN = round toward +infinity)
0: mtfsfi 7,2
   fctiw %f3,%f24
   bl record
   mr %r7,%r25
   bl check_fctiw_inex
0: fctiw %f3,%f25
   bl record
   mr %r7,%r26
   bl check_fctiw_inex

   # fctiw (RN = round toward -infinity)
0: mtfsfi 7,3
   fctiw %f3,%f24
   bl record
   mr %r7,%r24
   bl check_fctiw_inex
0: fctiw %f3,%f25
   bl record
   mr %r7,%r27
   bl check_fctiw_inex

   # fctiw (exceptions enabled)
0: mtfsfi 7,0
   mtfsb1 24
   fmr %f3,%f1
   fmr %f4,%f1
   fctiw %f4,%f26
   bl record
   bl add_fpscr_fex
   bl add_fpscr_vxcvi
   bl check_fpu_noresult_nofprf
0: fctiw %f4,%f28
   bl record
   bl add_fpscr_fex
   bl add_fpscr_vxcvi
   bl check_fpu_noresult_nofprf
0: fctiw %f4,%f10
   bl record
   bl add_fpscr_fex
   bl add_fpscr_vxcvi
   bl check_fpu_noresult_nofprf
0: fctiw %f4,%f11
   bl record
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl check_fpu_noresult_nofprf
   mtfsb0 24

   # fctiw (FPSCR[FX] handling)
0: fmr %f3,%f11
   fctiw %f4,%f3
   mtfsb0 0
   fctiw %f4,%f3
   bl record
   fmr %f3,%f4
   mr %r7,%r29
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl remove_fpscr_fx
   bl check_fctiw

   # fctiw (exception persistence)
0: fmr %f3,%f11
   fctiw %f4,%f3
   fmr %f4,%f1
   fctiw %f4,%f4
   bl record
   fmr %f3,%f4
   li %r7,1
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl check_fctiw

.if TEST_UNDOCUMENTED
   # fctiw (nonzero frA)
0: .int 0xFC6B001C  # fctiw %f3,%f0,frA=%f11
   bl record
   li %r7,0
   bl check_fctiw
.endif

   # fctiw.
0: fctiw. %f3,%f11  # SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: mr %r7,%r29
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl check_fctiw
0: mtfsf 255,%f0    # SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fctiw. %f3,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl check_fpu_noresult_nofprf
   mtfsb0 24

   # fctiwz
0: fctiwz %f3,%f24
   bl record
   mr %r7,%r24
   bl check_fctiw_inex
0: fctiwz %f3,%f25
   bl record
   mr %r7,%r26
   bl check_fctiw_inex

.if TEST_UNDOCUMENTED
   # fctiwz (nonzero frA)
0: .int 0xFC6B001E  # fctiwz %f3,%f0,frA=%f11
   bl record
   li %r7,0
   bl check_fctiw
.endif

   # fctiwz.
0: fctiwz. %f3,%f11
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: mr %r7,%r29
   bl add_fpscr_vxsnan
   bl add_fpscr_vxcvi
   bl check_fctiw

   ########################################################################
   # 5.1.1 (Optional) Move To/From System Register Instructions
   ########################################################################

   # The 750CL is not documented as supporting the mfocrf and mtocrf
   # instructions, but it supports them, treating them as mfcr and mtcrf
   # respectively.

   # mfocrf
0: li %r3,0
   mtcr %r3
   cmpwi cr7,%r3,0
   cmpwi cr6,%r3,1
   mfocrf %r3,1     # The mask field is ignored.
   bl record
   cmpwi %r3,0x82
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # mtocrf
0: lis %r3,0x4000
   mtocrf 128,%r3
   bl record
   bgt 0f
   addi %r6,%r6,32
0: lis %r3,0x0200
   mtocrf 64,%r3
   bl record
   bgt 1f
   addi %r6,%r6,32
1: beq cr1,0f
   addi %r6,%r6,32

0: li 0,%r0
   mtcr %r0

   ########################################################################
   # 5.2.1 (Optional) Floating-Point Arithmetic Instructions
   ########################################################################

   # Load constants.
0: lfd %f24,80(%r31)    # 4194304.0
   lfd %f25,176(%r31)   # 2048.0
   lfd %f28,64(%r31)    # 0.5
   lfd %f29,56(%r31)    # 0.25

   # The 750CL does not support the fsqrt or fsqrts instructions.

   # fres
0: fres %f3,%f2         # +normal
   bl record
   # The 750CL documentation says that the result is "correct within
   # one part in 4096", so check that it falls within the proper bounds.
   lfd %f4,256(%r31)
   lfd %f5,264(%r31)
   bl check_fpu_estimate_pnorm
0: fneg %f3,%f2         # -normal
   fres %f4,%f3
   bl record
   fmr %f3,%f4
   lfd %f4,264(%r31)
   fneg %f4,%f4
   lfd %f5,256(%r31)
   fneg %f5,%f5
   bl check_fpu_estimate_nnorm
0: lfs %f13,44(%r30)    # +tiny
   fres %f3,%f13
   bl record
   lfd %f4,152(%r31)
   bl add_fpscr_ox
   # The 750CL sets FPSCR[FI] but not FPSCR[XX] on inexact results.
   bl add_fpscr_fi
   bl check_fpu_pnorm
0: lfs %f13,48(%r30)    # -tiny
   fres %f4,%f13
   bl record
   fmr %f3,%f4
   lfd %f4,152(%r31)
   fneg %f4,%f4
   bl add_fpscr_ox
   bl add_fpscr_fi
   bl check_fpu_nnorm
0: lfd %f14,152(%r31)   # +huge
   fres %f3,%f14
   bl record
   lfs %f4,56(%r30)
   lfs %f5,60(%r30)
   bl add_fpscr_ux
   bl check_fpu_estimate_pdenorm
0: lfd %f14,152(%r31)   # -huge
   fneg %f14,%f14
   fres %f4,%f14
   bl record
   fmr %f3,%f4
   lfs %f4,60(%r30)
   fneg %f4,%f4
   lfs %f5,56(%r30)
   fneg %f5,%f5
   bl add_fpscr_ux
   bl check_fpu_estimate_ndenorm
0: fres %f3,%f26        # +infinity
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: fres %f3,%f27        # -infinity
   bl record
   fneg %f4,%f0
   bl check_fpu_nzero
0: fres %f3,%f0         # +0
   bl record
   fmr %f4,%f9
   bl add_fpscr_zx
   bl check_fpu_pinf
0: fneg %f3,%f0         # -0
   fres %f3,%f3
   bl record
   fneg %f4,%f9
   bl add_fpscr_zx
   bl check_fpu_ninf
0: fres %f3,%f10        # QNaN
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: fres %f3,%f11        # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   fmr %f5,%f11
   fmr %f3,%f24
   fres %f3,%f5
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsb1 27            # 0 (exception enabled)
   fmr %f3,%f24
   fmr %f4,%f0
   fres %f3,%f4
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_zx
   bl check_fpu_noresult
   mtfsb0 27

   # fres (FPSCR[FX] handling)
0: fres %f4,%f11
   mtfsb0 0
   fres %f4,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # fres (exception persistence)
0: fres %f4,%f11
   fres %f4,%f2
   bl record
   fmr %f3,%f4
   lfd %f4,256(%r31)
   lfd %f5,264(%r31)
   bl add_fpscr_vxsnan
   bl check_fpu_estimate_pnorm

.if TEST_UNDOCUMENTED
   # fres (nonzero frA and frC)
0: .int 0xEC6B12F0      # fres %f3,%f2,frA=%f11,frC=%f11
   bl record
   lfd %f4,256(%r31)
   lfd %f5,264(%r31)
   bl check_fpu_estimate_pnorm
.endif

   # fres.
0: fres. %f3,%f11       # SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   fres. %f3,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

.if TEST_RECIPROCAL_TABLES

   # fres (precise output: basic tests)
0: bl 1f
1: mflr %r3
   addi %r14,%r3,2f-1b
   addi %r15,%r3,0f-1b
   b 0f
   # 12 bytes per entry: input, output, expected FPSCR
2: .int 0x40000000,0x3EFFF800,0x00004000
   .int 0xC0000000,0xBEFFF800,0x00008000
   .int 0x7F7FFFFF,0x00200020,0x88034000
   .int 0xFF7FFFFF,0x80200020,0x88038000
   .int 0x00000001,0x7F7FFFFF,0x90024000
   .int 0x80000001,0xFF7FFFFF,0x90028000
   .int 0x001FFFFF,0x7F7FFFFF,0x90024000
   .int 0x801FFFFF,0xFF7FFFFF,0x90028000
   .int 0x00200000,0x7F7FF800,0x00004000
   .int 0x80200000,0xFF7FF800,0x00008000
0: lfs %f3,0(%r14)
   mtfsf 255,%f0
   fres %f5,%f3
   bl record
   mffs %f4
   stfd %f4,0(%r4)
   stfs %f5,0(%r4)
   lwz %r3,0(%r4)
   lwz %r7,4(%r14)
   lwz %r8,4(%r4)
   lwz %r9,8(%r14)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   beq 2f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r8,16(%r6)
   lwz %r3,0(%r14)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: addi %r14,%r14,12
   cmpw %r14,%r15
   blt 0b

   # fres (precise output: all base and delta values)
0: lis %r14,0x4000
   lis %r15,0x4080
   lis %r12,0x0001
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f4,%f3
   fres %f5,%f4
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,8(%r4)
   lwz %r7,12(%r4)
   bne 1f
   cmpw %r7,%r11
   beq 2f
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # fres (precise output: input precision and FI)
0: lis %r14,0x4000
   ori %r15,%r14,0x200
   li %r12,1
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f5,%f3
   fres %f5,%f5
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,8(%r4)
   lwz %r7,12(%r4)
   bne 1f
   cmpw %r7,%r11
   beq 2f
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # fres (precise output: denormal inputs)
0: lis %r14,0x0010
   lis %r15,0x00A0
   lis %r12,0x10
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f13,%f3
   fres %f5,%f13
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,8(%r4)
   lwz %r7,12(%r4)
   bne 1f
   cmpw %r7,%r11
   beq 2f
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # fres (precise output: input precision for denormals)
0: lis %r14,0x003F
   ori %r14,%r14,0xFE00
   lis %r15,0x0040
   ori %r15,%r15,0x0200
   li %r12,0x20
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f14,%f3
   fres %f5,%f14
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,8(%r4)
   lwz %r7,12(%r4)
   bne 1f
   cmpw %r7,%r11
   beq 2f
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # fres (precise output: denormal outputs)
0: lis %r14,0x7E00
   lis %r15,0x7F80
   lis %r12,0x10
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f15,%f3
   fres %f5,%f15
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,8(%r4)
   lwz %r7,12(%r4)
   bne 1f
   cmpw %r7,%r11
   beq 2f
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

.endif  # TEST_RECIPROCAL_TABLES

   # frsqrte
0: frsqrte %f3,%f29     # +normal
   bl record
   lfd %f4,272(%r31)
   lfd %f5,280(%r31)
   bl check_fpu_estimate_pnorm
0: frsqrte %f3,%f26     # +infinity
   bl record
   fmr %f4,%f0
   bl check_fpu_pzero
0: frsqrte %f3,%f0      # +0
   bl record
   fmr %f4,%f9
   bl add_fpscr_zx
   bl check_fpu_pinf
0: fneg %f3,%f0
   frsqrte %f3,%f3      # -0
   bl record
   fneg %f4,%f9
   bl add_fpscr_zx
   bl check_fpu_ninf
0: fneg %f6,%f29        # -normal
   frsqrte %f3,%f6
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxsqrt
   bl check_fpu_nan
0: frsqrte %f3,%f10     # QNaN
   bl record
   fmr %f4,%f10
   bl check_fpu_nan
0: frsqrte %f3,%f11     # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   fmr %f5,%f11
   fmr %f3,%f24
   frsqrte %f3,%f5
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # -infinity (exception enabled)
   mtfsb1 24
   fmr %f3,%f24
   frsqrte %f3,%f27
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsqrt
   bl check_fpu_noresult
   mtfsb0 24
0: mtfsb1 27            # -0 (exception enabled)
   fmr %f3,%f24
   fneg %f4,%f0
   frsqrte %f3,%f4
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_zx
   bl check_fpu_noresult
   mtfsb0 27

   # frsqrte (FPSCR[FX] handling)
0: frsqrte %f4,%f11
   mtfsb0 0
   frsqrte %f4,%f11
   bl record
   fmr %f3,%f4
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_fpu_nan

   # frsqrte (exception persistence)
0: frsqrte %f4,%f11
   frsqrte %f4,%f29
   bl record
   fmr %f3,%f4
   lfd %f4,272(%r31)
   lfd %f5,280(%r31)
   bl add_fpscr_vxsnan
   bl check_fpu_estimate_pnorm

.if TEST_UNDOCUMENTED
   # frsqrte (nonzero frA and frC)
0: .int 0xFC6BEAF4      # frsqrte %f3,%f29,frA=%f11,frC=%f11
   bl record
   lfd %f4,272(%r31)
   lfd %f5,280(%r31)
   bl check_fpu_estimate_pnorm
.endif

   # frsqrte.
0: frsqrte. %f3,%f11    # SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl check_fpu_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   fmr %f3,%f11
   frsqrte. %f3,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_fpu_noresult
   mtfsb0 24

.if TEST_RECIPROCAL_TABLES

   # On failure, these tests store the expected (double-precision) output
   # value in words 6 and 7 of the failure record.  Word 5 receives the
   # high 32 bits of the input value, except for the denormal input
   # precision tests which store the low 32 bits (since the high 32 bits
   # are always zero in that case).

   # frsqrte (precise output: basic tests)
0: bl 1f
1: mflr %r3
   addi %r14,%r3,2f-1b
   addi %r15,%r3,0f-1b
   b 0f
   # 24 bytes per entry: input, output, expected FPSCR, padding
2: .int 0x40000000,0x00000000, 0x3FE69FA0,0x00000000, 0x00004000, 0
   .int 0x40100000,0x00000000, 0x3FDFFE80,0x00000000, 0x00004000, 0
   .int 0x7FEFFFFF,0xFFFFFFFF, 0x1FF00008,0x2C000000, 0x00004000, 0
   .int 0x00000000,0x00000001, 0x617FFE80,0x00000000, 0x00004000, 0
0: lfd %f3,0(%r14)
   mtfsf 255,%f0
   frsqrte %f5,%f3
   bl record
   mffs %f4
   stfd %f4,0(%r4)
   stfd %f5,8(%r4)
   lwz %r3,8(%r4)
   lwz %r7,8(%r14)
   lwz %r8,12(%r4)
   lwz %r9,12(%r14)
   lwz %r10,4(%r4)
   lwz %r11,16(%r14)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   bne 1f
   cmpw %r10,%r11
   beq 2f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r14)
   stw %r3,20(%r6)
   stw %r7,24(%r6)
   stw %r9,28(%r6)
   addi %r6,%r6,32
2: addi %r14,%r14,24
   cmpw %r14,%r15
   blt 0b

   # frsqrte (precise output: all base and delta values)
0: lis %r14,0x4010
   lis %r15,0x4030
   li %r12,0x4000
0: stw %r14,0(%r4)
   stw %r0,4(%r4)
   lfd %f3,0(%r4)
   bl calc_frsqrte
   stfd %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f4,%f3
   frsqrte %f5,%f4
   bl record
   stfd %f5,16(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   lwz %r8,20(%r4)
   lwz %r9,12(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   bne 1f
   cmpw %r10,%r11
   beq 2f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   stw %r7,24(%r6)
   stw %r9,28(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # frsqrte (precise output: input precision)
0: lis %r14,0x401F
   ori %r14,%r14,0xF000
   lis %r15,0x4020
   ori %r14,%r14,0x1000
   li %r12,0x10
0: stw %r14,0(%r4)
   stw %r0,4(%r4)
   lfd %f3,0(%r4)
   bl calc_frsqrte
   stfd %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f5,%f3
   frsqrte %f5,%f5
   bl record
   stfd %f5,16(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   lwz %r8,20(%r4)
   lwz %r9,12(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   bne 1f
   cmpw %r10,%r11
   beq 2f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   stw %r7,24(%r6)
   stw %r9,28(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # frsqrte (precise output: denormal inputs)
0: lis %r14,0x0002
   lis %r15,0x0014
   lis %r12,2
0: stw %r14,0(%r4)
   stw %r0,4(%r4)
   lfd %f3,0(%r4)
   bl calc_frsqrte
   stfd %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f13,%f3
   frsqrte %f5,%f13
   bl record
   stfd %f5,16(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   lwz %r8,20(%r4)
   lwz %r9,12(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   bne 1f
   cmpw %r10,%r11
   beq 2f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   stw %r7,24(%r6)
   stw %r9,28(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # frsqrte (precise output: input precision for denormals)
0: lis %r14,0x001F
   ori %r14,%r14,0xFE00
   lis %r15,0x0020
   ori %r15,%r15,0x0200
   li %r12,0x10
0: stw %r0,0(%r4)
   stw %r14,4(%r4)
   lfd %f3,0(%r4)
   bl calc_frsqrte
   stfd %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   fmr %f14,%f3
   frsqrte %f5,%f14
   bl record
   stfd %f5,16(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   lwz %r8,20(%r4)
   lwz %r9,12(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   bne 1f
   cmpw %r10,%r11
   beq 2f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,4(%r4)
   stw %r3,20(%r6)
   stw %r7,24(%r6)
   stw %r9,28(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

.endif  # TEST_RECIPROCAL_TABLES

   ########################################################################
   # 5.2.2 (Optional) Floating-Point Select Instruction
   ########################################################################

   # fsel
0: fsel %f3,%f2,%f1,%f0     # +normal
   bl record
   fcmpu cr0,%f3,%f1
   beq 0f
   stfd %f3,8(%r6)
   addi %r6,%r6,32
0: fsel %f3,%f0,%f2,%f0     # +0
   bl record
   fcmpu cr0,%f3,%f2
   beq 0f
   stfd %f3,8(%r6)
   addi %r6,%r6,32
0: fneg %f3,%f0             # -0
   fsel %f3,%f3,%f1,%f0
   bl record
   fcmpu cr0,%f3,%f1
   beq 0f
   stfd %f3,8(%r6)
   addi %r6,%r6,32
0: fneg %f4,%f1             # -normal
   fsel %f3,%f4,%f0,%f2
   bl record
   fcmpu cr0,%f3,%f2
   beq 0f
   stfd %f3,8(%r6)
   addi %r6,%r6,32
0: fsel %f3,%f10,%f0,%f1    # QNaN
   bl record
   fcmpu cr0,%f3,%f1
   beq 0f
   stfd %f3,8(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0            # SNaN
   fsel %f3,%f11,%f0,%f2
   bl record
   mffs %f4                 # The SNaN should not generate an exception.
   stfd %f4,0(%r4)
   lwz %r3,4(%r4)
   fadd %f4,%f0,%f0         # Make sure the SNaN doesn't leak an exception
   mffs %f4                 # to the next FPU operation either.
   stfd %f4,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0
   bne 1f
   cmpwi %r7,0x2000
   beq 2f
1: stfd %f3,8(%r6)
   stw %r3,16(%r6)
   stw %r7,20(%r6)
   addi %r6,%r6,32
   b 0f
2: fcmpu cr0,%f3,%f2
   beq 0f
   stfd %f3,8(%r6)
   addi %r6,%r6,32

   # fsel.
0: li %r0,0
   mtcr %r0
   fadd %f3,%f11,%f11       # Record an exception for fsel. to pick up.
   fsel. %f3,%f2,%f1,%f0
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   bne 1f
   fcmpu cr0,%f3,%f1
   beq 0f
1: stfd %f3,8(%r6)
   stw %r3,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # 6.1 (Obsolete) Move To Condition Register from XER
   ########################################################################

   # This instruction is listed as "phased out of the architecture" in the
   # v2.01 UISA, but it was presumably still part of the UISA as of v1.10,
   # and the 750CL supports it.

   # mcrxr
0: li %r0,0
   mtcr %r0
   lis %r3,0x2000
   addi %r3,%r3,0x11
   mtxer %r3
   bl record
   mcrxr cr7
   mfcr %r3
   mfxer %r7
   cmpwi %r3,2
   bne 1f
   cmpwi %r7,0x11
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # Book II 3.2.1 Instruction Cache Instruction
   ########################################################################

   # This and many of the following instructions have no visible effect on
   # program state; they are included only to ensure that instruction
   # decoders properly accept the instructions.

   # Load r11 with the address of label 2.
0: bl 1f
2: b 0f
1: mflr %r11
   blr

   # icbi
0: li %r3,0f-2b
   icbi %r11,%r3
0: addi %r11,%r11,0f-2b
   lis %r0,0x8000
   icbi 0,%r11

   ########################################################################
   # Book II 3.2.2 Data Cache Instructions
   ########################################################################

   # dcbt
0: dcbt %r4,%r3
0: dcbt 0,%r4

   # dcbtst
0: dcbtst %r4,%r3
0: dcbtst 0,%r4

   # dcbz
0: li %r7,1
   stw %r7,0(%r4)
   li %r7,0
   dcbz %r4,%r7
   bl record
   lwz %r7,0(%r4)
   cmpwi %r7,0
   beq 0f
   stw %r7,8(%r6)
   addi %r6,%r6,32
0: li %r7,1
   stw %r7,0(%r4)
   li %r0,0x4000    # Assume this is at least as large as a cache line.
   dcbz 0,%r4
   bl record
   lwz %r7,0(%r4)
   cmpwi %r7,0
   beq 0f
   stw %r7,8(%r6)
   addi %r6,%r6,32

   # dcbst
0: dcbst %r4,%r3
0: dcbst 0,%r4

   # dcbf
0: li %r7,0x3FFC  # Careful not to flush anything we actually need!
   dcbf %r4,%r7
0: add %r7,%r4,%r7
   li %r0,-0x3FFC
   dcbf 0,%r4

   ########################################################################
   # Book II 3.3.1 Instruction Synchronize Instruction
   ########################################################################

   # isync
0: isync

   ########################################################################
   # Book II 3.3.2 Load and Reserve and Store Conditional Instructions
   ########################################################################

0: li %r11,-1
   stw %r11,0(%r4)
   stw %r11,4(%r4)
   stw %r11,8(%r4)
   stw %r11,12(%r4)
   stw %r11,16(%r4)

   # lwarx/stwcx.
0: li %r0,0
   stw %r0,4(%r4)
   li %r7,4
   lwarx %r3,%r4,%r7  # rA != 0
   bl record
   cmpwi %r3,0
   beq 0f
   addi %r6,%r6,32
0: li %r3,1
   stwcx. %r3,%r4,%r7
   bl record
   li %r3,-1
   bne 1f
   lwz %r3,4(%r4)
   cmpwi %r3,1
   beq 0f
1: stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r8,2
   stwcx. %r8,%r4,%r7  # No reservation, so this will fail.
   bl record
   li %r3,-1
   beq 1f
   lwz %r3,4(%r4)
   cmpwi %r3,1
   beq 0f
1: stw %r3,8(%r6)
   addi %r6,%r6,32

0: stw %r0,4(%r4)
   addi %r7,%r4,4
   li %r0,4
   lwarx %r3,0,%r7  # rA == 0
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,1
   stwcx. %r3,0,%r7
   bl record
   li %r3,-1
   bne 1f
   lwz %r3,4(%r4)
   cmpwi %r3,1
   beq 0f
1: stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r8,2
   stwcx. %r8,0,%r7
   bl record
   li %r3,-1
   beq 1f
   lwz %r3,4(%r4)
   cmpwi %r3,1
   beq 0f
1: stw %r3,8(%r6)
   addi %r6,%r6,32

   # Check that stwcx. to the wrong address still succeeds.
0: li %r0,0
   stw %r0,4(%r4)
   li %r7,4
   lwarx %r3,%r4,%r7
   bl record
   cmpwi %r3,0
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: li %r3,1
   stwcx. %r3,0,%r4
   bl record
   li %r3,-1
   li %r7,-1
   bne 1f
   lwz %r3,0(%r4)
   cmpwi %r3,1
   li %r7,0
   bne 1f
   lwz %r3,4(%r4)   # Also make sure nothing was written to the lwarx address.
   cmpwi %r3,0
   li %r7,4
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   addi %r6,%r6,32

   ########################################################################
   # Book II 3.3.3 Memory Barrier Instructions
   ########################################################################

   # sync
0: sync
   # The 750CL does not support lwsync and ptesync.

   # eieio
0: eieio

   ########################################################################
   # Book II 4.1 Time Base Instructions
   ########################################################################

   # mftb
   # Some versions of binutils-as incorrectly encode this as mfspr, so
   # code the instruction directly.
0: .int 0x7C6C42E6  # mftb %r3
   li %r0,MFTB_SPIN_COUNT
   mtctr %r0
1: bdnz 1b
   .int 0x7CEC42E6  # mftb %r7
   bl record
   sub. %r3,%r7,%r3
   bgt 0f
   stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # mftbu
0: .int 0x7D4C42E6  # mftb %r10
   .int 0x7C6D42E6  # mftbu %r3
   li %r0,MFTB_SPIN_COUNT
   mtctr %r0
1: bdnz 1b
   .int 0x7D6C42E6  # mftb %r11
   .int 0x7CED42E6  # mftbu %r7
   bl record
   sub %r3,%r7,%r3
   cmpwi %r3,0
   beq 0f
   cmpwi %r3,1      # The low 32 bits might have rolled over.
   beq 0f
   stw %r3,8(%r6)
   stw %r10,12(%r6)
   stw %r7,16(%r6)
   stw %r11,20(%r6)
   addi %r6,%r6,32

   ########################################################################
   # PowerPC 750CL Manual: Paired single instructions
   ########################################################################

.if TEST_PAIRED_SINGLE

   # The 750CL manual documents paired-single instructions as operating on
   # two 32-bit floating point values overlaid on a single 64-bit FPR.
   # The reality is a bit more complicated: the 750CL seems to track
   # whether each FPR is in double-precision or paired-single mode and
   # silently convert values between the two modes as needed.  This is
   # particularly problematic for testing because the stfd instruction
   # (which normally stores an FPR's bit pattern directly to memory)
   # causes a switch out of paired-single mode, and there are no
   # equivalent instructions for paired-single mode (psq_st flushes
   # denormals to zero, so we don't get an exact view of the register),
   # so we have no way to see exactly what's going on.
   #
   # Based on experimentation, the following appears to describe the
   # behavior of paired-single instructions on the 750CL:
   #
   # - Each FPR has an associated 32-bit "shadow" register used to hold
   #   the second slot of a paired-single value.
   #
   # - Instructions which read an FPR in paired-single mode use the 64-bit
   #   double-precision value of the primary register as the value of the
   #   first slot (ps0), and the 32-bit single-precision value of the
   #   shadow register as the second slot (ps1).
   #
   # - Instructions which write an FPR in paired-single mode write the
   #   single-precision value of ps0 to the primary register, extending it
   #   to 64 bits as the lfs instruction would do.
   #
   # To be fair, the manual states that "[i]t is a programming error to
   # apply double-precision instructions to paired-single operands and
   # vice versa"; it may be that IBM considers paired-single mode to be
   # something of a kludge, especially given that the paired-single
   # instruction set was abandoned in favor of full-fledged vector
   # instructions in later versions of the PowerPC ISA.  Nonetheless, we
   # test here for the actual behavior of the 750CL in case of code in
   # the wild which makes use of this behavior.

   # Load constants.  (This also serves as an indirect test that lfs
   # loads its value into both slots of a paired-single register.)
0: li %r0,0
   lfs %f0,0(%r30)     # (0.0f,0.0f)
   lfs %f1,4(%r30)     # (1.0f,1.0f)
   fadd %f3,%f1,%f1
   fadd %f4,%f3,%f1
   stfs %f3,0(%r4)
   stfs %f4,4(%r4)
   lfs %f2,0(%r4)      # (2.0f,2.0f)
   lfs %f31,4(%r4)     # (3.0f,3.0f)
   lfs %f6,20(%r30)    # (QNaN,QNaN)
   lfs %f7,24(%r30)    # (SNaN,SNaN)

   ########################################################################
   # ps_merge{00,01,10,11}
   ########################################################################

0: stw %r0,0(%r4)
   lis %r11,0x8000
   stw %r11,4(%r4)
   lfd %f3,0(%r4)
   mtfsf 255,%f3

   # ps_merge00
0: ps_merge00 %f24,%f0,%f1  # (0.0f,1.0f)
   bl record
   stfs %f24,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3         # Also check that FPSCR is unmodified.
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   addi %r6,%r6,32
0: ps_merge00 %f25,%f2,%f31  # (2.0f,3.0f)
   bl record
   stfs %f25,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   lis %r7,0x4000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
   stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   addi %r6,%r6,32

   # ps_merge00.
0: mtcr %r0
   ps_merge00. %f24,%f0,%f1  # (0.0f,1.0f)
   bl record
   stfs %f24,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_merge11
0: ps_merge11 %f3,%f24,%f25
   bl record
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x3F80
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3  # Copy slot 1 into slot 0 so we can read it out.
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: addi %r6,%r6,32

   # ps_merge11.
0: mtcr %r0
   ps_merge11. %f3,%f24,%f25
   bl record
   mfcr %r9
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x3F80
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: addi %r6,%r6,32

   # ps_merge01
0: ps_merge01 %f3,%f24,%f25
   bl record
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   cmpwi %r3,0
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: addi %r6,%r6,32

   # ps_merge01.
0: mtcr %r0
   ps_merge01. %f3,%f24,%f25
   bl record
   mfcr %r9
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   cmpwi %r3,0
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: addi %r6,%r6,32

   # ps_merge10
0: ps_merge10 %f3,%f24,%f25
   bl record
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x3F80
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   lis %r7,0x4000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: addi %r6,%r6,32

   # ps_merge10.
0: mtcr %r0
   ps_merge10. %f3,%f24,%f25
   bl record
   mfcr %r9
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   lis %r7,0x3F80
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   lis %r7,0x4000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: addi %r6,%r6,32

   # Make sure merging doesn't silence SNaNs.
0: ps_merge01 %f3,%f7,%f7
   bl record
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f4
   stfd %f4,8(%r4)
   lwz %r8,12(%r4)
   lis %r7,0xFFA0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: addi %r6,%r6,32

   ########################################################################
   # lfs (denormal value to ps1)
   ########################################################################

0: lfs %f3,44(%r30)  # +tiny
   bl record
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   lwz %r7,44(%r30)
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: addi %r6,%r6,32
0: lfs %f3,48(%r30)  # -tiny
   bl record
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   lwz %r7,48(%r30)
   cmpw %r3,%r7
   stw %r3,8(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,0(%r4)
   lwz %r3,0(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   stw %r8,16(%r6)
   stw %r11,20(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   beq 0f
1: addi %r6,%r6,32

   ########################################################################
   # ps_cmpu{0,1} equality tests (for the check_ps subroutine)
   ########################################################################

   # ps_cmpu0 (equality)
0: ps_cmpu0 cr0,%f24,%f0
   bl record
   beq 0f
   mfcr %r3
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: ps_cmpu0 cr0,%f24,%f1
   bl record
   bne 0f
   mfcr %r3
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # ps_cmpu1 (equality)
0: ps_cmpu1 cr0,%f24,%f0
   bl record
   bne 0f
   mfcr %r3
   stw %r3,8(%r6)
   addi %r6,%r6,32
0: ps_cmpu1 cr0,%f24,%f1
   bl record
   beq 0f
   mfcr %r3
   stw %r3,8(%r6)
   addi %r6,%r6,32

   ########################################################################
   # Paired-single load and store instructions (floating-point data)
   ########################################################################

.if ESPRESSO
   mtspr 896,%r0    # UGQR0
.elseif CAN_SET_GQR
   mtspr 912,%r0    # GQR0
.else
   # If we can't set GQRs, we assume that GQR0 has a value of zero.
.endif

0: lis %r10,0x4000  # 2.0f
   stw %r10,0(%r4)
   lis %r11,0x4040  # 3.0f
   stw %r11,4(%r4)

   # psq_l
0: psq_l %f3,0(%r4),0,0
   bl record
   fmr %f4,%f2
   fadd %f5,%f2,%f1
   bl check_ps
0: psq_l %f3,4(%r4),1,0
   bl record
   fadd %r4,%r2,%r1
   fmr %f5,%f1
   bl check_ps

   # psq_lx
0: addi %r7,%r4,4
   li %r8,-4
   psq_lx %f3,%r7,%r8,0,0
   bl record
   fmr %f4,%f2
   fadd %f5,%f2,%f1
   bl check_ps
0: li %r0,4  # Should be ignored (0 means "zero" and not "r0").
   psq_lx %f3,0,%r7,1,0
   bl record
   fadd %r4,%r2,%r1
   fmr %f5,%f1
   bl check_ps

   # psq_lu
0: addi %r7,%r4,-4
   psq_lu %f3,4(%r7),0,0
   bl record
   cmpw %r7,%r4
   beq 1f
   li %r3,-1
   stw %r3,8(%r6)
   stw %r7,12(%r6)
   b 0f
1: fmr %f4,%f2
   fadd %f5,%f2,%f1
   bl check_ps
0: psq_lu %f3,4(%r7),1,0
   bl record
   addi %r3,%r4,4
   cmpw %r7,%r3
   beq 1f
   li %r3,-1
   stw %r3,8(%r6)
   stw %r7,12(%r6)
   b 0f
1: fadd %r4,%r2,%r1
   fmr %f5,%f1
   bl check_ps

   # psq_lux
0: addi %r7,%r4,4
   li %r8,-4
   psq_lux %f3,%r7,%r8,0,0
   bl record
   cmpw %r7,%r4
   beq 1f
   li %r3,-1
   stw %r3,8(%r6)
   stw %r7,12(%r6)
   b 0f
1: fmr %f4,%f2
   fadd %f5,%f2,%f1
   bl check_ps
0: li %r0,4
   psq_lux %f3,%r7,%r0,1,0
   bl record
   addi %r3,%r4,4
   cmpw %r7,%r3
   beq 1f
   li %r3,-1
   stw %r3,8(%r6)
   stw %r7,12(%r6)
   b 0f
1: fadd %r4,%r2,%r1
   fmr %f5,%f1
   bl check_ps

   # psq_st
0: li %r12,-1
   stw %r12,0(%r4)
   stw %r12,4(%r4)
   stw %r12,8(%r4)
   psq_st %f25,0(%r4),0,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r11
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   addi %r6,%r6,32
0: stw %r12,0(%r4)
   stw %r12,4(%r4)
   psq_st %f25,4(%r4),1,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r12
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   addi %r6,%r6,32

   # psq_stx
0: stw %r12,0(%r4)
   stw %r12,4(%r4)
   stw %r12,8(%r4)
   addi %r7,%r4,4
   li %r8,-4
   psq_stx %f25,%r7,%r8,0,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r11
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   addi %r6,%r6,32
0: stw %r12,0(%r4)
   stw %r12,4(%r4)
   li %r0,4  # Should be ignored (0 means "zero" and not "r0").
   psq_stx %f25,0,%r7,1,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r12
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   addi %r6,%r6,32

   # psq_stu
0: li %r12,-1
   stw %r12,0(%r4)
   stw %r12,4(%r4)
   stw %r12,8(%r4)
   addi %r7,%r4,-4
   psq_stu %f25,4(%r7),0,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r11
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   stw %r7,20(%r6)
   addi %r6,%r6,32
0: stw %r12,0(%r4)
   stw %r12,4(%r4)
   psq_stu %f25,4(%r7),1,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r12
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   bne 1f
   addi %r3,%r4,4
   cmpw %r7,%r3
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   stw %r7,20(%r6)
   addi %r6,%r6,32

   # psq_stux
0: stw %r12,0(%r4)
   stw %r12,4(%r4)
   stw %r12,8(%r4)
   addi %r7,%r4,4
   li %r8,-4
   psq_stux %f25,%r7,%r8,0,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r11
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   bne 1f
   cmpw %r7,%r4
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   stw %r7,20(%r6)
   addi %r6,%r6,32
0: stw %r12,0(%r4)
   stw %r12,4(%r4)
   li %r0,4
   psq_stux %f25,%r7,%r0,1,0
   bl record
   lwz %r3,0(%r4)
   cmpw %r3,%r12
   bne 1f
   lwz %r3,4(%r4)
   cmpw %r3,%r10
   bne 1f
   lwz %r3,8(%r4)
   cmpw %r3,%r12
   bne 1f
   addi %r3,%r4,4
   cmpw %r7,%r3
   beq 0f
1: lwz %r3,0(%r4)
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   lwz %r3,8(%r4)
   stw %r3,16(%r6)
   stw %r7,20(%r6)
   addi %r6,%r6,32

   # Loads and stores should not generate exceptions or silence SNaNs.
0: mtfsf 255,%f0
   psq_l %f3,20(%r30),0,0  # SNaN in ps1
   bl record
   psq_st %r3,0(%r4),0,0
   lwz %r3,0(%r4)
   lis %r7,0x7FD0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r8,4(%r4)
   stw %r8,16(%r6)
   bne 1f
   lis %r7,0xFFA0
   cmpw %r3,%r7
   bne 1f
   cmpwi %r8,0
   beq 0f
1: addi %r6,%r6,32
0: psq_l %f3,24(%r30),1,0  # SNaN in ps0
   bl record
   psq_st %r3,0(%r4),0,0
   lwz %r3,0(%r4)
   lis %r7,0xFFA0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r8,4(%r4)
   stw %r8,16(%r6)
   bne 1f
   lis %r7,0x3F80
   cmpw %r3,%r7
   bne 1f
   cmpwi %r8,0
   beq 0f
1: addi %r6,%r6,32

   # Denormalized values are loaded without change but flushed to zero
   # on store.  These should also not cause any exceptions.
0: li %r3,1
   stw %r3,0(%r4)
   li %r3,2
   stw %r3,4(%r4)
   psq_l %f3,0(%r4),0,0
   bl record
   stfs %f3,0(%r4)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,4(%r4)
   lwz %r3,0(%r4)
   lwz %r7,4(%r4)
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r8,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,2
   bne 1f
   cmpwi %r8,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r3,3
   stw %r3,0(%r4)
   li %r3,4
   stw %r3,4(%r4)
   psq_l %f3,0(%r4),0,0
   psq_st %f3,0(%r4),0,0
   bl record
   lwz %r3,0(%r4)
   lwz %r7,4(%r4)
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r8,4(%r4)
   cmpwi %r3,0
   bne 1f
   cmpwi %r7,0
   bne 1f
   cmpwi %r8,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32

   # Loads and stores should not change the sign of zero.
0: lis %r7,0x8000
   stw %r7,0(%r4)
   lfs %f3,0(%r4)
   psq_st %f3,8(%r4),0,0
   bl record
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   bne 1f
   lwz %r3,12(%r4)
   cmpw %r3,%r7
   beq 0f
1: lwz %r3,8(%r4)
   stw %r3,8(%r6)
   lwz %r3,12(%r4)
   stw %r3,12(%r6)
   addi %r6,%r6,32

   ########################################################################
   # Paired-single load and store instructions (quantized integer data)
   ########################################################################

0: li %r0,0

.if CAN_SET_GQR || ESPRESSO

0: lis %r3,0x7F80
   stw %r3,0(%r4)
   lis %r3,0xFF80
   stw %r3,4(%r4)
   psq_l %f28,0(%r4),0,0
   psq_l %f29,20(%r30),0,0
   lis %r24,0x0123
   ori %r24,%r24,0xFEDC
   lis %r3,0x4391   # 291.0f
   ori %r3,%r3,0x8000
   stw %r3,0(%r4)
   lfs %f6,0(%r4)
   lis %r3,0x477E   # 65244.0f
   ori %r3,%r3,0xDC00
   stw %r3,0(%r4)
   lfs %f7,0(%r4)
   lis %r3,0xC392   # -292.0f
   stw %r3,0(%r4)
   lfs %f8,0(%r4)

   # 8-bit unsigned, no shift
0: lis %r3,0x0004
   .if ESPRESSO
      mtspr 897,%r3    # UGQR1
   .else
      mtspr 913,%r3    # GQR1
   .endif
   li %r3,0x01FE
   sth %r3,0(%r4)
   psq_l %f3,0(%r4),0,1
   bl record
   fmr %f4,%f1
   lis %r3,0x437E   # 254.0f
   stw %r3,0(%r4)
   lfs %f5,0(%r4)
   bl check_ps
0: li %r3,0x0004
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,0(%r4)
   psq_st %f3,0(%r4),0,1
   bl record
   lhz %r3,0(%r4)
   cmpwi %r3,0x01FE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit unsigned, positive shift
0: lis %r3,0x0204
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   li %r3,0x04F8
   sth %r3,4(%r4)
   psq_l %f3,4(%r4),0,1
   bl record
   fmr %f4,%f1
   lis %r3,0x4278   # 62.0f
   stw %r3,0(%r4)
   lfs %f5,0(%r4)
   bl check_ps
0: li %r3,0x0204
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,4(%r4)
   psq_st %f3,4(%r4),0,1
   bl record
   lhz %r3,4(%r4)
   cmpwi %r3,0x04F8
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit unsigned, negative shift
0: lis %r3,0x3F04
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   li %r3,0x01FE
   sth %r3,8(%r4)
   psq_l %f3,8(%r4),0,1
   bl record
   fmr %f4,%f2
   lis %r3,0x43FE   # 508.0f
   stw %r3,0(%r4)
   lfs %f5,0(%r4)
   bl check_ps
0: li %r3,0x3F04
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,8(%r4)
   psq_st %f3,8(%r4),0,1
   bl record
   lhz %r3,8(%r4)
   cmpwi %r3,0x01FE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit unsigned, overflow
0: li %r3,0x0004
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,12(%r4)
   psq_st %f28,12(%r4),0,1
   bl record
   lhz %r3,12(%r4)
   ori %r7,%r0,0xFF00
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit unsigned, NaN
0: li %r3,0x0004
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,16(%r4)
   psq_st %f29,16(%r4),0,1
   bl record
   lhz %r3,16(%r4)
   ori %r7,%r0,0xFF00
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit unsigned, no shift
0: lis %r3,0x0005
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r24,20(%r4)
   psq_l %f3,20(%r4),0,1
   bl record
   fmr %f4,%f6
   fmr %f5,%f7
   bl check_ps
0: li %r3,0x0005
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,20(%r4)
   psq_st %f3,20(%r4),0,1
   bl record
   lwz %r3,20(%r4)
   cmpw %r3,%r24
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit unsigned, positive shift
0: lis %r3,0x0205
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r24,24(%r4)
   psq_l %f3,24(%r4),0,1
   bl record
   lfd %f9,56(%r31)  # 0.25
   fmul %f4,%f6,%f9
   fmul %f5,%f7,%f9
   bl check_ps
0: li %r3,0x0205
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,24(%r4)
   psq_st %f3,24(%r4),0,1
   bl record
   lwz %r3,24(%r4)
   cmpw %r3,%r24
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit unsigned, negative shift
0: lis %r3,0x3F05
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r24,28(%r4)
   psq_l %f3,28(%r4),0,1
   bl record
   fmul %f4,%f6,%f2
   fmul %f5,%f7,%f2
   bl check_ps
0: li %r3,0x3F05
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,28(%r4)
   psq_st %f3,28(%r4),0,1
   bl record
   lwz %r3,28(%r4)
   cmpw %r3,%r24
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit unsigned, overflow
0: li %r3,0x0005
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,32(%r4)
   psq_st %f28,32(%r4),0,1
   bl record
   lwz %r3,32(%r4)
   lis %r7,0xFFFF
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit unsigned, NaN
0: li %r3,0x0005
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,36(%r4)
   psq_st %f29,36(%r4),0,1
   bl record
   lwz %r3,36(%r4)
   lis %r7,0xFFFF
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit signed, no shift
0: lis %r3,0x0006
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   li %r3,0x01FE
   sth %r3,40(%r4)
   psq_l %f3,40(%r4),0,1
   bl record
   fmr %f4,%f1
   fneg %f5,%f2
   bl check_ps
0: li %r3,0x0006
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,40(%r4)
   psq_st %f3,40(%r4),0,1
   bl record
   lhz %r3,40(%r4)
   cmpwi %r3,0x01FE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit signed, positive shift
0: lis %r3,0x0206
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   li %r3,0x04F8
   sth %r3,44(%r4)
   psq_l %f3,44(%r4),0,1
   bl record
   fmr %f4,%f1
   fneg %f5,%f2
   bl check_ps
0: li %r3,0x0206
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,44(%r4)
   psq_st %f3,44(%r4),0,1
   bl record
   lhz %r3,44(%r4)
   cmpwi %r3,0x04F8
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit signed, negative shift
0: lis %r3,0x3F06
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   li %r3,0x01FE
   sth %r3,48(%r4)
   psq_l %f3,48(%r4),0,1
   bl record
   fmr %f4,%f2
   fadd %f5,%f2,%f2
   fneg %f5,%f5
   bl check_ps
0: li %r3,0x3F06
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,48(%r4)
   psq_st %f3,48(%r4),0,1
   bl record
   lhz %r3,48(%r4)
   cmpwi %r3,0x01FE
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit signed, overflow
0: li %r3,0x0006
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,52(%r4)
   psq_st %f28,52(%r4),0,1
   bl record
   lhz %r3,52(%r4)
   cmpwi %r3,0x7F80
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 8-bit signed, NaN
0: li %r3,0x0006
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,56(%r4)
   psq_st %f29,56(%r4),0,1
   bl record
   lhz %r3,56(%r4)
   cmpwi %r3,0x7F80
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit signed, no shift
0: lis %r3,0x0007
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r24,60(%r4)
   psq_l %f3,60(%r4),0,1
   bl record
   fmr %f4,%f6
   fmr %f5,%f8
   bl check_ps
0: li %r3,0x0007
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,60(%r4)
   psq_st %f3,60(%r4),0,1
   bl record
   lwz %r3,60(%r4)
   cmpw %r3,%r24
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit signed, positive shift
0: lis %r3,0x0207
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r24,64(%r4)
   psq_l %f3,64(%r4),0,1
   bl record
   lfd %f9,56(%r31)  # 0.25
   fmul %f4,%f6,%f9
   fmul %f5,%f8,%f9
   bl check_ps
0: li %r3,0x0207
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,64(%r4)
   psq_st %f3,64(%r4),0,1
   bl record
   lwz %r3,64(%r4)
   cmpw %r3,%r24
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit signed, negative shift
0: lis %r3,0x3F07
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r24,68(%r4)
   psq_l %f3,68(%r4),0,1
   bl record
   fmul %f4,%f6,%f2
   fmul %f5,%f8,%f2
   bl check_ps
0: li %r3,0x3F07
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,68(%r4)
   psq_st %f3,68(%r4),0,1
   bl record
   lwz %r3,68(%r4)
   cmpw %r3,%r24
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit signed, overflow
0: li %r3,0x0007
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,72(%r4)
   psq_st %f28,72(%r4),0,1
   bl record
   lwz %r3,72(%r4)
   lis %r7,0x7FFF
   ori %r7,%r7,0x8000
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

   # 16-bit signed, NaN
0: li %r3,0x0007
   .if ESPRESSO
      mtspr 897,%r3
   .else
      mtspr 913,%r3
   .endif
   stw %r0,76(%r4)
   psq_st %f29,76(%r4),0,1
   bl record
   lwz %r3,76(%r4)
   lis %r7,0x7FFF
   ori %r7,%r7,0x8000
   cmpw %r3,%r7
   beq 0f
   stw %r3,8(%r6)
   addi %r6,%r6,32

.endif  # CAN_SET_GQR

   ########################################################################
   # Paired-single interactions with load/store instructions
   ########################################################################

   # Paired-single output -> stfs
0: stw %r0,4(%r4)
   ps_merge10 %f3,%f25,%f25  # (3.0f,2.0f)
   stfs %f3,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lwz %r8,4(%r4)
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpwi %r8,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   addi %r6,%r6,32

   # Paired-single output -> stfd
0: li %r3,-1
   stw %r3,4(%r4)
   ps_merge10 %f3,%f25,%f25  # (3.0f,2.0f)
   stfd %f3,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lwz %r8,4(%r4)
   lis %r7,0x4008
   cmpw %r3,%r7
   bne 1f
   cmpwi %r8,0
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,12(%r6)
   addi %r6,%r6,32

   # lfd -> paired-single input
   # Note that the 750CL has incomplete interlocking between lfd and
   # paired-single instructions, so these tests are sensitive to cache
   # and pipeline states.
   .balign 64
0: isync
   lfs %f3,0(%r30)  # Prime slot 1.
   lfd %f3,8(%r31)  # 1.0
   bl record
   lfd %f4,8(%r31)
   lfs %f5,0(%r30)
   bl check_ps
   # Precision should be preserved on a loaded double value.
0: lfs %f3,8(%r30)
   lfd %f3,288(%r31)  # 1.333...
   bl record
   lfd %f4,288(%r31)
   lfs %f5,8(%r30)
   bl check_ps
   # In some cases, the high word of a double value loaded from lfd can
   # leak into the second slot of a paired-single value.  We assume here
   # that ps_mr works correctly.
0: ps_mr %f4,%f0
   lfd %f4,8(%r31)    # 1.0
   ps_merge01 %f3,%f4,%f4
   bl record2
   fmr %f4,%f1
   lfs %f5,8(%r31)
   bl check_ps

   # lfd -> ps_merge
0: lfd %f3,288(%r31)  # 1.333...
   ps_merge00 %f3,%f3,%f3
   bl record
   # Slot 0 is rounded, while slot 1 is truncated.
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAB
   stw %r3,0(%r4)
   addi %r3,%r3,-1
   stw %r3,4(%r4)
   lfs %f4,0(%r4)
   lfs %f5,4(%r4)
   bl check_ps
0: lfd %f4,320(%r31)  # HUGE_VAL
   ps_merge00 %f3,%f4,%f4
   bl record
   stfd %f3,8(%r6)
   stfs %f3,16(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,20(%r6)
   # Slot 0 rounds up to infinity.
   lwz %r3,8(%r6)
   lis %r7,0x7FF0
   cmpw %r3,%r7
   bne 1f
   lwz %r3,12(%r6)
   cmpwi %r3,0
   bne 1f
   lwz %r3,16(%r6)
   lis %r7,0x7F80
   cmpw %r3,%r7
   bne 1f
   # Slot 1 is clamped to the maximum normal value.
   lwz %r3,20(%r6)
   addi %r7,%r7,-1
   cmpw %r3,%r7
   beq 0f
1: addi %r6,%r6,32
0: lfd %f5,376(%r31)  # Minimum double-precision denormal
   ps_merge00 %f3,%f5,%f5
   bl record
   stfd %f3,8(%r6)
   stfs %f3,16(%r6)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,20(%r6)
   # Slot 0 is rounded down to 0 as double-precision...
   lwz %r3,8(%r6)
   cmpwi %r3,0
   bne 1f
   lwz %r3,12(%r6)
   cmpwi %r3,0
   bne 1f
   # ... but clamped to the minimum (single-precision) normal value when
   # stored as single-precision.
   lwz %r3,16(%r6)
   lis %r7,0x80
   cmpw %r3,%r7
   bne 2f
   # A subsequent double-precision store of ps0 still stores zero.
   stfd %f3,8(%r6)
   lwz %r3,8(%r6)
   cmpwi %r3,0
   bne 2f
   lwz %r3,12(%r6)
   cmpwi %r3,0
   bne 2f
   # Slot 1 is also clamped.
   lwz %r3,20(%r6)
   lis %r7,0x80
   cmpw %r3,%r7
   bne 2f
   b 0f
1: li %r3,-1    # Indicate failure of the initial double-precision test.
   stw %r3,20(%r6)
2: addi %r6,%r6,32

   # lfd over paired-single register -> stfd
0: ps_merge00 %f3,%f2,%f2
   lfd %f3,64(%r31)  # 0.5
   stfd %f3,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lwz %r7,64(%r31)
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lwz %r7,68(%r31)
   cmpw %r3,%r7
   beq 0f
1: addi %r6,%r6,32
0: ps_merge00 %f4,%f31,%f31
   lfd %f4,360(%r31)  # 1.666...
   stfd %f4,0(%r4)
   bl record
   lwz %r3,0(%r4)
   lwz %r7,360(%r31)
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lwz %r7,364(%r31)
   cmpw %r3,%r7
   beq 0f
1: addi %r6,%r6,32

   ########################################################################
   # Paired-single move instructions
   ########################################################################

0: lfs %f6,20(%r30) # QNaN
   lfs %f7,24(%r30) # SNaN
   mtfsf 255,%f0
   mtfsfi 0,8       # Ensure that Rc=1 insns have a nonzero value to write.
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r11,4(%r4)
   li %r0,0

   # ps_mr
0: mtcr %r0
   ps_mr %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3         # Also check that FPSCR is unmodified.
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x4000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0      # Check that CR1 is unmodified (Rc=0).
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
   # Check that denormals are moved unmodified.
0: lfs %f5,44(%r30)  # Minimum single-precision denormal
   ps_mr %f3,%f5
   bl record
   # We can't use psq_st to store these since they get flushed to zero,
   # so use stfs and ps_merge instead.
   stfs %f3,0(%r4)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,4(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   li %r7,1
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
   # Check that NaNs don't cause exceptions with these instructions.
0: ps_mr %f3,%f6
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FD0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_mr %f3,%f7
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFA0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_mr (excess precision on input)
0: lfd_ps %f4,288(%r31),%f0
   ps_mr %f3,%f4
   bl record
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF5
   ori %r7,%r7,0x5555
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x6000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_mr.
0: mtcr %r0
   ps_mr. %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x4000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_neg
0: mtcr %r0
   ps_neg %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xC000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0xC040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: lfs %f5,44(%r30)
   ps_neg %f3,%f5
   bl record
   stfs %f3,0(%r4)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,4(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x8000
   ori %r7,%r7,1
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_neg %f3,%f6
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFD0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_neg %f3,%f7
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FA0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
   # ps_neg should negate the sign of zero as well.
0: ps_neg %f3,%f0
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x8000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_neg %f3,%f0
   ps_neg %f3,%f3
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   cmpwi %r3,0
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_neg (excess precision on input)
0: lfd_ps %f4,288(%r31),%f0
   ps_neg %f3,%f4
   bl record
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF5
   ori %r7,%r7,0x5555
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x6000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_neg.
0: mtcr %r0
   ps_neg. %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xC000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0xC040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_abs
0: mtcr %r0
   ps_mr %f3,%f0
   ps_abs %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x4000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_neg %f3,%f25
   ps_abs %f3,%f3
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x4000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: lfs %f5,48(%r30)
   ps_abs %f3,%f5
   bl record
   stfs %f3,0(%r4)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,4(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   li %r7,1
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_abs %f3,%f6
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FD0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_abs %f3,%f7
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x7FA0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_abs (excess precision on input)
0: lfd_ps %f4,288(%r31),%f0
   ps_abs %f3,%f4
   bl record
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x3FF5
   ori %r7,%r7,0x5555
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x6000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_abs.
0: mtcr %r0
   ps_mr %f3,%f0
   ps_abs. %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x4000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_nabs
0: mtcr %r0
   ps_mr %f3,%f0
   ps_nabs %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xC000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0xC040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_neg %f3,%f25
   ps_nabs %f3,%f3
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xC000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0xC040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: lfs %f5,48(%r30)
   ps_nabs %f3,%f5
   bl record
   stfs %f3,0(%r4)
   ps_merge11 %f3,%f3,%f3
   stfs %f3,4(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x8000
   ori %r7,%r7,1
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_nabs %f3,%f6
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFD0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_nabs %f3,%f7
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xFFA0
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: ps_nabs %f3,%f0
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0x8000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_nabs (excess precision on input)
0: lfd_ps %f4,288(%r31),%f0
   ps_nabs %f3,%f4
   bl record
   stfd %f3,0(%r4)
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xBFF5
   ori %r7,%r7,0x5555
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0x6000
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   cmpwi %r3,0
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_nabs.
0: mtcr %r0
   ps_mr %f3,%f0
   ps_nabs. %f3,%f25
   bl record
   psq_st %f3,0(%r4),0,0
   lwz %r3,0(%r4)
   mffs %f3
   stfd %f3,8(%r4)
   lwz %r8,12(%r4)
   mfcr %r9
   lis %r7,0xC000
   cmpw %r3,%r7
   stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   bne 1f
   lis %r7,0xC040
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r11
   bne 1f
   andis. %r3,%r9,0x0F00
   lis %r7,0x0800
   cmpw %r3,%r7
   beq 0f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   ########################################################################
   # Paired-single compare instructions
   ########################################################################

   # ps_cmpu0
0: mtcr %r0
   mtfsf 255,%f0
   ps_cmpu0 cr0,%f0,%f0     # 0.0 == 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: ps_cmpu0 cr1,%f1,%f0     # 1.0 > 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2400
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_neg %f8,%f1
   ps_cmpu0 cr7,%f8,%f0     # -1.0 < 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpu0 cr7,%f6,%f0     # QNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,0x1000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Clear FPSCR here to verify that FPSCR[19] is set again and not just
   # carried over from the last test.
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpu0 cr7,%f0,%f6     # 0.0 ? QNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,0x1000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpu0 cr7,%f7,%f0     # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Clear FPSCR to verify that FPSCR[19] is set and also to ensure that
   # the exception flag is newly raised.
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpu0 cr7,%f0,%f7     # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Test again with FPSCR[FX] cleared to ensure that it's not set again
   # when the exception flag does not change.
0: mtfsb0 0
   mtcr %r0
   ps_cmpu0 cr5,%f0,%f7     # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x100
   bne 1f
   lis %r8,0x2100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
   # Check that a set exception flag is not cleared if no exception occurs.
0: mtcr %r0
   ps_cmpu0 cr5,%f0,%f0     # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x200
   bne 1f
   lis %r8,0x2100
   ori %r8,%r8,0x2000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpu0 (NaNs in unused slot)
0: mtcr %r0
   mtfsf 255,%f0
   ps_merge00 %f8,%f1,%f7
   ps_merge00 %f9,%f0,%f7
   ps_cmpu0 cr0,%f8,%f9
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x4000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpu0 (excess precision)
0: mtcr %r0
   mtfsf 255,%f0
   lfd %f3,288(%r31)
   lfd %f4,288(%r31)
   ps_cmpu0 cr0,%f3,%f4     # 1.333... == 1.333...
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   mtfsf 255,%f0
   lfd %f3,288(%r31)
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAA
   stw %r3,0(%r4)
   lfs %f5,0(%r4)
   ps_cmpu0 cr0,%f3,%f5     # 1.333... > truncate_to_float(1.333...)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x4000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   mtfsf 255,%f0
   lfd %f3,288(%r31)
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAB
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   ps_cmpu0 cr1,%f3,%f4     # 1.333... < round_to_float(1.333...)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x0800
   cmpw %r3,%r8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo0 (VE=0)
0: mtcr %r0
   mtfsf 255,%f0
   ps_cmpo0 cr0,%f0,%f0     # 0.0 == 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: ps_cmpo0 cr1,%f1,%f0     # 1.0 > 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2400
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_neg %f8,%r1
   ps_cmpo0 cr7,%f8,%f0     # -1.0 < 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpo0 cr7,%f6,%f0     # QNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA008
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpo0 cr7,%f0,%f6     # 0.0 ? QNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA008
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpo0 cr7,%f7,%f0     # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpo0 cr7,%f0,%f7     # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   ps_cmpo0 cr5,%f0,%f7     # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x100
   bne 1f
   lis %r8,0x2108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpo0 cr5,%f0,%f0     # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x200
   bne 1f
   lis %r8,0x2108
   ori %r8,%r8,0x2000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo0 (VE=1)
0: mtfsf 255,%f0
   mtfsb1 24
   mtcr %r0
   ps_cmpo0 cr6,%f7,%f0     # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x10
   bne 1f
   lis %r8,0xE100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtfsb1 24
   mtcr %r0
   ps_cmpo0 cr6,%f0,%f7     # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x10
   bne 1f
   lis %r8,0xE100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   ps_cmpo0 cr4,%f0,%f7     # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x1000
   bne 1f
   lis %r8,0x6100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpo0 cr4,%f0,%f0     # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x2000
   bne 1f
   lis %r8,0x6100
   ori %r8,%r8,0x2080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo0 (NaNs in unused slot)
0: mtcr %r0
   mtfsf 255,%f0
   ps_merge00 %f8,%f1,%f7
   ps_merge00 %f9,%f0,%f7
   ps_cmpo0 cr0,%f8,%f9
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x4000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo0 (excess precision)
0: mtcr %r0
   mtfsf 255,%f0
   lfd %f3,288(%r31)
   lfd %f4,288(%r31)
   ps_cmpo0 cr0,%f3,%f4     # 1.333... == 1.333...
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   mtfsf 255,%f0
   lfd %f3,288(%r31)
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAA
   stw %r3,0(%r4)
   lfs %f5,0(%r4)
   ps_cmpo0 cr0,%f3,%f5     # 1.333... > truncate_to_float(1.333...)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x4000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   mtfsf 255,%f0
   lfd %f3,288(%r31)
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAB
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   ps_cmpo0 cr1,%f3,%f4     # 1.333... < round_to_float(1.333...)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x0800
   cmpw %r3,%r8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpu1
0: mtcr %r0
   mtfsf 255,%f0
   ps_cmpu1 cr0,%f0,%f0     # 0.0 == 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: ps_cmpu1 cr1,%f1,%f0     # 1.0 > 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2400
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_neg %f8,%f1
   ps_cmpu1 cr7,%f8,%f0     # -1.0 < 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpu1 cr7,%f6,%f0     # QNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,0x1000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpu1 cr7,%f0,%f6     # 0.0 ? QNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   cmpwi %r7,0x1000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpu1 cr7,%f7,%f0     # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpu1 cr7,%f0,%f7     # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   ps_cmpu1 cr5,%f0,%f7     # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x100
   bne 1f
   lis %r8,0x2100
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpu1 cr5,%f0,%f0     # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x200
   bne 1f
   lis %r8,0x2100
   ori %r8,%r8,0x2000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpu1 (NaNs in unused slot)
0: mtcr %r0
   mtfsf 255,%f0
   ps_merge00 %f8,%f7,%f1
   ps_merge00 %f9,%f7,%f0
   ps_cmpu1 cr0,%f8,%f9
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x4000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo1 (VE=0)
0: mtcr %r0
   mtfsf 255,%f0
   ps_cmpo1 cr0,%f0,%f0     # 0.0 == 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x2000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: ps_cmpo1 cr1,%f1,%f0     # 1.0 > 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x2400
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_neg %f8,%r1
   ps_cmpo1 cr7,%f8,%f0     # -1.0 < 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,8
   bne 1f
   ori %r8,%r0,0x8000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpo1 cr7,%f6,%f0     # QNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA008
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpo1 cr7,%f0,%f6     # 0.0 ? QNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA008
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpo1 cr7,%f7,%f0     # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtcr %r0
   ps_cmpo1 cr7,%f0,%f7     # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,1
   bne 1f
   lis %r8,0xA108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   ps_cmpo1 cr5,%f0,%f7     # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x100
   bne 1f
   lis %r8,0x2108
   ori %r8,%r8,0x1000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpo1 cr5,%f0,%f0     # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x200
   bne 1f
   lis %r8,0x2108
   ori %r8,%r8,0x2000
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo1 (VE=1)
0: mtfsf 255,%f0
   mtfsb1 24
   mtcr %r0
   ps_cmpo1 cr6,%f7,%f0     # SNaN ? 0.0
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x10
   bne 1f
   lis %r8,0xE100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   mtfsb1 24
   mtcr %r0
   ps_cmpo1 cr6,%f0,%f7     # 0.0 ? SNaN
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x10
   bne 1f
   lis %r8,0xE100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtfsb0 0
   mtcr %r0
   ps_cmpo1 cr4,%f0,%f7     # 0.0 ? SNaN (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x1000
   bne 1f
   lis %r8,0x6100
   ori %r8,%r8,0x1080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32
0: mtcr %r0
   ps_cmpo1 cr4,%f0,%f0     # 0.0 == 0.0 (2nd time)
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0x2000
   bne 1f
   lis %r8,0x6100
   ori %r8,%r8,0x2080
   cmpw %r7,%r8
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   # ps_cmpo1 (NaNs in unused slot)
0: mtcr %r0
   mtfsf 255,%f0
   ps_merge00 %f8,%f7,%f1
   ps_merge00 %f9,%f7,%f0
   ps_cmpo1 cr0,%f8,%f9
   bl record
   mfcr %r3
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r7,4(%r4)
   lis %r8,0x4000
   cmpw %r3,%r8
   bne 1f
   cmpwi %r7,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,16(%r6)
   addi %r6,%r6,32

   ########################################################################
   # Paired-single elementary arithmetic instructions
   ########################################################################

   # Load constants (these are mostly the same as used in the regular FP
   # instruction tests, with some constants changed for precision reasons).
0: lfd %f24,80(%r31)   # 4194304.0
   lfd %f25,112(%r31)  # 4194304.5
   lfd %f26,56(%r31)   # 0.25
   lfd %f27,64(%r31)   # 0.5
   lfd %f28,184(%r31)  # 2097152.0
   fadd %f29,%f28,%f26 # 2097152.25
   lfd %f30,128(%r31)  # 4194305.0
   lfd %f7,152(%r31)   # HUGE_VALF (maximum normal single-precision value)
   lfd %f9,16(%r31)    # Positive infinity
   lfd %f10,32(%r31)   # QNaN
   lfd %f11,40(%r31)   # SNaN
   lfd %f12,48(%r31)   # SNaN converted to QNaN
   # Store and load as single-precision so both slots are initialized.
   stfs %f24,0(%r4)
   lfs %f24,0(%r4)
   stfs %f25,0(%r4)
   lfs %f25,0(%r4)
   stfs %f26,0(%r4)
   lfs %f26,0(%r4)
   stfs %f27,0(%r4)
   lfs %f27,0(%r4)
   stfs %f28,0(%r4)
   lfs %f28,0(%r4)
   stfs %f29,0(%r4)
   lfs %f29,0(%r4)
   stfs %f30,0(%r4)
   lfs %f30,0(%r4)
   stfs %f7,0(%r4)
   lfs %f7,0(%r4)
   stfs %f9,0(%r4)
   lfs %f9,0(%r4)
   stfs %f10,0(%r4)
   lfs %f10,0(%r4)
   stfs %f11,0(%r4)
   lfs %f11,0(%r4)
   stfs %f12,0(%r4)
   lfs %f12,0(%r4)

   li %r0,0
   mtcr %r0
   mtfsf 255,%f0

   # ps_add
0: ps_add %f3,%f1,%f24  # normal + normal
   bl record
   fmr %f4,%f30
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_add %f3,%f7,%f7   # normal + normal (result overflows)
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f4
   bl check_ps_pinf_inex
0: ps_add %f3,%f9,%f1   # +infinity + normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_add %f3,%f9,%f9   # +infinity + +infinity
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f13,%f9      # +infinity + -infinity
   ps_add %f3,%f9,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_add %f3,%f10,%f1  # QNaN + normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_add %f3,%f1,%f11  # normal + SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_add %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # -infinity + +infinity (exception enabled)
   mtfsb1 24
   ps_neg %f13,%f9
   ps_mr %f3,%f24
   ps_add %f3,%f13,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_add (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_add %f3,%f1,%f14
   mtfsb0 0
   ps_add %f3,%f1,%f14
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_add (exception persistence)
0: ps_add %f3,%f1,%f11
   ps_add %f3,%f24,%f1
   bl record
   fmr %f4,%f30
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_pnorm

   # ps_add (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) + (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f7
   ps_add %f3,%f5,%f4
   bl record
   fmr %f4,%f12
   fmr %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f7
   ps_add %f3,%f5,%f13
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f7,%f24
   ps_add %f3,%f13,%f5
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   # If only the second slot (ps1) of a paired-single operation raises an
   # enabled exception, FPRF is set based on the ps0 result even though
   # the result is not written back to the output register.  (This appears
   # to be a bug in the hardware.)
   bl check_ps_pinf
   mtfsb0 24

   # ps_add (excess precision/range on input)
0: lfd_ps %f4,368(%r31),%f0  # (1.0 + 1ulp) + -1.0
   ps_neg %f5,%f1
   ps_add %f3,%f4,%f5
   bl record
   lis %r3,0x2580
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fneg %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,368(%r31),%f0  # -1.0 + (1.0 + 1ulp)
   ps_neg %f5,%f1
   ps_add %f3,%f5,%f4
   bl record
   lis %r3,0x2580
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fneg %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # 2^128 + -HUGE_VALF
   ps_neg %f13,%f7
   ps_add %f3,%f4,%f13
   bl record
   lfd %f4,344(%r31)
   fneg %f5,%f7
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0 # -HUGE_VALF + 2^128
   ps_neg %f13,%f7
   ps_add %f3,%f13,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f5,%f7
   bl check_ps_pnorm
0: mtfsfi 7,1           # 1.999...9 + 0.000...1 (round down)
   lfd_ps %f3,304(%r31),%f0
   lfd_ps %f4,352(%r31),%f0
   ps_add %f3,%f3,%f4
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1           # 0.000...1 + 1.999...9 (round down)
   lfd_ps %f3,304(%r31),%f0
   lfd_ps %f4,352(%r31),%f0
   ps_add %f3,%f4,%f3
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0

   # ps_add.
0: ps_add. %f3,%f1,%f11  # normal + SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # normal + SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_add. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_sum0
0: ps_sum0 %f3,%f1,%f0,%f24 # normal + normal
   bl record
   fmr %f4,%f30
   fmr %f5,%f0
   bl check_ps_pnorm
0: ps_sum0 %f3,%f7,%f1,%f7  # normal + normal (result overflows)
   bl record
   fmr %f4,%f9
   fmr %f5,%f1
   bl add_fpscr_ox
   bl check_ps_pinf_inex
0: ps_sum0 %f3,%f9,%f0,%f1  # +infinity + normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f0
   bl check_ps_pinf
0: ps_sum0 %f3,%f9,%f1,%f9  # +infinity + +infinity
   bl record
   fmr %f4,%f9
   fmr %f5,%f1
   bl check_ps_pinf
0: ps_neg %f13,%f9          # +infinity + -infinity
   ps_sum0 %f3,%f9,%f0,%f13
   bl record
   lfd %f4,168(%r31)
   fmr %f5,%f0
   bl add_fpscr_vxisi
   bl check_ps_nan
0: ps_sum0 %f3,%f10,%f1,%f1 # QNaN + normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f1
   bl check_ps_nan
0: ps_sum0 %f3,%f1,%f0,%f11 # normal + SNaN
   bl record
   fmr %f4,%f12
   fmr %f5,%f0
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0            # SNaN + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_sum0 %f3,%f11,%f1,%f1
   bl record
   fmr %f4,%f24
   fmr %f5,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # -infinity + +infinity (exception enabled)
   mtfsb1 24
   ps_neg %f13,%f9
   ps_mr %f3,%f24
   ps_sum0 %f3,%f13,%f0,%f9
   bl record
   fmr %f4,%f24
   fmr %f5,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_ps_noresult
   mtfsb0 24

   # ps_sum0 (FPSCR[FX] handling)
0: ps_sum0 %f3,%f1,%f2,%f11
   mtfsb0 0
   ps_sum0 %f3,%f1,%f2,%f11
   bl record
   fmr %f4,%f12
   fmr %f5,%f2
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_ps_nan

   # ps_sum0 (exception persistence)
0: ps_sum0 %f3,%f1,%f0,%f11
   ps_sum0 %f3,%f1,%f2,%f24
   bl record
   fmr %f4,%f30
   fmr %f5,%f2
   bl add_fpscr_vxsnan
   bl check_ps_pnorm

   # ps_sum0 (different values in slots)
0: ps_merge00 %f4,%f1,%f11  # (normal + normal, normal)
   ps_merge00 %f5,%f11,%f2  # NaNs in unused slots should be ignored.
   ps_merge00 %f13,%f11,%f0
   ps_sum0 %f3,%f4,%f13,%f5
   bl record
   fmr %f4,%f31
   fmr %f5,%f0
   bl check_ps_pnorm
0: ps_merge00 %f13,%f9,%f11  # (infinity + -infinity, normal)
   fneg %f5,%f9
   ps_merge00 %f5,%f11,%f5
   ps_merge00 %f4,%f11,%f1
   ps_sum0 %f3,%f13,%f4,%f5
   bl record
   lfd %f4,168(%r31)
   fmr %f5,%f1
   bl add_fpscr_vxisi
   bl check_ps_nan
0: ps_merge00 %f4,%f1,%f11  # (normal + normal, NaN)
   ps_merge00 %f5,%f11,%f2
   ps_sum0 %f3,%f4,%f11,%f5 # Copying a NaN should not cause an exception.
   bl record
   psq_st %f3,0(%r4),0,0    # Can't use check_ps_pnorm because of the NaN.
   stfd %f3,8(%r6)
   lwz %r3,0(%r4)
   lwz %r8,4(%r4)
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r9,4(%r4)
   mtfsf 255,%f0
   lis %r7,0x4040
   cmpw %r3,%r7
   bne 1f
   lis %r7,0xFFA0
   cmpw %r8,%r7
   bne 1f
   cmpwi %r9,0x4000
   beq 2f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
2: mtcr %r0

   # ps_sum0 (excess precision/range on input)
0: lfd_ps %f4,368(%r31),%f0  # (1.0 + 1ulp) + -1.0
   ps_neg %f5,%f1
   ps_sum0 %f3,%f4,%f1,%f5
   bl record
   lis %r3,0x2580
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # 2^128 + -HUGE_VALF
   ps_neg %f13,%f7
   ps_sum0 %f3,%f4,%f1,%f13
   bl record
   lfd %f4,344(%r31)
   fmr %f5,%f1
   bl check_ps_pnorm
0: mtfsfi 7,1           # 1.999...9 + 0.000...1 (round down)
   lfd_ps %f3,304(%r31),%f0
   lfd %f4,352(%r31)
   stfs %f4,0(%r4)
   lfs %f4,0(%r4)
   ps_sum0 %f3,%f3,%f1,%f4
   bl record
   fmr %f4,%f2
   fmr %f5,%f1
   bl check_ps_pnorm
   mtfsfi 7,0

   # ps_sum0.
0: ps_sum0. %f3,%f1,%f0,%f11  # normal + SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   fmr %f5,%f0
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # normal + SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_sum0. %f3,%f1,%f2,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   fmr %f5,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_ps_noresult
   mtfsb0 24

   # ps_sum1
0: ps_sum1 %f3,%f1,%f0,%f24 # normal + normal
   bl record
   fmr %f5,%f30
   fmr %f4,%f0
   bl check_ps_pnorm
0: ps_sum1 %f3,%f7,%f1,%f7  # normal + normal (result overflows)
   bl record
   fmr %f5,%f9
   fmr %f4,%f1
   bl add_fpscr_ox
   bl check_ps_pinf_inex
0: ps_sum1 %f3,%f9,%f0,%f1  # +infinity + normal
   bl record
   fmr %f5,%f9
   fmr %f4,%f0
   bl check_ps_pinf
0: ps_sum1 %f3,%f9,%f1,%f9  # +infinity + +infinity
   bl record
   fmr %f5,%f9
   fmr %f4,%f1
   bl check_ps_pinf
0: ps_neg %f13,%f9          # +infinity + -infinity
   ps_sum1 %f3,%f9,%f0,%f13
   bl record
   lfd %f5,168(%r31)
   fmr %f4,%f0
   bl add_fpscr_vxisi
   bl check_ps_nan
0: ps_sum1 %f3,%f10,%f1,%f1 # QNaN + normal
   bl record
   fmr %f5,%f10
   fmr %f4,%f1
   bl check_ps_nan
0: ps_sum1 %f3,%f1,%f0,%f11 # normal + SNaN
   bl record
   fmr %f5,%f12
   fmr %f4,%f0
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0            # SNaN + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_sum1 %f3,%f11,%f1,%f1
   bl record
   fmr %f5,%f24
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0            # -infinity + +infinity (exception enabled)
   mtfsb1 24
   ps_neg %f13,%f9
   ps_mr %f3,%f24
   ps_sum1 %f3,%f13,%f0,%f9
   bl record
   fmr %f5,%f24
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   bl check_ps_noresult
   mtfsb0 24

   # ps_sum1 (FPSCR[FX] handling)
0: ps_sum1 %f3,%f1,%f2,%f11
   mtfsb0 0
   ps_sum1 %f3,%f1,%f2,%f11
   bl record
   fmr %f5,%f12
   fmr %f4,%f2
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   bl check_ps_nan

   # ps_sum1 (exception persistence)
0: ps_sum1 %f3,%f1,%f0,%f11
   ps_sum1 %f3,%f1,%f2,%f24
   bl record
   fmr %f5,%f30
   fmr %f4,%f2
   bl add_fpscr_vxsnan
   bl check_ps_pnorm

   # ps_sum1 (different values in slots)
0: ps_merge00 %f4,%f1,%f11  # (normal, normal + normal)
   ps_merge00 %f5,%f11,%f2  # NaNs in unused slots should be ignored.
   ps_merge00 %f13,%f0,%f11
   ps_sum1 %f3,%f4,%f13,%f5
   bl record
   fmr %f5,%f31
   fmr %f4,%f0
   bl check_ps_pnorm
0: ps_merge00 %f13,%f9,%f11  # (normal, infinity + -infinity)
   fneg %f5,%f9
   ps_merge00 %f5,%f11,%f5
   ps_merge00 %f4,%f1,%f11
   ps_sum1 %f3,%f13,%f4,%f5
   bl record
   lfd %f5,168(%r31)
   fmr %f4,%f1
   bl add_fpscr_vxisi
   bl check_ps_nan
0: ps_merge00 %f4,%f1,%f11  # (NaN, normal + normal)
   ps_merge00 %f5,%f11,%f2
   ps_sum1 %f3,%f4,%f11,%f5 # Copying a NaN should not cause an exception.
   bl record
   psq_st %f3,0(%r4),0,0    # Can't use check_ps_pnorm because of the NaN.
   stfd %f3,8(%r6)
   lwz %r3,0(%r4)
   lwz %r8,4(%r4)
   mffs %f3
   stfd %f3,0(%r4)
   lwz %r9,4(%r4)
   mtfsf 255,%f0
   lis %r7,0xFFA0
   cmpw %r3,%r7
   bne 1f
   lis %r7,0x4040
   cmpw %r8,%r7
   bne 1f
   cmpwi %r9,0x4000
   beq 2f
1: stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
2: mtcr %r0

   # ps_sum1 (excess precision on input)
0: lfd_ps %f4,288(%r31),%f0
   ps_neg %f5,%f1
   ps_sum1 %f3,%f1,%f4,%f5
   bl record
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAB   # Input value is rounded when copied.
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_pzero
0: lfd_ps %f3,320(%r31),%f0
   ps_neg %f5,%f1
   ps_sum1 %f3,%f1,%f3,%f5
   bl record
   fmr %f4,%f9
   fmr %f5,%f0
   bl check_ps_pzero
   # frC[ps0] does not get the special rounding treatment as in multiply
   # instructions; it's rounded to single precision and copied as normal.
0: mtfsfi 7,1
   lis %r3,0x4000
   stw %r3,0(%r4)
   lis %r3,0x1800
   stw %r3,4(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_sum1 %f3,%f1,%f4,%f1
   bl record
   fmr %f4,%f2
   fmr %f5,%f2
   bl check_ps_pnorm
   mtfsfi 7,0

   # ps_sum1.
0: ps_sum1. %f3,%f1,%f0,%f11  # normal + SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f5,%f12
   fmr %f4,%f0
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # normal + SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_sum1. %f3,%f1,%f2,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f5,%f11
   fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_ps_noresult
   mtfsb0 24

   # ps_sub
0: ps_sub %f3,%f30,%f1  # normal - normal
   bl record
   fmr %f4,%f24
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_neg %f4,%f7       # -normal - normal (result overflows)
   ps_sub %f3,%f4,%f7
   bl record
   ps_neg %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f4
   bl check_ps_ninf_inex
0: ps_sub %f3,%f9,%f1   # +infinity - normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_sub %f3,%f9,%f9   # +infinity - +infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f13,%f9      # +infinity - -infinity
   ps_sub %f3,%f9,%f13
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_sub %f3,%f10,%f1  # QNaN - normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_sub %f3,%f1,%f11  # normal - SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_sub %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # -infinity - -infinity (exception enabled)
   mtfsb1 24
   ps_neg %f13,%f9
   ps_mr %f3,%f24
   ps_sub %f3,%f13,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_sub (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_sub %f3,%f1,%f14
   mtfsb0 0
   ps_sub %f3,%f1,%f14
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_sub (exception persistence)
0: ps_sub %f3,%f1,%f11
   ps_mr %f14,%f30
   ps_sub %f3,%f14,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_pnorm

   # ps_sub (different values in slots)
0: ps_neg %f13,%f7      # (NaN,normal) - (normal,normal) = (NaN,overflow)
   ps_merge00 %f5,%f11,%f13
   ps_merge00 %f4,%f24,%f7
   ps_sub %f3,%f5,%f4
   bl record
   fmr %f4,%f12
   fneg %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_neg %f13,%f7
   ps_merge00 %f5,%f11,%f13
   ps_merge00 %f13,%f24,%f7
   ps_sub %f3,%f5,%f13
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_neg %f13,%f7
   ps_merge00 %f13,%f13,%f11
   ps_merge00 %f5,%f7,%f24
   ps_sub %f3,%f13,%f5
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_ninf
   mtfsb0 24

   # ps_sub (excess precision/range on input)
0: lfd_ps %f4,368(%r31),%f0  # (1.0 + 1ulp) - 1.0
   ps_sub %f3,%f4,%f1
   bl record
   lis %r3,0x2580
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fneg %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,368(%r31),%f0  # 1.0 - (1.0 + 1ulp)
   ps_sub %f3,%f1,%f4
   bl record
   lis %r3,0xA580
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fmr %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f3,336(%r31),%f0  # 2^128 - HUGE_VALF
   ps_sub %f3,%f3,%f7
   bl record
   lfd %f4,344(%r31)
   fneg %f5,%f7
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # HUGE_VALF - 2^128
   ps_sub %f3,%f7,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   fmr %f5,%f7
   bl check_ps_nnorm
0: mtfsfi 7,1           # 1.999...9 - -0.000...1 (round down)
   lfd_ps %f3,304(%r31),%f0
   lfd %f4,352(%r31)
   fneg %f4,%f4
   stfd %f4,0(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_sub %f3,%f3,%f4
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1           # 0.000...1 - -1.999...9 (round down)
   lfd %f3,304(%r31)
   fneg %f3,%f3
   stfd %f3,0(%r4)
   lfd_ps %f3,0(%r4),%f0
   lfd_ps %f4,352(%r31),%f0
   ps_sub %f3,%f4,%f3
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0

   # ps_sub.
0: ps_sub. %f3,%f1,%f11  # normal - SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # normal - SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_sub. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_mul
0: ps_mul %f3,%f2,%f28  # normal * normal
   bl record
   fmr %f4,%f24
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_mul %f3,%f7,%f2   # normal * normal (result overflows)
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f4
   bl check_ps_pinf_inex
0: ps_mul %f3,%f0,%f28  # 0 * normal
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_neg %f4,%f0       # -0 * normal
   ps_mul %f3,%f4,%f28
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_neg %f4,%f28        # -normal * 0
   ps_mul %f3,%f4,%f0
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_mul %f3,%f9,%f2   # +infinity * normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_mul %f3,%f9,%f9   # +infinity * +infinity
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f13,%f9      # +infinity * -infinity
   ps_mul %f3,%f9,%f13
   bl record
   fmr %f4,%f13
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_mul %f3,%f9,%f0   # +infinity * 0
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f13,%f9      # 0 * -infinity
   ps_mul %f3,%f0,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_mul %f3,%f10,%f1  # QNaN * normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_mul %f3,%f1,%f11  # normal * SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN * normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_mul %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # 0 * +infinity (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_mul %f3,%f0,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_mul (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_mul %f3,%f1,%f14
   mtfsb0 0
   ps_mul %f3,%f1,%f14
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_mul (exception persistence)
0: ps_mul %f3,%f1,%f11
   ps_mul %f3,%f28,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_pnorm

   # ps_mul (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) * (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f2
   ps_mul %f3,%f5,%f4
   bl record
   fmr %f4,%f12
   fmr %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f2
   ps_mul %f3,%f5,%f13
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f2,%f24
   ps_mul %f3,%f13,%f5
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_mul (excess precision/range on input)
0: lfd_ps %f4,336(%r31),%f0  # 2^128 * 0.5
   lfd_ps %f14,64(%r31),%f0
   ps_mul %f3,%f4,%f14
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # 0.5 * 2^128
   ps_mul %f3,%f14,%f4
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: mtfsfi 7,2           # 1.333... * 1.5 (round up)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f4,296(%r31),%f0
   ps_mul %f3,%f3,%f4
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
0: mtfsfi 7,1           # 1.333...4 * 1.5 (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   addi %r3,%r3,1
   stw %r3,4(%r4)
   lfd_ps %f3,0(%r4),%f0
   lfd_ps %f13,296(%r31),%f0
   ps_mul %f3,%f3,%f13
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
0: mtfsfi 7,2           # 1.5 * 1.333... (round up)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f4,296(%r31),%f0
   ps_mul %f3,%f4,%f3
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
   # Tests for mantissa rounding of the second multiply operand, as with fmuls.
   # This test will fail if multiplication is implemented using full precision.
0: mtfsfi 7,1           # 1.5 * 1.333...4 (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   addi %r3,%r3,1
   stw %r3,4(%r4)
   lfd_ps %f3,0(%r4),%f0
   lfd_ps %f13,296(%r31),%f0
   ps_mul %f3,%f13,%f3
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,8(%r4)
   lfs %f4,8(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation rounds to 25 or more bits.
0: mtfsfi 7,2           # 1.5 * 1.333... @ 26 bits (round up)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   andis. %r3,%r3,0xFC00
   stw %r3,4(%r4)
   lfd_ps %f15,0(%r4),%f0
   lfd_ps %f4,296(%r31),%f0
   ps_mul %f3,%f4,%f15
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation rounds to 23 or fewer bits.
0: mtfsfi 7,1           # 1.5 * 1.333... @ 25 bits (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   andis. %r3,%r3,0xF800
   stw %r3,4(%r4)
   lfd_ps %f4,0(%r4),%f0
   lfd_ps %f15,296(%r31),%f0
   ps_mul %f3,%f15,%f4
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation follows FPSCR[RN] instead
   # of always rounding to nearest.
0: mtfsfi 7,1           # 1.5 * 1.333...4 @ 25 bits (round down)
   lwz %r3,288(%r31)
   stw %r3,0(%r4)
   lwz %r3,292(%r31)
   andis. %r3,%r3,0xF800
   addis %r3,%r3,0x0800
   stw %r3,4(%r4)
   lfd_ps %f4,0(%r4),%f0
   lfd_ps %f13,296(%r31),%f0
   ps_mul %f4,%f13,%f4
   bl record
   fmr %f3,%f4
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
   # This test will fail if the implementation does not set FPSCR[OX]
   # after rounding frC to infinity.
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * HUGE_VAL
   ps_mul %f13,%f4,%f4
   bl record
   fmr %f3,%f13
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f0
   bl check_ps_pinf_inex
   # This test will fail if the implementation rounds a large frC to
   # infinity when the result would not overflow.
0: lfd_ps %f4,376(%r31),%f0  # minimum_denormal * HUGE_VAL
   lfd_ps %f14,320(%r31),%f0
   ps_mul %f3,%f4,%f14
   bl record
   lis %r3,0x3CD0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm
   # This test will fail if the implementation shifts out the sign bit of
   # frA when avoiding round-to-infinity of frC.  (It will also fail if
   # the previous test failed.)
0: lfd_ps %f13,320(%r31),%f0  # -minimum_denormal * HUGE_VAL
   lwz %r3,376(%r31)
   xoris %r3,%r3,0x8000
   stw %r3,0(%r4)
   lwz %r3,380(%r31)
   stw %r3,4(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_mul %f3,%f4,%f13
   bl record
   lis %r3,0xBCD0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lfd %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_nnorm
   # This test will fail if the implementation sets FPSCR[FI] or FPSCR[XX]
   # based on the result of rounding frC.
0: lfd_ps %f3,384(%r31),%f0  # 1 * (1.0f - 0.25ulp)
   ps_mul %f3,%f1,%f3
   bl record
   fmr %f4,%f1
   fmr %f5,%f0
   bl check_ps_pnorm
   # One or both of these two tests will fail if the implementation does
   # not normalize denormals before rounding.
0: mtfsfi 7,1           # 2^1023 * (2^-1048 - 1ulp)
   lis %r3,0x7FE0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   stw %r0,8(%r4)
   lis %r3,0x3FF
   ori %r3,%r3,0xFFFF
   stw %r3,12(%r4)
   lfd_ps %f3,0(%r4),%f0
   lfd_ps %f14,8(%r4),%f0
   ps_mul %f3,%f3,%f14
   bl record
   lis %r3,0x3E60
   stw %r3,16(%r4)
   stw %r0,20(%r4)
   lfd %f4,16(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1           # 2^1023 * (2^-1049 - 1ulp)
   lis %r3,0x7FE0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   stw %r0,8(%r4)
   lis %r3,0x1FF
   ori %r3,%r3,0xFFFF
   stw %r3,12(%r4)
   lfd_ps %f3,0(%r4),%f0
   lfd_ps %f15,8(%r4),%f0
   ps_mul %f3,%f3,%f15
   bl record
   lis %r3,0x3E4F
   ori %r3,%r3,0xFFFF
   stw %r3,16(%r4)
   lis %r3,0xE000
   stw %r3,20(%r4)
   lfd %f4,16(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
   # This test will fail if normalizing frC shifts the sign bit out.
0: mtfsfi 7,1           # 2^1023 * (2^-1048 - 1ulp)
   lis %r3,0x7FE0
   stw %r3,0(%r4)
   stw %r0,4(%r4)
   lis %r3,0x8000
   stw %r3,8(%r4)
   lis %r3,0x3FF
   ori %r3,%r3,0xFFFF
   stw %r3,12(%r4)
   lfd_ps %f13,0(%r4),%f0
   lfd_ps %f15,8(%r4),%f0
   ps_mul %f3,%f13,%f15
   bl record
   lis %r3,0xBE60
   stw %r3,16(%r4)
   stw %r0,20(%r4)
   lfd %f4,16(%r4)
   fmr %f5,%f0
   bl check_ps_nnorm
   mtfsfi 7,0
   # This test will fail if the implementation fails to raise an inexact
   # or underflow exception when normalization of a second-operand denormal
   # forces the first operand to zero.
0: lfd_ps %f14,240(%r31),%f0  # minimum_normal * minimum_denormal
   lfd_ps %f3,376(%r31),%f0
   ps_mul %f3,%f14,%f3
   bl record
   fmr %f4,%f0
   fmr %f5,%f0
   bl add_fpscr_ux
   bl check_ps_pzero_inex
   # This test will fail if the implementation changes the sign of the
   # result during underflow.
0: lfd_ps %f13,376(%r31),%f0  # minimum_denormal * -minimum_denormal
   fneg %f14,%f13
   stfd %f14,0(%r4)
   lfd_ps %f14,0(%r4),%f0
   ps_mul %f3,%f13,%f14
   bl record
   fneg %f4,%f0
   fmr %f5,%f0
   bl add_fpscr_ux
   bl check_ps_nzero_inex
   # This test will fail if the implementation raises an inexact exception
   # when multiplying a rounded value by zero.
0: lfd_ps %f14,288(%r31),%f0  # 0 * 1.333...
   ps_mul %f3,%f0,%f14
   bl record
   fmr %f4,%f0
   fmr %f5,%f0
   bl check_ps_pzero
   # This test will fail if the implementation raises a 0*inf exception
   # when multiplying a value that gets rounded up to infinity.
0: lfd_ps %f15,320(%r31),%f0  # 0 * HUGE_VAL
   ps_mul %f3,%f0,%f15
   bl record
   fmr %f4,%f0
   fmr %f5,%f0
   bl check_ps_pzero
   # This test will fail if the implementation raises a 0*inf exception
   # when multiplying infinity by a value that gets rounded down to zero.
0: lfd_ps %f4,376(%r31),%f1  # infinity * minimum_denormal
   ps_mul %f3,%f9,%f4
   bl record
   fmr %f4,%f9
   fmr %f5,%f9
   bl check_ps_pinf
   # This test will fail (with a result of zero) if the implementation
   # blindly rounds up the mantissa without checking for NaNness.
0: li %r3,-1            # 1 * SNaN(-1)
   stw %r3,0(%r4)
   stw %r3,4(%r4)
   lfd_ps %f13,0(%r4),%f0
   ps_mul %f3,%f1,%f13
   bl record
   lis %r3,0xE000
   stw %r3,4(%r4)
   lfd %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_nan

   # ps_mul.
0: ps_mul. %f3,%f1,%f11  # normal * SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # normal * SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_mul. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_muls0
0: ps_merge00 %f4,%f1,%f7   # (normal,normal) * normal = (normal,overflow)
   ps_merge00 %f5,%f2,%f11  # NaN in second slot should be ignored.
   ps_muls0 %f3,%f4,%f5
   bl record
   fmr %f4,%f2
   fmr %f5,%f9
   bl add_fpscr_ox
   bl check_ps_pnorm_inex
0: ps_merge00 %f5,%f1,%f7   # (normal,normal) * NaN = (NaN,NaN)
   ps_merge00 %f4,%f11,%f2
   ps_muls0 %f3,%f5,%f4
   bl record
   fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal, exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f2,%f11
   ps_muls0 %f3,%f5,%f13
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_muls0.
0: ps_merge00 %f5,%f1,%f7   # (normal,normal) * NaN = (NaN,NaN)
   ps_merge00 %f4,%f11,%f2
   ps_muls0. %f3,%f5,%f4
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal, exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f2,%f11
   ps_muls0. %f3,%f5,%f13
   bl record
   mfcr %r3
   lis %r7,0x0F00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_muls1
0: ps_merge00 %f4,%f1,%f7   # (normal,normal) * normal = (normal,overflow)
   ps_merge00 %f5,%f11,%f2  # NaN in first slot should be ignored.
   ps_muls1 %f3,%f4,%f5
   bl record
   fmr %f4,%f2
   fmr %f5,%f9
   bl add_fpscr_ox
   bl check_ps_pnorm_inex
0: ps_merge00 %f5,%f1,%f7   # (normal,normal) * NaN = (NaN,NaN)
   ps_merge00 %f4,%f2,%f11
   ps_muls1 %f3,%f5,%f4
   bl record
   fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal, exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f11,%f2
   ps_muls1 %f3,%f5,%f13
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_muls1.
0: ps_merge00 %f5,%f1,%f7   # (normal,normal) * NaN = (NaN,NaN)
   ps_merge00 %f4,%f2,%f11
   ps_muls1. %f3,%f5,%f4
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal, exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f11,%f2
   ps_muls1. %f3,%f5,%f13
   bl record
   mfcr %r3
   lis %r7,0x0F00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_div
0: ps_div %f3,%f24,%f2  # normal / normal
   bl record
   fmr %f4,%f28
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_div %f3,%f7,%f26  # normal / normal (result overflows)
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f4
   bl check_ps_pinf_inex
0: ps_div %f3,%f9,%f2   # +infinity / normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_div %f3,%f9,%f9   # +infinity / +infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxidi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_div %f3,%f2,%f0   # normal / 0
   bl record
   fmr %f4,%f9
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f3,%f0       # normal / -0
   ps_div %f3,%f2,%f3
   bl record
   ps_neg %f4,%f9
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_div %f3,%f0,%f0   # 0 / 0
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxzdz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_div %f3,%f10,%f1  # QNaN / normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_div %f3,%f1,%f11  # normal / SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN / normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_div %f3,%f11,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # +infinity / -infinity (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_neg %f13,%f9
   ps_div %f3,%f9,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxidi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # 0 / 0 (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_mr %f4,%f0
   ps_div %f3,%f4,%f4
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxzdz
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsb1 27            # normal / 0 (exception enabled)
   ps_mr %f3,%f24
   ps_div %f3,%f1,%f0
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 27

   # ps_div (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_div %f3,%f1,%f14
   mtfsb0 0
   ps_div %f3,%f1,%f14
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_div (exception persistence)
0: ps_div %f3,%f1,%f11
   ps_div %f3,%f24,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_pnorm

   # ps_div (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) / (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f27
   ps_div %f3,%f5,%f4
   bl record
   fmr %f4,%f12
   fmr %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f27
   ps_div %f3,%f5,%f13
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f27,%f24
   ps_div %f3,%f13,%f5
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24
0: ps_merge00 %f3,%f0,%f9  # (0,inf) / (0,inf)
   ps_div %f3,%f3,%f3
   bl record
   lfd %f4,168(%r31)
   fmr %f5,%f4
   bl add_fpscr_vxidi   # Both invalid exceptions should be raised.
   bl add_fpscr_vxzdz
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f0,%f9
   ps_div %f3,%f5,%f5
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxidi
   bl add_fpscr_vxzdz
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # (normal,normal) / (normal,0) (exception enabled)
   mtfsb1 27
   ps_merge00 %f4,%f2,%f0
   ps_mr %f3,%f1
   ps_div %f3,%f1,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_zx
   bl check_ps_pnorm
   mtfsb0 27
0: mtfsf 255,%f0        # (normal,normal) / (0,normal) = (inf,overflow) (exception enabled)
   mtfsb1 27
   ps_merge00 %f5,%f1,%f7
   ps_merge00 %f14,%f0,%f27
   ps_mr %f3,%f1
   ps_div %f3,%f5,%f14
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_ox
   bl add_fpscr_zx
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 27

   # ps_div (excess precision/range on input)
0: lfd_ps %f4,336(%r31),%f0  # 2^128 / 2.0
   ps_div %f3,%f4,%f2
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # 2^128 / 0.5
   lfd_ps %f13,64(%r31),%f1  # Avoid 0/0 in the second slot.
   ps_div %f3,%f4,%f13
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f0
   bl check_ps_pinf_inex
0: lis %r3,0x3FFA       # 1.666666716337204 / 1.25 = 1.3333333730697632 (exact)
   ori %r3,%r3,0xAAAA
   stw %r3,0(%r4)
   lis %r3,0xB800
   stw %r3,4(%r4)
   lfd_ps %f3,0(%r4),%f0
   lis %r3,0x3FA0
   stw %r3,8(%r4)
   lfs %f4,8(%r4)
   ps_div %f3,%f3,%f4
   bl record
   lis %r3,0x3FAA
   ori %r3,%r3,0xAAAB
   stw %r3,12(%r4)
   lfs %f4,12(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f4,0(%r4),%f0  # 1.666666716337204 / 1.3333333730697632 = 1.25 (exact)
   lfs %f3,12(%r4)
   ps_div %f3,%f4,%f3
   bl record
   lfs %f4,8(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm

   # ps_div.
0: ps_div. %f3,%f1,%f11  # normal / SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # normal / SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_div. %f3,%f1,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   ########################################################################
   # Paired-single multiply-add instructions
   ########################################################################

   # ps_madd
0: ps_madd %f3,%f2,%f28,%f1   # normal * normal + normal
   bl record
   fmr %f4,%f30
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_madd %f3,%f0,%f1,%f2    # 0 * normal + normal
   bl record
   fmr %f4,%f2
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_neg %f4,%f0             # -0 * normal + 0
   ps_madd %f3,%f4,%f1,%f0
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_neg %f4,%f1             # 0 * -normal + 0
   ps_madd %f3,%f0,%f4,%f0
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_neg %f4,%f0             # 0 * normal + -0
   ps_madd %f3,%f0,%f1,%f4
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: lfs %f3,12(%r30)         # minimum_normal * minimum_normal + -minimum_normal
   ps_neg %f4,%f3
   ps_madd %f3,%f3,%f3,%f4
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_nnorm_inex
0: ps_madd %f3,%f9,%f2,%f1    # +infinity * normal + normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_madd %f3,%f9,%f2,%f9    # +infinity * normal + +infinity
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f13,%f9            # +infinity * -normal + -infinity
   ps_neg %f2,%f2
   ps_madd %f3,%f9,%f2,%f13
   bl record
   ps_neg %f2,%f2
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_neg %f13,%f9            # -infinity * normal + +infinity
   ps_madd %f3,%f13,%f2,%f9
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f7             # huge * -huge + +infinity
   ps_madd %f3,%f7,%f4,%f9
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_madd %f3,%f9,%f0,%f1    # +infinity * 0 + normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f9             # +infinity * 0 + -infinity
   ps_madd %f3,%f9,%f0,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f13,%f9            # 0 * -infinity + SNaN
   ps_madd %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_madd %f3,%f10,%f1,%f2   # QNaN * normal + normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_madd %f3,%f1,%f11,%f2   # normal * SNaN + normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_madd %f3,%f1,%f2,%f11   # normal * normal + SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f9             # +infinity * QNaN + -infinity
   ps_madd %f3,%f9,%f10,%f4
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # SNaN * normal + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_madd %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # 0 * +infinity + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_madd %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # +infinity * normal + -infinity (exception enabled)
   mtfsb1 24
   ps_neg %f13,%f9
   ps_mr %f3,%f24
   ps_madd %f3,%f9,%f2,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
   # Check that the product is not rounded.
0: mtfsfi 7,2                 # 1.333... * 1.5 + 0 (round toward +inf)
   lfs %f3,28(%r30)
   lfs %f13,32(%r30)
   ps_madd %f3,%f3,%f13,%f0   # Should round to (2.0 + 1ulp).
   bl record
   lfs %f4,40(%r30)
   ps_add %f3,%f3,%r4         # Should not change the result.
   lfs %f4,36(%r30)
   fmr %f5,%f4
   bl check_ps_pnorm_inex
   mtfsfi 7,0
0: mtfsfi 7,2                 # 1.333... * 1.5 + -1ulp (round toward +inf)
   lfs %f3,28(%r30)
   lfs %f13,32(%r30)
   lfs %f4,40(%r30)
   ps_madd %f3,%f3,%f13,%f4   # Should be exactly 2.0.
   bl record
   fmr %f4,%f2
   fmr %f5,%f4
   bl check_ps_pnorm
   mtfsfi 7,0

   # ps_madd (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_madd %f3,%f1,%f14,%f2
   mtfsb0 0
   ps_madd %f3,%f1,%f14,%f2
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_madd (exception persistence)
0: ps_madd %f3,%f1,%f11,%f2
   ps_madd %f3,%f28,%f2,%f1
   bl record
   fmr %f4,%f30
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_pnorm

   # ps_madd (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) * (normal,normal) + (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f2
   ps_merge00 %f13,%f0,%f1
   ps_madd %f3,%f5,%f4,%f13
   bl record
   fmr %f4,%f12
   fmr %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f2
   ps_merge00 %f4,%f0,%f1
   ps_madd %f3,%f5,%f13,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f2,%f24
   ps_merge00 %f4,%f0,%f1
   ps_madd %f3,%f13,%f5,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24
0: mtfsf 255,%f0              # -infinity * (0,normal) + +infinity
   ps_neg %f13,%f9
   ps_merge00 %f3,%f0,%f2
   ps_madd %f3,%f13,%f3,%f9
   bl record
   lfd %f4,168(%r31)
   fmr %f5,%f4
   bl add_fpscr_vximz         # Both invalid exceptions should be raised.
   bl add_fpscr_vxisi
   bl check_ps_nan

   # ps_madd (excess precision/range on input)
0: mtfsfi 7,1               # 1.333... * 1.5 + 1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd_ps %f4,312(%r31),%f0
   ps_madd %f3,%f3,%f13,%f4 # Should be exactly 2.0 even for single precision.
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... + 1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd_ps %f4,312(%r31),%f0
   ps_madd %f3,%f13,%f3,%f4 # Should round down due to frC rounding.
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
0: ps_neg %f13,%f7          # HUGE_VALF * 2 + -HUGE_VALF
   ps_madd %f3,%f7,%f2,%f13
   bl record
   fmr %f4,%f7
   fmr %f5,%f7
   bl check_ps_pnorm
0: ps_neg %f13,%f7          # 2 * HUGE_VALF + -HUGE_VALF
   ps_madd %f3,%f2,%f7,%f13
   bl record
   fmr %f4,%f7
   fmr %f5,%f7
   bl check_ps_pnorm
0: lfd_ps %f13,336(%r31),%f0  # 2^128 * 0.5 + 0
   lfd_ps %f4,64(%r31),%f0
   ps_madd %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f13,336(%r31),%f0  # 0.5 * 2^128 + 0
   lfd_ps %f4,64(%r31),%f0
   ps_madd %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # 1 * -HUGE_VALF + 2^128
   ps_neg %f13,%f7
   ps_madd %f3,%f1,%f13,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f5,%f7
   bl check_ps_pnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 1 + -HUGE_VAL
   fneg %f13,%f4
   stfd %f13,0(%r4)
   lfd_ps %f14,0(%r4),%f0
   ps_madd %f3,%f4,%f1,%f14
   bl record
   fmr %f4,%f0
   fmr %f5,%f0
   bl check_ps_pzero
0: lfd_ps %f4,320(%r31),%f0  # 1 * HUGE_VAL + -HUGE_VAL
   fneg %f13,%f4
   stfd %f13,0(%r4)
   lfd_ps %f14,0(%r4),%f0
   ps_madd %f3,%f1,%f4,%f14
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f0
   bl check_ps_pinf_inex
0: lfd_ps %f3,64(%r31),%f0  # 0.5 * HUGE_VAL + -HUGE_VAL
   lfd_ps %f4,320(%r31),%f0
   fneg %f13,%f4
   stfd %f13,0(%r4)
   lfd_ps %f13,0(%r4),%f0
   ps_madd %f3,%f3,%f4,%f13
   bl record
   fneg %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f0
   bl check_ps_ninf_inex
0: lfd_ps %f14,288(%r31),%f0  # 0 * 1.333... + 1
   ps_madd %f3,%f0,%f14,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 0 + 1
   ps_madd %f3,%f4,%f0,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,320(%r31),%f0  # 0 * HUGE_VAL + 1
   ps_madd %f3,%f0,%f4,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + -min_normal_f
   lfs %f3,12(%r30)
   ps_neg %f3,%f3
   ps_madd %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_nnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + 1
   ps_madd %f3,%f4,%f4,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + inf
   ps_madd %f3,%f4,%f4,%f9
   bl record
   fmr %f4,%f9
   fmr %f5,%f9
   bl check_ps_pinf
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + QNaN
   ps_madd %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   fmr %f5,%f10
   bl check_ps_nan

   # ps_madd.
0: ps_madd. %f3,%f1,%f11,%f2  # normal * SNaN + normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # normal * SNaN + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_madd. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_madds0
0: ps_merge00 %f4,%f1,%f7   # (normal,normal) * normal + (normal,normal) = (normal,overflow)
   ps_merge00 %f5,%f2,%f11  # NaN in second slot should be ignored.
   ps_madds0 %f3,%f4,%f5,%f4
   bl record
   fmr %f4,%f31
   fmr %f5,%f9
   bl add_fpscr_ox
   bl check_ps_pnorm_inex
0: ps_merge00 %f5,%f1,%f7   # (normal,normal) * NaN + (normal,normal) = (NaN,NaN)
   ps_merge00 %f4,%f11,%f2
   ps_madds0 %f3,%f5,%f4,%f5
   bl record
   fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal + (normal,normal), exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f2,%f11
   ps_madds0 %f3,%f5,%f13,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_madds0.
0: ps_merge00 %f5,%f1,%f7   # (normal,normal) * NaN + (normal,normal) = (NaN,NaN)
   ps_merge00 %f4,%f11,%f2
   ps_madds0. %f3,%f5,%f4,%f5
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal + (normal,normal), exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f2,%f11
   ps_madds0. %f3,%f5,%f13,%f1
   bl record
   mfcr %r3
   lis %r7,0x0F00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_madds1
0: ps_merge00 %f4,%f1,%f7   # (normal,normal) * normal = (normal,overflow)
   ps_merge00 %f5,%f11,%f2  # NaN in first slot should be ignored.
   ps_madds1 %f3,%f4,%f5,%f4
   bl record
   fmr %f4,%f31
   fmr %f5,%f9
   bl add_fpscr_ox
   bl check_ps_pnorm_inex
0: ps_merge00 %f5,%f1,%f7  # (normal,normal) * NaN + (normal,normal) = (NaN,NaN)
   ps_merge00 %f4,%f2,%f11
   ps_madds1 %f3,%f5,%f4,%f5
   bl record
   fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal + (normal,normal), exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f11,%f2
   ps_madds1 %f3,%f5,%f13,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_madds1.
0: ps_merge00 %f5,%f1,%f7  # (normal,normal) * NaN + (normal,normal) = (NaN,NaN)
   ps_merge00 %f4,%f2,%f11
   ps_madds1. %f3,%f5,%f4,%f5
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   fmr %f5,%f12
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # (normal,NaN) * normal + (normal,normal), exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f7,%f11
   ps_merge00 %f13,%f11,%f2
   ps_madds1. %f3,%f5,%f13,%f1
   bl record
   mfcr %r3
   lis %r7,0x0F00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_msub
0: ps_msub %f3,%f1,%f30,%f1   # normal * normal - normal
   bl record
   fmr %f4,%f24
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_msub %f3,%f0,%f1,%f2    # 0 * normal - normal
   bl record
   fneg %f4,%f2
   fmr %f5,%f4
   bl check_ps_nnorm
0: ps_neg %f4,%f0             # -0 * normal - 0
   ps_msub %f3,%f4,%f1,%f0
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_neg %f4,%f1             # 0 * -normal - 0
   ps_msub %f3,%f0,%f4,%f0
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_neg %f4,%f0             # 0 * normal - -0
   ps_msub %f3,%f0,%f1,%f4
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: lfs %f3,12(%r30)          # minimum_normal * minimum_normal - minimum_normal
   ps_msub %f3,%f3,%f3,%f3
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_nnorm_inex
0: ps_msub %f3,%f9,%f2,%f1    # +infinity * normal - normal
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f13,%f9            # +infinity * normal - -infinity
   ps_msub %f3,%f9,%f2,%f13
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f2,%f2             # +infinity * -normal - +infinity
   ps_msub %f3,%f9,%f2,%f9
   bl record
   ps_neg %f2,%f2
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_neg %f13,%f9            # -infinity * normal - -infinity
   ps_msub %f3,%f13,%f2,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f7             # huge * -huge - -infinity
   ps_neg %f3,%f9
   ps_msub %f3,%f7,%f4,%f3
   bl record
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_msub %f3,%f9,%f0,%f1    # +infinity * 0 - normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_msub %f3,%f9,%f0,%f9    # +infinity * 0 - infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f13,%f9            # 0 * -infinity - SNaN
   ps_msub %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_msub %f3,%f10,%f1,%f2   # QNaN * normal - normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_msub %f3,%f1,%f11,%f2   # normal * SNaN - normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_msub %f3,%f1,%f2,%f11   # normal * normal - SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_msub %f3,%f9,%f10,%f9   # +infinity * QNaN - infinity
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # SNaN * normal - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_msub %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # 0 * +infinity - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_msub %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # +infinity * normal - +infinity (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_msub %f3,%f9,%f2,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_msub (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_msub %f3,%f1,%f14,%f2
   mtfsb0 0
   ps_msub %f3,%f1,%f14,%f2
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_msub (exception persistence)
0: ps_msub %f3,%f1,%f11,%f2
   ps_msub %f3,%f1,%f30,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_pnorm

   # ps_msub (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) * (normal,normal) - (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f2
   ps_merge00 %f13,%f0,%f1
   ps_msub %f3,%f5,%f4,%f13
   bl record
   fmr %f4,%f12
   fmr %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f2
   ps_merge00 %f4,%f0,%f1
   ps_msub %f3,%f5,%f13,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f2,%f24
   ps_merge00 %f4,%f0,%f1
   ps_msub %f3,%f13,%f5,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_pinf
   mtfsb0 24

   # ps_msub (excess precision/range on input)
0: mtfsfi 7,1               # 1.333... * 1.5 - -1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd %f4,312(%r31)
   fneg %f4,%f4
   stfd %f4,0(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_msub %f3,%f3,%f13,%f4
   bl record
   fmr %f4,%f2
   fmr %f5,%f0
   bl check_ps_pnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... - -1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd %f4,312(%r31)
   fneg %f4,%f4
   stfd %f4,0(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_msub %f3,%f13,%f3,%f4
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fmr %f5,%f0
   bl check_ps_pnorm_inex
   mtfsfi 7,0
0: ps_msub %f3,%f7,%f2,%f7  # HUGE_VALF * 2 - HUGE_VALF
   bl record
   fmr %f4,%f7
   fmr %f5,%f7
   bl check_ps_pnorm
0: ps_msub %f3,%f2,%f7,%f7  # 2 * HUGE_VALF - HUGE_VALF
   bl record
   fmr %f4,%f7
   fmr %f5,%f7
   bl check_ps_pnorm
0: lfd_ps %f13,336(%r31),%f0  # 2^128 * 0.5 - 0
   lfd_ps %f4,64(%r31),%f0
   ps_msub %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f13,336(%r31),%f0  # 0.5 * 2^128 - 0
   lfd_ps %f4,64(%r31),%f0
   ps_msub %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   fmr %f5,%f0
   bl check_ps_pnorm
0: lfd_ps %f4,336(%r31),%f0  # 1 * HUGE_VALF - 2^128
   ps_msub %f3,%f1,%f7,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   fmr %f5,%f7
   bl check_ps_nnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 1 - HUGE_VAL
   ps_msub %f3,%f4,%f1,%f4
   bl record
   fmr %f4,%f0
   fmr %f5,%f0
   bl check_ps_pzero
0: lfd_ps %f4,320(%r31),%f0  # 1 * HUGE_VAL - HUGE_VAL
   ps_msub %f3,%f1,%f4,%f4
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f0
   bl check_ps_pinf_inex
0: lfd_ps %f3,64(%r31),%f0  # 0.5 * HUGE_VAL - HUGE_VAL
   lfd_ps %f4,320(%r31),%f0
   ps_msub %f3,%f3,%f4,%f4
   bl record
   fneg %f4,%f9
   bl add_fpscr_ox
   fmr %f5,%f0
   bl check_ps_ninf_inex
0: lfd_ps %f14,288(%r31),%f0  # 0 * 1.333... - 1
   ps_msub %f3,%f0,%f14,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 0 - 1
   ps_msub %f3,%f4,%f0,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f4,320(%r31),%f0  # 0 * HUGE_VAL - 1
   ps_msub %f3,%f0,%f4,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - min_normal_f
   lfs %f3,12(%r30)
   ps_msub %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   fneg %f4,%f4
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_nnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - 1
   ps_msub %f3,%f4,%f4,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - inf
   ps_msub %f3,%f4,%f4,%f9
   bl record
   fneg %f4,%f9
   fneg %f5,%f9
   bl check_ps_ninf
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - QNaN
   ps_msub %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   fmr %f5,%f10
   bl check_ps_nan

   # ps_msub.
0: ps_msub. %f3,%f1,%f11,%f2  # normal * SNaN - normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # normal * SNaN - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_msub. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_nmadd
0: ps_nmadd %f3,%f2,%f28,%f1  # normal * normal + normal
   bl record
   fneg %f4,%f30
   fmr %f5,%f4
   bl check_ps_nnorm
0: ps_nmadd %f3,%f0,%f1,%f2   # 0 * normal + normal
   bl record
   fneg %f4,%f2
   fmr %f5,%f4
   bl check_ps_nnorm
0: ps_neg %f4,%f0             # -0 * normal + 0
   ps_nmadd %f3,%f4,%f1,%f0
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_neg %f4,%f1             # 0 * -normal + 0
   ps_nmadd %f3,%f0,%f4,%f0
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_neg %f4,%f0             # 0 * normal + -0
   ps_nmadd %f3,%f0,%f1,%f4
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: lfs %f3,12(%r30)         # minimum_normal * minimum_normal + -minimum_normal
   ps_neg %f4,%f3
   ps_nmadd %f3,%f3,%f3,%f4
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_pnorm_inex
0: ps_nmadd %f3,%f9,%f2,%f1   # +infinity * normal + normal
   bl record
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_nmadd %f3,%f9,%f2,%f9   # +infinity * normal + +infinity
   bl record
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_neg %f13,%f9            # +infinity * -normal + -infinity
   ps_neg %f2,%f2
   ps_nmadd %f3,%f9,%f2,%f13
   bl record
   ps_neg %f2,%f2
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f13,%f9            # -infinity * normal + +infinity
   ps_nmadd %f3,%f13,%f2,%f9
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f7             # huge * -huge + +infinity
   ps_nmadd %f3,%f7,%f4,%f9
   bl record
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_nmadd %f3,%f9,%f0,%f1   # +infinity * 0 + normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f9             # +infinity * 0 + -infinity
   ps_nmadd %f3,%f9,%f0,%f4
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f13,%f9            # 0 * -infinity + SNaN
   ps_nmadd %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmadd %f3,%f10,%f1,%f2  # QNaN * normal + normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmadd %f3,%f1,%f11,%f2  # normal * SNaN + normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmadd %f3,%f1,%f2,%f11  # normal * normal + SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f9             # +infinity * QNaN + -infinity
   ps_nmadd %f3,%f9,%f10,%f4
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # SNaN * normal + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_nmadd %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # 0 * +infinity + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_nmadd %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # +infinity * normal + -infinity (exception enabled)
   mtfsb1 24
   ps_neg %f13,%f9
   ps_mr %f3,%f24
   ps_nmadd %f3,%f9,%f2,%f13
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_nmadd (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_nmadd %f3,%f1,%f14,%f2
   mtfsb0 0
   ps_nmadd %f3,%f1,%f14,%f2
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_nmadd (exception persistence)
0: ps_nmadd %f3,%f1,%f11,%f2
   ps_nmadd %f3,%f28,%f2,%f1
   bl record
   fneg %f4,%f30
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nnorm

   # ps_nmadd (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) * (normal,normal) + (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f2
   ps_merge00 %f13,%f0,%f1
   ps_nmadd %f3,%f5,%f4,%f13
   bl record
   fmr %f4,%f12
   fneg %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f2
   ps_merge00 %f4,%f0,%f1
   ps_nmadd %f3,%f5,%f13,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f2,%f24
   ps_merge00 %f4,%f0,%f1
   ps_nmadd %f3,%f13,%f5,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_ninf
   mtfsb0 24

   # ps_nmadd (excess precision/range on input)
0: mtfsfi 7,1               # 1.333... * 1.5 + 1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd_ps %f4,312(%r31),%f0
   ps_nmadd %f3,%f3,%f13,%f4
   bl record
   fneg %f4,%f2
   fneg %f5,%f0
   bl check_ps_nnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... + 1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd_ps %f4,312(%r31),%f0
   ps_nmadd %f3,%f13,%f3,%f4
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fneg %f4,%f4
   fneg %f5,%f0
   bl check_ps_nnorm_inex
   mtfsfi 7,0
0: ps_neg %f13,%f7          # HUGE_VALF * 2 + -HUGE_VALF
   ps_nmadd %f3,%f7,%f2,%f13
   bl record
   fneg %f4,%f7
   fneg %f5,%f7
   bl check_ps_nnorm
0: ps_neg %f13,%f7          # 2 * HUGE_VALF + -HUGE_VALF
   ps_nmadd %f3,%f2,%f7,%f13
   bl record
   fneg %f4,%f7
   fneg %f5,%f7
   bl check_ps_nnorm
0: lfd_ps %f13,336(%r31),%f0  # 2^128 * 0.5 + 0
   lfd_ps %f4,64(%r31),%f0
   ps_nmadd %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   fneg %f5,%f0
   bl check_ps_nnorm
0: lfd_ps %f13,336(%r31),%f0  # 0.5 * 2^128 + 0
   lfd_ps %f4,64(%r31),%f0
   ps_nmadd %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   fneg %f5,%f0
   bl check_ps_nnorm
0: lfd_ps %f4,336(%r31),%f0  # 1 * -HUGE_VALF + 2^128
   ps_neg %f13,%f7
   ps_nmadd %f3,%f1,%f13,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f4,%f4
   fmr %f5,%f7
   bl check_ps_nnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 1 + -HUGE_VAL
   fneg %f14,%f4
   stfd %f14,0(%r4)
   lfd_ps %f14,0(%r4),%f0
   ps_nmadd %f3,%f4,%f1,%f14
   bl record
   fneg %f4,%f0
   fneg %f5,%f0
   bl check_ps_nzero
0: lfd_ps %f4,320(%r31),%f0  # 1 * HUGE_VAL + -HUGE_VAL
   fneg %f14,%f4
   stfd %f14,0(%r4)
   lfd_ps %f14,0(%r4),%f0
   ps_nmadd %f3,%f1,%f4,%f14
   bl record
   fneg %f4,%f9
   bl add_fpscr_ox
   fneg %f5,%f0
   bl check_ps_ninf_inex
0: lfd_ps %f3,64(%r31),%f0  # 0.5 * HUGE_VAL + -HUGE_VAL
   lfd_ps %f4,320(%r31),%f0
   fneg %f13,%f4
   stfd %f13,0(%r4)
   lfd_ps %f13,0(%r4),%f0
   ps_nmadd %f3,%f1,%f3,%f13
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fneg %f5,%f0
   bl check_ps_pinf_inex
0: lfd_ps %f14,288(%r31),%f0  # 0 * 1.333... + 1
   ps_nmadd %f3,%f0,%f14,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 0 + 1
   ps_nmadd %f3,%f4,%f0,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f4,320(%r31),%f0  # 0 * HUGE_VAL + 1
   ps_nmadd %f3,%f0,%f4,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + -min_normal_f
   lfs %f3,12(%r30)
   ps_neg %f3,%f3
   ps_nmadd %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_pnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + 1
   ps_nmadd %f3,%f4,%f4,%f1
   bl record
   fneg %f4,%f1
   fneg %f5,%f1
   bl check_ps_nnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + inf
   ps_nmadd %f3,%f4,%f4,%f9
   bl record
   fneg %f4,%f9
   fneg %f5,%f9
   bl check_ps_ninf
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d + QNaN
   ps_nmadd %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   fmr %f5,%f10
   bl check_ps_nan

   # ps_nmadd.
0: ps_nmadd. %f3,%f1,%f11,%f2  # normal * SNaN + normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # normal * SNaN + normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_nmadd. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_nmsub
0: ps_nmsub %f3,%f1,%f30,%f1  # normal * normal - normal
   bl record
   fneg %f4,%f24
   fmr %f5,%f4
   bl check_ps_nnorm
0: ps_nmsub %f3,%f0,%f1,%f2   # 0 * normal - normal
   bl record
   fmr %f4,%f2
   fmr %f5,%f4
   bl check_ps_pnorm
0: ps_neg %f4,%f0             # -0 * normal - 0
   ps_nmsub %f3,%f4,%f1,%f0
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_neg %f4,%f1             # 0 * -normal - 0
   ps_nmsub %f3,%f0,%f4,%f0
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_neg %f4,%f0             # 0 * normal - -0
   ps_nmsub %f3,%f0,%f1,%f4
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: lfs %f3,12(%r30)          # minimum_normal * minimum_normal - minimum_normal
   ps_nmsub %f3,%f3,%f3,%f3
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_pnorm_inex
0: ps_nmsub %f3,%f9,%f2,%f1   # +infinity * normal - normal
   bl record
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_neg %f13,%f9            # +infinity * normal - -infinity
   ps_nmsub %f3,%f9,%f2,%f13
   bl record
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_neg %f2,%f2             # +infinity * -normal - +infinity
   ps_nmsub %f3,%f9,%f2,%f9
   bl record
   ps_neg %f2,%f2
   fmr %f4,%f9
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f13,%f9            # -infinity * normal - -infinity
   ps_nmsub %f3,%f13,%f2,%f13
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f4,%f7             # huge * -huge - -infinity
   ps_neg %f3,%f9
   ps_nmsub %f3,%f7,%f4,%f3
   bl record
   fneg %f4,%f9
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_nmsub %f3,%f9,%f0,%f1   # +infinity * 0 - normal
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmsub %f3,%f9,%f0,%f9   # +infinity * 0 - infinity
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_nan
0: ps_neg %f13,%f9            # 0 * -infinity - SNaN
   ps_nmsub %f3,%f0,%f13,%f11
   bl record
   fmr %f4,%f12
   bl add_fpscr_vximz
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmsub %f3,%f10,%f1,%f2  # QNaN * normal - normal
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmsub %f3,%f1,%f11,%f2  # normal * SNaN - normal
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmsub %f3,%f1,%f2,%f11  # normal * normal - SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: ps_nmsub %f3,%f9,%f10,%f9  # +infinity * QNaN - infinity
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # SNaN * normal - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_nmsub %f3,%f11,%f1,%f2
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # 0 * +infinity - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_nmsub %f3,%f0,%f9,%f1
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vximz
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0              # +infinity * normal - +infinity (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_nmsub %f3,%f9,%f2,%f9
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxisi
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   # ps_nmsub (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_nmsub %f3,%f1,%f11,%f2
   mtfsb0 0
   ps_nmsub %f3,%f1,%f11,%f2
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_nmsub (exception persistence)
0: ps_nmsub %f3,%f1,%f11,%f2
   ps_nmsub %f3,%f30,%f1,%f1
   bl record
   fneg %f4,%f24
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nnorm

   # ps_nmsub (different values in slots)
0: ps_merge00 %f5,%f11,%f7  # (NaN,normal) * (normal,normal) - (normal,normal) = (NaN,overflow)
   ps_merge00 %f4,%f24,%f2
   ps_merge00 %f13,%f0,%f1
   ps_nmsub %f3,%f5,%f4,%f13
   bl record
   fmr %f4,%f12
   fneg %f5,%f9
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f5,%f11,%f7
   ps_merge00 %f13,%f24,%f2
   ps_merge00 %f4,%f0,%f1
   ps_nmsub %f3,%f5,%f13,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_mr %f3,%f1
   ps_merge00 %f13,%f7,%f11
   ps_merge00 %f5,%f2,%f24
   ps_merge00 %f4,%f0,%f1
   ps_nmsub %f3,%f13,%f5,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_xx_fi
   bl check_ps_ninf
   mtfsb0 24

   # ps_nmsub (excess precision/range on input)
0: mtfsfi 7,1               # 1.333... * 1.5 - -1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd %f4,312(%r31)
   fneg %f4,%f4
   stfd %f4,0(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_nmsub %f3,%f3,%f13,%f4
   bl record
   fneg %f4,%f2
   fneg %f5,%f0
   bl check_ps_nnorm
   mtfsfi 7,0
0: mtfsfi 7,1               # 1.5 * 1.333... - -1ulp (round toward zero)
   lfd_ps %f3,288(%r31),%f0
   lfd_ps %f13,296(%r31),%f0
   lfd %f4,312(%r31)
   fneg %f4,%f4
   stfd %f4,0(%r4)
   lfd_ps %f4,0(%r4),%f0
   ps_nmsub %f3,%f13,%f3,%f4
   bl record
   lis %r3,0x4000
   addi %r3,%r3,-1
   stw %r3,0(%r4)
   lfs %f4,0(%r4)
   fneg %f4,%f4
   fneg %f5,%f0
   bl check_ps_nnorm_inex
   mtfsfi 7,0
0: ps_nmsub %f3,%f7,%f2,%f7  # HUGE_VALF * 2 - HUGE_VALF
   bl record
   fneg %f4,%f7
   fneg %f5,%f7
   bl check_ps_nnorm
0: ps_nmsub %f3,%f2,%f7,%f7  # 2 * HUGE_VALF - HUGE_VALF
   bl record
   fneg %f4,%f7
   fneg %f5,%f7
   bl check_ps_nnorm
0: lfd_ps %f13,336(%r31),%f0  # 2^128 * 0.5 - 0
   lfd_ps %f4,64(%r31),%f0
   ps_nmsub %f3,%f13,%f4,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   fneg %f5,%f0
   bl check_ps_nnorm
0: lfd_ps %f13,336(%r31),%f0  # 0.5 * 2^128 - 0
   lfd_ps %f4,64(%r31),%f0
   ps_nmsub %f3,%f4,%f13,%f0
   bl record
   lfd %f4,328(%r31)
   fneg %f4,%f4
   fneg %f5,%f0
   bl check_ps_nnorm
0: lfd_ps %f4,336(%r31),%f0  # 1 * HUGE_VALF - 2^128
   ps_nmsub %f3,%f1,%f7,%f4
   bl record
   lfd %f4,344(%r31)
   fneg %f5,%f7
   bl check_ps_pnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 1 - HUGE_VAL
   ps_nmsub %f3,%f4,%f1,%f4
   bl record
   fneg %f4,%f0
   fneg %f5,%f0
   bl check_ps_nzero
0: lfd_ps %f4,320(%r31),%f0  # 1 * HUGE_VAL - HUGE_VAL
   ps_nmsub %f3,%f1,%f4,%f4
   bl record
   fneg %f4,%f9
   bl add_fpscr_ox
   fneg %f5,%f0
   bl check_ps_ninf_inex
0: lfd_ps %f3,64(%r31),%f0  # 0.5 * HUGE_VAL - HUGE_VAL
   lfd_ps %f4,320(%r31),%f0
   ps_nmsub %f3,%f3,%f4,%f4
   bl record
   fmr %f4,%f9
   bl add_fpscr_ox
   fneg %f5,%f0
   bl check_ps_pinf_inex
0: lfd_ps %f14,288(%r31),%f0  # 0 * 1.333... - 1
   ps_nmsub %f3,%f0,%f14,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,320(%r31),%f0  # HUGE_VAL * 0 - 1
   ps_nmsub %f3,%f4,%f0,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,320(%r31),%f0  # 0 * HUGE_VAL - 1
   ps_nmsub %f3,%f0,%f4,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - min_normal_f
   lfs %f3,12(%r30)
   ps_nmsub %f3,%f4,%f4,%f3
   bl record
   lfs %f4,12(%r30)
   bl add_fpscr_ux
   fmr %f5,%f4
   bl check_ps_pnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - 1
   ps_nmsub %f3,%f4,%f4,%f1
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm_inex
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - inf
   ps_nmsub %f3,%f4,%f4,%f9
   bl record
   fmr %f4,%f9
   fmr %f5,%f9
   bl check_ps_pinf
0: lfd_ps %f4,376(%r31),%f0  # min_denormal_d * min_denormal_d - QNaN
   ps_nmsub %f3,%f4,%f4,%f10
   bl record
   fmr %f4,%f10
   fmr %f5,%f10
   bl check_ps_nan

   # ps_nmsub.
0: ps_nmsub. %f3,%f1,%f11,%f2  # normal * SNaN - normal
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0              # normal * SNaN - normal (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_nmsub. %f3,%f1,%f3,%f2
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

   ########################################################################
   # Paired-single miscellaneous mathematical instructions
   ########################################################################

   # Load constants.
0: lfd %f24,80(%r31)    # 4194304.0
   lfd %f25,176(%r31)   # 2048.0
   lfd %f26,16(%r31)    # +infinity
   fneg %f27,%f26       # -infinity
   lfd %f28,64(%r31)    # 0.5
   lfd %f29,56(%r31)    # 0.25
   stfs %f24,0(%r4)
   lfs %f24,0(%r4)
   stfs %f25,0(%r4)
   lfs %f25,0(%r4)
   stfs %f26,0(%r4)
   lfs %f26,0(%r4)
   stfs %f27,0(%r4)
   lfs %f27,0(%r4)
   stfs %f28,0(%r4)
   lfs %f28,0(%r4)
   stfs %f29,0(%r4)
   lfs %f29,0(%r4)

   # ps_res
0: ps_res %f3,%f2       # +normal
   bl record
   lfd %f4,256(%r31)
   fmr %f5,%f4
   lfd %f6,264(%r31)
   fmr %f7,%f6
   bl check_ps_estimate_pnorm
0: ps_neg %f3,%f2       # -normal
   ps_res %f4,%f3
   bl record
   ps_mr %f3,%f4
   lfd %f4,264(%r31)
   fneg %f4,%f4
   fmr %f5,%f4
   lfd %f6,256(%r31)
   fneg %f6,%f6
   fmr %f7,%f6
   bl check_ps_estimate_nnorm
0: lfs %f13,44(%r30)    # +tiny
   ps_res %f3,%f13
   bl record
   lfd %f4,152(%r31)
   bl add_fpscr_ox
   bl add_fpscr_fi
   fmr %f5,%r4
   bl check_ps_pnorm
0: lfs %f13,48(%r30)    # -tiny
   ps_res %f4,%f13
   bl record
   ps_mr %f3,%f4
   lfd %f4,152(%r31)
   fneg %f4,%f4
   bl add_fpscr_ox
   bl add_fpscr_fi
   fmr %f5,%r4
   bl check_ps_nnorm
0: lfd %f14,152(%r31)   # +huge
   stfs %f14,0(%r4)
   lfs %f14,0(%r4)
   ps_res %f3,%f14
   bl record
   lfs %f4,56(%r30)
   fmr %f5,%f4
   lfs %f6,60(%r30)
   fmr %f7,%f6
   bl add_fpscr_ux
   bl check_ps_estimate_pdenorm
0: lfs %f14,0(%r4)      # -huge
   ps_neg %f14,%f14
   ps_res %f4,%f14
   bl record
   ps_mr %f3,%f4
   lfs %f4,60(%r30)
   fneg %f4,%f4
   fmr %f5,%f4
   lfs %f6,56(%r30)
   fneg %f6,%f6
   fmr %f7,%f6
   bl add_fpscr_ux
   bl check_ps_estimate_ndenorm
0: ps_res %f3,%f26      # +infinity
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_res %f3,%f27      # -infinity
   bl record
   fneg %f4,%f0
   fmr %f5,%f4
   bl check_ps_nzero
0: ps_res %f3,%f0       # +0
   bl record
   fmr %f4,%f9
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f3,%f0       # -0
   ps_res %f3,%f3
   bl record
   fneg %f4,%f9
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_res %f3,%f10      # QNaN
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_res %f3,%f11      # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f5,%f11
   ps_mr %f3,%f24
   ps_res %f3,%f5
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsb1 27            # 0 (exception enabled)
   ps_mr %f3,%f24
   ps_mr %f4,%f0
   ps_res %f3,%f4
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 27

   # ps_res (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_res %f3,%f14
   mtfsb0 0
   ps_res %f3,%f14
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_res (exception persistence)
0: ps_res %f3,%f11
   ps_mr %f15,%f2
   ps_res %f3,%f15
   bl record
   lfd %f4,256(%r31)
   fmr %f5,%f4
   lfd %f6,264(%r31)
   fmr %f7,%f6
   bl add_fpscr_vxsnan
   bl check_ps_estimate_pnorm

   # ps_res (different values in slots)
0: lfs %f3,44(%r30)     # 1 / (SNaN,normal) = (NaN,overflow)
   ps_merge00 %f3,%f11,%f3
   ps_res %f14,%f3
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f12
   lfd %f5,152(%r31)
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_fi
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   lfs %f3,44(%r30)
   ps_merge00 %f4,%f11,%f3
   ps_mr %f14,%f1
   ps_res %f14,%f4
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_fi
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   lfs %f3,44(%r30)
   ps_merge00 %f5,%f3,%f11
   ps_mr %f14,%f1
   ps_res %f14,%f5
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl add_fpscr_ox
   bl add_fpscr_fi
   bl check_ps_pnorm
   mtfsb0 24
0: mtfsf 255,%f0        # 1 / (normal,0) (exception enabled)
   mtfsb1 27
   ps_merge00 %f13,%f2,%f0
   ps_mr %f14,%f1
   ps_res %f14,%f13
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_zx
   bl check_ps_pnorm
   mtfsb0 27

   # ps_res.
0: ps_res. %f3,%f11     # SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_res. %f3,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

.if TEST_RECIPROCAL_TABLES

   # If frD[ps0] and FPSCR match the expected output but frD[ps1] does not,
   # these tests will store frD[ps1] instead of frD[ps0] to word 2 of the
   # failure record and set the lowest bit of word 4 (nominally the value
   # of FPSCR after the instruction under test) to indicate that fact.

   # ps_res (precise output: basic tests)
0: bl 1f
1: mflr %r3
   addi %r14,%r3,2f-1b
   addi %r15,%r3,0f-1b
   b 0f
   # 12 bytes per entry: input, output, expected FPSCR if in ps0
2: .int 0x40000000,0x3EFFF800,0x00004000
   .int 0xC0000000,0xBEFFF800,0x00008000
   .int 0x7F7FFFFF,0x00200020,0x88034000
   .int 0xFF7FFFFF,0x80200020,0x88038000
   .int 0x00000001,0x7F7FFFFF,0x90024000
   .int 0x80000001,0xFF7FFFFF,0x90028000
   .int 0x001FFFFF,0x7F7FFFFF,0x90024000
   .int 0x801FFFFF,0xFF7FFFFF,0x90028000
   .int 0x00200000,0x7F7FF800,0x00004000
   .int 0x80200000,0xFF7FF800,0x00008000
0: lfs %f3,0(%r14)
   mtfsf 255,%f0
   ps_res %f5,%f3
   bl record
   mffs %f4
   stfd %f4,0(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   lwz %r7,4(%r14)
   lwz %r8,4(%r4)
   lwz %r9,8(%r14)
   cmpw %r3,%r7
   bne 1f
   cmpw %r8,%r9
   bne 1f
   lwz %r3,12(%r4)
   cmpw %r3,%r7
   beq 2f
   # Low bit of word 4 (FPSCR) set indicates that the error was in ps1.
   ori %r8,%r8,1
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r8,16(%r6)
   lwz %r3,0(%r14)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: addi %r14,%r14,12
   cmpw %r14,%r15
   blt 0b

   # ps_res (precise output: all base and delta values)
0: lis %r14,0x4000
   lis %r15,0x4080
   lis %r12,0x0001
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f4,%f3
   ps_res %f5,%f4
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,16(%r4)
   lwz %r7,20(%r4)
   bne 1f
   cmpw %r7,%r11
   bne 1f
   lwz %r3,12(%r4)
   lwz %r8,4(%r4)
   cmpw %r3,%r8
   beq 2f
   ori %r7,%r7,1
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_res (precise output: input precision and FI)
0: lis %r14,0x4000
   ori %r15,%r14,0x200
   li %r12,1
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f5,%f3
   ps_res %f5,%f5
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,16(%r4)
   lwz %r7,20(%r4)
   bne 1f
   cmpw %r7,%r11
   bne 1f
   lwz %r3,12(%r4)
   lwz %r8,4(%r4)
   cmpw %r3,%r8
   beq 2f
   ori %r7,%r7,1
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_res (precise output: denormal inputs)
0: lis %r14,0x0010
   lis %r15,0x00A0
   lis %r12,0x10
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f13,%f3
   ps_res %f5,%f13
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,16(%r4)
   lwz %r7,20(%r4)
   bne 1f
   cmpw %r7,%r11
   bne 1f
   lwz %r3,12(%r4)
   lwz %r8,4(%r4)
   cmpw %r3,%r8
   beq 2f
   ori %r7,%r7,1
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_res (precise output: input precision for denormals)
0: lis %r14,0x003F
   ori %r14,%r14,0xFE00
   lis %r15,0x0040
   ori %r15,%r15,0x0200
   li %r12,0x20
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f14,%f3
   ps_res %f5,%f14
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,16(%r4)
   lwz %r7,20(%r4)
   bne 1f
   cmpw %r7,%r11
   bne 1f
   lwz %r3,12(%r4)
   lwz %r8,4(%r4)
   cmpw %r3,%r8
   beq 2f
   ori %r7,%r7,1
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_res (precise output: denormal outputs)
0: lis %r14,0x7E00
   lis %r15,0x7F80
   lis %r12,0x10
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_fres
   stfs %f4,4(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f15,%f3
   ps_res %f5,%f15
   bl record
   lwz %r7,4(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   cmpw %r3,%r7
   mffs %f4
   stfd %f4,16(%r4)
   lwz %r7,20(%r4)
   bne 1f
   cmpw %r7,%r11
   bne 1f
   lwz %r3,12(%r4)
   lwz %r8,4(%r4)
   cmpw %r3,%r8
   beq 2f
   ori %r7,%r7,1
1: stw %r3,8(%r6)
   lwz %r3,4(%r4)
   stw %r3,12(%r6)
   stw %r7,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

.endif  # TEST_RECIPROCAL_TABLES

   # ps_rsqrte
0: ps_rsqrte %f3,%f29   # +normal
   bl record
   lfd %f4,272(%r31)
   fmr %f5,%f4
   lfd %f6,280(%r31)
   fmr %f7,%f6
   bl check_ps_estimate_pnorm
0: ps_rsqrte %f3,%f26   # +infinity
   bl record
   fmr %f4,%f0
   fmr %f5,%f4
   bl check_ps_pzero
0: ps_rsqrte %f3,%f0    # +0
   bl record
   fmr %f4,%f9
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_pinf
0: ps_neg %f3,%f0
   ps_rsqrte %f3,%f3    # -0
   bl record
   fneg %f4,%f9
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_ninf
0: ps_neg %f6,%f29      # -normal
   ps_rsqrte %f3,%f6
   bl record
   lfd %f4,168(%r31)
   bl add_fpscr_vxsqrt
   fmr %f5,%f4
   bl check_ps_nan
0: ps_rsqrte %f3,%f10   # QNaN
   bl record
   fmr %f4,%f10
   fmr %f5,%f4
   bl check_ps_nan
0: ps_rsqrte %f3,%f11   # SNaN
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f5,%f11
   ps_mr %f3,%f24
   ps_rsqrte %f3,%f5
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # -infinity (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f24
   ps_rsqrte %f3,%f27
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_vxsqrt
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24
0: mtfsb1 27            # -0 (exception enabled)
   ps_mr %f3,%f24
   ps_neg %f4,%f0
   ps_rsqrte %f3,%f4
   bl record
   fmr %f4,%f24
   bl add_fpscr_fex
   bl add_fpscr_zx
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 27

   # ps_rsqrte (FPSCR[FX] handling)
0: ps_mr %f14,%f11
   ps_rsqrte %f3,%f14
   mtfsb0 0
   ps_rsqrte %f3,%f14
   bl record
   fmr %f4,%f12
   bl add_fpscr_vxsnan
   bl remove_fpscr_fx
   fmr %f5,%f4
   bl check_ps_nan

   # ps_rsqrte (exception persistence)
0: ps_rsqrte %f3,%f11
   ps_mr %f15,%f29
   ps_rsqrte %f3,%f15
   bl record
   lfd %f4,272(%r31)
   fmr %f5,%f4
   lfd %f6,280(%r31)
   fmr %f7,%f6
   bl add_fpscr_vxsnan
   bl check_ps_estimate_pnorm

   # ps_rsqrte (different values in slots)
0: ps_merge00 %f3,%f11,%f2  # 1 / sqrt(SNaN,normal) = (NaN,normal)
   ps_rsqrte %f14,%f3
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f12
   lis %r3,0x3F34  # Exact result from the 750CL, for simplicity.
   ori %r3,%r3,0xFD00
   stw %r3,0(%r4)
   lfs %f5,0(%r4)
   bl add_fpscr_vxsnan
   bl check_ps_nan
0: mtfsf 255,%f0        # Same, with exception enabled
   mtfsb1 24
   ps_merge00 %f4,%f11,%f2
   ps_mr %f14,%f1
   ps_rsqrte %f14,%f4
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_ps_noresult
   mtfsb0 24
0: mtfsf 255,%f0        # Same, with NaN in the second slot
   mtfsb1 24
   ps_merge00 %f5,%f2,%f11
   ps_mr %f14,%f1
   ps_rsqrte %f14,%f5
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   bl check_ps_pnorm
   mtfsb0 24
0: mtfsf 255,%f0        # 1 / sqrt(normal,0) (exception enabled)
   mtfsb1 27
   ps_merge00 %f13,%f2,%f0
   ps_mr %f14,%f1
   ps_rsqrte %f14,%f13
   bl record
   ps_mr %f3,%f14
   fmr %f4,%f1
   fmr %f5,%f1
   bl add_fpscr_fex
   bl add_fpscr_zx
   bl check_ps_pnorm
   mtfsb0 27

   # ps_rsqrte (excess range on input)
0: mtfsf 255,%f0
   lfd %f3,320(%r31)    # HUGE_VAL
   isync
   ps_merge00 %f3,%f3,%f3
   isync
   lfd %f3,320(%r31)
   isync
   ps_rsqrte %f13,%f3
   bl record
   mffs %f4
   stfd %f4,8(%r4)
   isync
   stfd %f13,0(%r4)
   isync
   ps_merge11 %f3,%f13,%f13
   stfs %f3,8(%r4)
   lwz %r3,0(%r4)
   lwz %r7,4(%r4)
   lwz %r8,8(%r4)
   lwz %r9,12(%r4)
   # The mantissa of ps0 is rounded to single precision, but the exponent
   # keeps its full double-precision range.
   lis %r10,0x1FF0
   ori %r10,%r10,0x0008
   lis %r11,0x2000
   # ps1's exponent gets reset to 0 (unbiased).
   lis %r12,0x3F80
   ori %r12,%r12,0x0041
   cmpw %r3,%r10
   bne 1f
   cmpw %r7,%r11
   bne 1f
   cmpw %r8,%r12
   bne 1f
   cmpwi %r9,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: mtfsf 255,%f0
   lfd %f3,376(%r31)    # Minimum double-precision denormal
   isync
   ps_merge00 %f3,%f3,%f3
   isync
   lfd %f4,376(%r31)
   isync
   ps_rsqrte %f13,%f4
   bl record
   mffs %f4
   stfd %f4,8(%r4)
   isync
   stfd %f13,0(%r4)
   isync
   ps_merge11 %f3,%f13,%f13
   stfs %f3,8(%r4)
   lwz %r3,0(%r4)
   lwz %r7,4(%r4)
   lwz %r8,8(%r4)
   lwz %r9,12(%r4)
   # ps0's exponent still keeps its full range.
   lis %r10,0x617F
   ori %r10,%r10,0xFE80
   li %r11,0
   # ps1's exponent gets reset, in this case to -1.
   lis %r12,0x3F7F
   ori %r12,%r12,0xF400
   cmpw %r3,%r10
   bne 1f
   cmpw %r7,%r11
   bne 1f
   cmpw %r8,%r12
   bne 1f
   cmpwi %r9,0x4000
   beq 0f
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32

   # ps_rsqrte.
0: mtcr %r0
   mtfsf 255,%f0
   ps_rsqrte. %f3,%f11  # SNaN
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f12
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_nan
0: mtfsf 255,%f0        # SNaN (exception enabled)
   mtfsb1 24
   ps_mr %f3,%f11
   ps_rsqrte. %f3,%f3
   bl record
   mfcr %r3
   lis %r7,0x0E00
   cmpw %r3,%r7
   beq 1f
   stw %r3,16(%r6)
   li %r3,-1
   stw %r3,8(%r6)
   addi %r6,%r6,32
   mtcr %r0
   mtfsf 255,%f0
   b 0f
1: fmr %f4,%f11
   bl add_fpscr_fex
   bl add_fpscr_vxsnan
   fmr %f5,%f4
   bl check_ps_noresult
   mtfsb0 24

.if TEST_RECIPROCAL_TABLES

   # As for ps_res tests, a set low bit in word 4 of the failure record
   # indicates that an error was detected in frD[ps1] rather than frD[ps0]
   # or FPSCR, and (since ps_rsqrte's inputs are single-precision) the
   # input is stored to word 5 of the failure record.

   # ps_rsqrte (precise output: basic tests)
0: bl 1f
1: mflr %r3
   addi %r14,%r3,2f-1b
   addi %r15,%r3,0f-1b
   b 0f
   # 12 bytes per entry: input, output, expected FPSCR
2: .int 0x40000000, 0x3F34FD00, 0x00004000
   .int 0x40800000, 0x3EFFF400, 0x00004000
   .int 0x7F7FFFFF, 0x1F800041, 0x00004000
   .int 0x00800000, 0x5EFFF400, 0x00004000
   .int 0x00000001, 0x64B4FD00, 0x00004000
0: lfs %f3,0(%r14)
   mtfsf 255,%f0
   ps_rsqrte %f5,%f3
   bl record
   mffs %f4
   stfd %f4,0(%r4)
   stfs %f5,8(%r4)
   ps_merge10 %f5,%f5,%f5
   stfs %f5,12(%r4)
   lwz %r3,8(%r4)
   lwz %r7,4(%r14)
   lwz %r10,4(%r4)
   lwz %r11,8(%r14)
   cmpw %r3,%r7
   bne 1f
   cmpw %r10,%r11
   bne 1f
   lwz %r3,12(%r4)
   cmpw %r3,%r7
   beq 2f
   ori %r10,%r10,1
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r14)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: addi %r14,%r14,12
   cmpw %r14,%r15
   blt 0b

   # ps_rsqrte (precise output: all base and delta values)
0: lis %r14,0x4080
   lis %r15,0x4180
   lis %r12,2
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_ps_rsqrte
   stfs %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f4,%f3
   ps_rsqrte %f5,%f4
   bl record
   stfs %f5,16(%r4)
   ps_merge11 %f5,%f5,%f5
   stfs %f5,20(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r10,%r11
   bne 1f
   lwz %r3,20(%r4)
   cmpw %r3,%r7
   beq 2f
   ori %r10,%r10,1
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_rsqrte (precise output: input precision)
0: lis %r14,0x40FF
   ori %r14,%r14,0x8000
   lis %r15,0x4100
   ori %r14,%r14,0x8000
   li %r12,0x80
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_ps_rsqrte
   stfs %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f5,%f3
   ps_rsqrte %f5,%f5
   bl record
   stfs %f5,16(%r4)
   ps_merge11 %f5,%f5,%f5
   stfs %f5,20(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r10,%r11
   bne 1f
   lwz %r3,20(%r4)
   cmpw %r3,%r7
   beq 2f
   ori %r10,%r10,1
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_rsqrte (precise output: denormal inputs)
0: lis %r14,0x0010
   lis %r15,0x00A0
   lis %r12,0x10
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_ps_rsqrte
   stfs %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f13,%f3
   ps_rsqrte %f5,%f13
   bl record
   stfs %f5,16(%r4)
   ps_merge11 %f5,%f5,%f5
   stfs %f5,20(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r10,%r11
   bne 1f
   lwz %r3,20(%r4)
   cmpw %r3,%r7
   beq 2f
   ori %r10,%r10,1
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

   # ps_rsqrte (precise output: input precision for denormals)
0: lis %r14,0x001F
   ori %r14,%r14,0xFE00
   lis %r15,0x0020
   ori %r15,%r15,0x0200
   li %r12,0x10
0: stw %r14,0(%r4)
   lfs %f3,0(%r4)
   bl calc_ps_rsqrte
   stfs %f4,8(%r4)
   mr %r11,%r3
   mtfsf 255,%f0
   ps_mr %f14,%f3
   ps_rsqrte %f5,%f14
   bl record
   stfs %f5,16(%r4)
   ps_merge11 %f5,%f5,%f5
   stfs %f5,20(%r4)
   lwz %r3,16(%r4)
   lwz %r7,8(%r4)
   mffs %f4
   stfd %f4,24(%r4)
   lwz %r10,28(%r4)
   cmpw %r3,%r7
   bne 1f
   cmpw %r10,%r11
   bne 1f
   lwz %r3,20(%r4)
   cmpw %r3,%r7
   beq 2f
   ori %r10,%r10,1
1: stw %r3,8(%r6)
   stw %r7,12(%r6)
   stw %r10,16(%r6)
   lwz %r3,0(%r4)
   stw %r3,20(%r6)
   addi %r6,%r6,32
2: add %r14,%r14,%r12
   cmpw %r14,%r15
   blt 0b

.endif  # TEST_RECIPROCAL_TABLES

   ########################################################################
   # Paired-single select instruction
   ########################################################################

   # ps_sel
0: ps_merge00 %f4,%f2,%f0   # (+normal,+0)
   ps_merge00 %f5,%f1,%f2
   ps_sel %f3,%f4,%f5,%f0
   bl record
   fmr %f4,%f1
   fmr %f5,%f2
   bl check_ps
0: ps_neg %f3,%f0           # (-0,-normal)
   ps_neg %f4,%f1
   ps_merge00 %f4,%f3,%f4
   ps_merge00 %f5,%f1,%f0
   ps_merge00 %f6,%f0,%f2
   ps_sel %f3,%f4,%f5,%f6
   bl record
   fmr %f4,%f1
   fmr %f5,%f2
   bl check_ps
0: mtfsf 255,%f0            # (QNaN,SNaN)
   ps_merge00 %f4,%f10,%f11
   ps_merge00 %f5,%f1,%f2
   ps_sel %f3,%f4,%f0,%f5
   bl record
   mffs %f4                 # The SNaN should not generate an exception.
   stfd %f4,0(%r4)
   lwz %r3,4(%r4)
   fadd %f4,%f0,%f0         # Make sure the SNaN doesn't leak an exception
   mffs %f4                 # to the next FPU operation either.
   stfd %f4,0(%r4)
   lwz %r7,4(%r4)
   cmpwi %r3,0
   bne 1f
   cmpwi %r7,0x2000
   beq 2f
1: psq_st %f3,8(%r6),0,0
   stw %r3,16(%r6)
   stw %r7,20(%r6)
   addi %r6,%r6,32
   b 0f
2: fmr %f4,%f1
   fmr %f5,%f2
   bl check_ps

   # ps_sel.
0: mtcr %r0
   fadd %f3,%f11,%f11       # Record an exception for ps_sel. to pick up.
   ps_sel. %f3,%f2,%f1,%f0
   bl record
   mfcr %r3
   lis %r7,0x0A00
   cmpw %r3,%r7
   beq 1f
   stw %r3,8(%r6)
   li %r3,-1
   stw %r3,12(%r6)
   stw %r3,16(%r6)
   stw %r3,20(%r6)
   addi %r6,%r6,32
   b 0f
1: fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps

   ########################################################################
   # Paired-single interactions with FP move instructions
   ########################################################################

   # lfd -> fmr over double-precision register -> paired-single input
0: fadd %f4,%f1,%f1
   lfd %f4,8(%r31)  # 1.0
   ps_merge00 %f3,%f2,%f2
   fadd %f3,%f1,%f1
   fmr %f3,%f4
   bl record
   fmr %f5,%f2
   bl check_ps
0: lfs %f3,8(%r30)
   fadd %f5,%f1,%f1
   lfd %f5,288(%r31)  # 1.333...
   ps_merge00 %f3,%f31,%f31
   fadd %f3,%f1,%f1
   fmr %f3,%f5
   bl record
   lfd %f4,288(%r31)
   fadd %f5,%f2,%f1
   bl check_ps

   # lfd -> fmr over paired-single register -> paired-single input
0: fadd %f6,%f1,%f1
   lfd %f6,8(%r31)  # 1.0
   ps_merge00 %f3,%f2,%f2
   fmr %f3,%f6
   bl record
   fmr %f4,%f1
   fmr %f5,%f2
   bl check_ps
0: fadd %f7,%f1,%f1
   lfd %f7,288(%r31)  # 1.333...
   lfs %f3,0(%r30)
   ps_merge00 %f3,%f31,%f31
   fmr %f3,%f7
   bl record
   lfd %f4,288(%r31)
   fadd %f5,%f2,%f1
   bl check_ps

   # lfs -> fmr over double-precision register -> paired-single input
0: lfs %f8,4(%r30)  # 1.0
   ps_merge00 %f3,%f2,%f2
   lfd %f3,288(%r31)  # 1.333...
   fadd %f3,%f3,%f0
   fmr %f3,%f8
   bl record
   fmr %f4,%f1
   fmr %f5,%f2
   bl check_ps
0: lfs %f9,16(%r30)  # 1.666...
   ps_merge00 %f3,%f31,%f31
   fadd %f3,%f1,%f1
   fmr %f3,%f9
   bl record
   lfs %f4,16(%r30)
   fadd %f5,%f2,%f1
   bl check_ps

   # lfs -> fmr over paired-single register -> paired-single input
0: lfs %f10,4(%r30)  # 1.0
   ps_merge00 %f3,%f2,%f2
   fmr %f3,%f10
   bl record
   fmr %f4,%f1
   fmr %f5,%f2
   bl check_ps
0: lfs %f11,16(%r30)  # 1.666...
   ps_merge00 %f3,%f31,%f31
   fmr %f3,%f11
   bl record
   lfs %f4,16(%r30)
   fadd %f5,%f2,%f1
   bl check_ps

   ########################################################################
   # Paired-single interactions with frsp instruction
   ########################################################################

   # The fres instruction is documented to leave the ps1 slot of the
   # destination operand in an undefined state, but we check here for the
   # actual behavior of the 750CL.

0: mtfsf 255,%f0
   ps_mr %f3,%f0
   lfd %f4,8(%r31)
   frsp %f3,%f4
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm

0: mtfsf 255,%f0
   ps_mr %f3,%f0
   lfd %f4,8(%r31)
   frsp %f3,%f4
   ps_merge01 %f3,%f3,%f3
   bl record
   fmr %f4,%f1
   fmr %f5,%f1
   bl check_ps_pnorm

.endif  # TEST_PAIRED_SINGLE

   ########################################################################
   # End of the test code.
   ########################################################################

0: sub %r3,%r6,%r5
   srwi %r3,%r3,5
   lwz %r0,0x7F00(%r4)
   mtcr %r0
   lwz %r0,0x7F04(%r4)
   mtlr %r0
   lwz %r14,0x7F08(%r4)
   lwz %r15,0x7F0C(%r4)
   lwz %r24,0x7F10(%r4)
   lwz %r25,0x7F14(%r4)
   lwz %r26,0x7F18(%r4)
   lwz %r27,0x7F1C(%r4)
   lwz %r28,0x7F20(%r4)
   lwz %r29,0x7F24(%r4)
   lwz %r30,0x7F28(%r4)
   lwz %r31,0x7F2C(%r4)
   lfd %f14,0x7F30(%r4)
   lfd %f15,0x7F38(%r4)
   lfd %f24,0x7F40(%r4)
   lfd %f25,0x7F48(%r4)
   lfd %f26,0x7F50(%r4)
   lfd %f27,0x7F58(%r4)
   lfd %f28,0x7F60(%r4)
   lfd %f29,0x7F68(%r4)
   lfd %f30,0x7F70(%r4)
   lfd %f31,0x7F78(%r4)
   blr

   ########################################################################
   # Subroutines to store an instruction word in the failure buffer.
   # One of these is called for every tested instruction; failures are
   # recorded by advancing the store pointer.
   # On entry:
   #     R6 = pointer at which to store instruction
   # On return:
   #     R8 = destroyed
   #     R9 = destroyed
   ########################################################################

record:
   mflr %r9
   addi %r9,%r9,-8
   lwz %r8,0(%r9)
   stw %r8,0(%r6)
   stw %r9,4(%r6)
   li %r8,0
   stw %r8,8(%r6)
   stw %r8,12(%r6)
   stw %r8,16(%r6)
   stw %r8,20(%r6)
   blr

   # Alternate version used when the blr is separated by one instruction
   # from the instruction under test.  Used in testing branch instructions.
record2:
   mflr %r9
   addi %r9,%r9,-12
   lwz %r8,0(%r9)
   stw %r8,0(%r6)
   stw %r9,4(%r6)
   li %r8,0
   stw %r8,8(%r6)
   stw %r8,12(%r6)
   stw %r8,16(%r6)
   stw %r8,20(%r6)
   blr

   ########################################################################
   # Subroutines to validate the result and CR/XER changes from a
   # fixed-point instruction.  On failure, the instruction is automatically
   # recorded in the failure table (assuming a previous "bl record" at the
   # appropriate place).
   # On entry:
   #     R3 = result of instruction
   #     R7 = expected result
   # On return:
   #     R8 = destroyed
   #     R9 = destroyed
   #     XER = 0
   #     CR = 0
   ########################################################################

check_alu:  # CA = 0, OV = 0, SO = 0, CR0 not modified
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   cmpwi %r9,0
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_lt:  # CA = 0, OV = 0, SO = 0, CR0 = less than
   mfcr %r8
   lis %r9,0x4000
   addis %r9,%r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   cmpwi %r9,0
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_gt:  # CA = 0, OV = 0, SO = 0, CR0 = greater than
   mfcr %r8
   lis %r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   cmpwi %r9,0
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_eq:  # CA = 0, OV = 0, SO = 0, CR0 = equal
   mfcr %r8
   lis %r9,0x2000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   cmpwi %r9,0
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_undef:  # CA = 0, OV = 0, SO = 0, CR0[0:2] = undefined, CR0[3] = 0
   crclr 0
   crclr 1
   crclr 2
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   cmpwi %r9,0
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ov:  # CA = 0, OV = 1, SO = 1, CR0 not modified
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   lis %r8,0x6000
   addis %r8,%r8,0x6000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ov_lt:  # CA = 0, OV = 1, SO = 1, CR0 = less than
   mfcr %r8
   lis %r9,0x5000
   addis %r9,%r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x6000
   addis %r8,%r8,0x6000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ov_gt:  # CA = 0, OV = 1, SO = 1, CR0 = greater than
   mfcr %r8
   lis %r9,0x5000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x6000
   addis %r8,%r8,0x6000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ov_eq:  # CA = 0, OV = 1, SO = 1, CR0 = equal
   mfcr %r8
   lis %r9,0x3000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x6000
   addis %r8,%r8,0x6000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ov_undef:  # CA = 0, OV = 1, SO = 1, CR0[0:2] = undefined, CR0[3] = 1
   crclr 0
   crclr 1
   crclr 2
   mfcr %r8
   lis %r9,0x1000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x6000
   addis %r8,%r8,0x6000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_so:  # CA = 0, OV = 0, SO = 1, CR0 not modified
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   lis %r8,0x4000
   addis %r8,%r8,0x4000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_so_lt:  # CA = 0, OV = 0, SO = 1, CR0 = less than
   mfcr %r8
   lis %r9,0x5000
   addis %r9,%r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x4000
   addis %r8,%r8,0x4000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_so_gt:  # CA = 0, OV = 0, SO = 1, CR0 = greater than
   mfcr %r8
   lis %r9,0x5000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x4000
   addis %r8,%r8,0x4000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_so_eq:  # CA = 0, OV = 0, SO = 1, CR0 = equal
   mfcr %r8
   lis %r9,0x3000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x4000
   addis %r8,%r8,0x4000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_so_undef:  # CA = 0, OV = 0, SO = 1, CR0[0:2] = undefined, CR0[3] = 1
   crclr 0
   crclr 1
   crclr 2
   mfcr %r8
   lis %r9,0x1000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x4000
   addis %r8,%r8,0x4000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca:  # CA = 1, OV = 0, SO = 0, CR0 not modified
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   lis %r8,0x2000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_lt:  # CA = 1, OV = 0, SO = 0, CR0 = less than
   mfcr %r8
   lis %r9,0x4000
   addis %r9,%r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x2000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_gt:  # CA = 1, OV = 0, SO = 0, CR0 = greater than
   mfcr %r8
   lis %r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x2000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_eq:  # CA = 1, OV = 0, SO = 0, CR0 = equal
   mfcr %r8
   lis %r9,0x2000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   cmpw %r9,%r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_ov:  # CA = 1, OV = 1, SO = 1, CR0 not modified
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   lis %r8,0x7000
   addis %r8,%r8,0x7000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_ov_lt:  # CA = 1, OV = 1, SO = 1, CR0 = less than
   mfcr %r8
   lis %r9,0x5000
   addis %r9,%r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x7000
   addis %r8,%r8,0x7000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_ov_gt:  # CA = 1, OV = 1, SO = 1, CR0 = greater than
   mfcr %r8
   lis %r9,0x5000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x7000
   addis %r8,%r8,0x7000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_ov_eq:  # CA = 1, OV = 1, SO = 1, CR0 = equal
   mfcr %r8
   lis %r9,0x3000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x7000
   addis %r8,%r8,0x7000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_so:  # CA = 1, OV = 0, SO = 1, CR0 not modified
   mfcr %r8
   cmpwi %r8,0
   mfxer %r9
   bne 1f
   lis %r8,0x5000
   addis %r8,%r8,0x5000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_so_lt:  # CA = 1, OV = 0, SO = 1, CR0 = less than
   mfcr %r8
   lis %r9,0x5000
   addis %r9,%r9,0x4000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x5000
   addis %r8,%r8,0x5000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_so_gt:  # CA = 1, OV = 0, SO = 1, CR0 = greater than
   mfcr %r8
   lis %r9,0x5000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x5000
   addis %r8,%r8,0x5000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

check_alu_ca_so_eq:  # CA = 1, OV = 0, SO = 1, CR0 = equal
   mfcr %r8
   lis %r9,0x3000
   cmpw %r8,%r9
   mfxer %r9
   bne 1f
   lis %r8,0x5000
   addis %r8,%r8,0x5000
   cmpw %r9,%r8
   mfcr %r8
   bne 1f
   cmpw %r3,%r7
   beq 0f
1: stw %r3,8(%r6)
   stw %r8,16(%r6)
   stw %r9,20(%r6)
   addi %r6,%r6,32
0: li %r8,0
   mtxer %r8
   mtcr %r8
   blr

   ########################################################################
   # Subroutines to validate the result and CR/FPSCR changes from a
   # floating-point instruction.  On failure, the instruction is
   # automatically recorded in the failure table (assuming a previous
   # "bl record" at the appropriate place).  For "inexact" values (*_inex
   # routines), FX, XX, and FI are automatically added to the set of FPSCR
   # flags in R0.
   # On entry:
   #     F3 = result of instruction
   #     F4 = expected result
   #     R0 = expected value of FPSCR exceptions (from add_fpscr_*)
   #     CR[24:31] = 0
   # On return:
   #     R0 = 0
   #     R8 = destroyed
   #     R9 = destroyed
   #     FPSCR = all exceptions cleared
   #     CR = 0
   #     0x7000..0x700F(%r4) = destroyed
   ########################################################################

check_fpu_pzero:  # Positive zero, FI = 0
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pzero_inex:  # Positive zero, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_nzero:  # Negative zero, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_nzero_inex:  # Negative zero, FI = 1
   oris %r0,%r0,0x8203
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pdenorm:  # Positive denormalized number, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pdenorm_inex:  # Positive denormalized number, FI = 1
   oris %r0,%r0,0x8203
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_ndenorm:  # Negative denormalized number, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_ndenorm_inex:  # Negative denormalized number, FI = 1
   oris %r0,%r0,0x8203
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pnorm:  # Positive normalized number, FI = 0
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pnorm_inex:  # Positive normalized number, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_nnorm:  # Negative normalized number, FI = 0
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_nnorm_inex:  # Negative normalized number, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pinf:  # Positive infinity, FI = 0
   ori %r0,%r0,0x5000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_pinf_inex:  # Positive infinity, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x5000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_ninf:  # Negative infinity, FI = 0
   ori %r0,%r0,0x9000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_ninf_inex:  # Negative infinity, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x9000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_nan:  # Not a Number, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x1000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   stfd %f3,0x7000(%r4)
   stfd %f4,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_noresult:  # No result (aborted by exception)
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   stfd %f3,0x7000(%r4)
   stfd %f4,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_noresult_nofprf:  # For exception-enabled fctiw
   mtfsb0 15
   mtfsb0 16
   mtfsb0 17
   mtfsb0 18
   mtfsb0 19
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   stfd %f3,0x7000(%r4)
   stfd %f4,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

   ########################################################################
   # Subroutines to validate the result and CR/FPSCR changes from a
   # floating-point estimation instruction (fres or frsqrte).  Equivalent
   # to the check_fpu_* routines, but these allow a range of error for the
   # result and ignore FPSCR[XX] and FPSCR[FI] when checking the state of
   # FPSCR.
   # On entry:
   #     F3 = result of instruction
   #     F4 = lower bound of expected result (inclusive)
   #     F5 = upper bound of expected result (inclusive)
   #     R0 = expected value of FPSCR exceptions (from add_fpscr_*)
   #     CR[24:31] = 0
   # On return:
   #     R0 = 0
   #     R8 = destroyed
   #     R9 = destroyed
   #     FPSCR = all exceptions cleared
   #     CR = 0
   #     0x7000..0x700F(%r4) = destroyed
   ########################################################################

check_fpu_estimate_pnorm:  # Positive normal number
   # XX and FI are undefined for estimation instructions.
   mtfsb0 6
   mtfsb0 14
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   fcmpu cr1,%f3,%f5
   crnor 2,0,5
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_estimate_nnorm:  # Negative normal number
   mtfsb0 6
   mtfsb0 14
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   fcmpu cr1,%f3,%f5
   crnor 2,0,5
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_estimate_pdenorm:  # Positive denormalized number
   mtfsb0 6
   mtfsb0 14
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   fcmpu cr1,%f3,%f5
   crnor 2,0,5
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fpu_estimate_ndenorm:  # Negative denormalized number
   mtfsb0 6
   mtfsb0 14
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   fcmpu cr0,%f3,%f4
   fcmpu cr1,%f3,%f5
   crnor 2,0,5
   beq 0f
1: stfd %f3,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

   ########################################################################
   # Subroutines to validate the result and CR/FPSCR changes from a
   # floating-point convert-to-integer instruction.  Equivalent to the
   # check_fpu_* routines, except that FPRF is not checked and the
   # expected value is received in an integer register.
   # On entry:
   #     F3 = result of instruction
   #     R7 = expected result
   #     R0 = expected value of FPSCR exceptions (from add_fpscr_*)
   #     CR[24:31] = 0
   # On return:
   #     R0 = 0
   #     R8 = destroyed
   #     R9 = destroyed
   #     FPSCR = all exceptions and FPRF cleared
   #     CR = 0
   #     0x7000..0x7007(%r4) = destroyed
   ########################################################################

check_fctiw:
   mtfsb0 15
   mtfsb0 16
   mtfsb0 17
   mtfsb0 18
   mtfsb0 19
   mflr %r9
   bl check_fpscr
   mtlr %r9
   stfd %f3,0x7000(%r4)
   lwz %r9,0x7004(%r4)
   bne 1f
   cmpw %r9,%r7
   beq 0f
1: stw %r9,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_fctiw_inex:
   oris %r0,%r0,0x8202
   mtfsb0 15
   mtfsb0 16
   mtfsb0 17
   mtfsb0 18
   mtfsb0 19
   mflr %r9
   bl check_fpscr
   mtlr %r9
   stfd %f3,0x7000(%r4)
   lwz %r9,0x7004(%r4)
   bne 1f
   cmpw %r9,%r7
   beq 0f
1: stw %r9,8(%r6)
   stw %r8,16(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

   ########################################################################
   # Subroutines to get FPSCR exception flags for use with check_fpu_* and
   # check_fcti*.
   # On entry:
   #     R0 = current set of exception flags
   # On return:
   #     R0 = R0 | requested exception flag
   ########################################################################

add_fpscr_fex:
   oris %r0,%r0,0x4000
   blr

add_fpscr_ox:
   oris %r0,%r0,0x9000
   blr

add_fpscr_ux:
   oris %r0,%r0,0x8800
   blr

add_fpscr_zx:
   oris %r0,%r0,0x8400
   blr

add_fpscr_xx:
   oris %r0,%r0,0x8200
   blr

add_fpscr_xx_fi:
   oris %r0,%r0,0x8202
   blr

# The fres and ps_res instructions set FI without XX.
add_fpscr_fi:
   oris %r0,%r0,0x0002
   blr

add_fpscr_vxsnan:
   oris %r0,%r0,0xA100
   blr

add_fpscr_vxisi:
   oris %r0,%r0,0xA080
   blr

add_fpscr_vxidi:
   oris %r0,%r0,0xA040
   blr

add_fpscr_vxzdz:
   oris %r0,%r0,0xA020
   blr

add_fpscr_vximz:
   oris %r0,%r0,0xA010
   blr

add_fpscr_vxvc:
   oris %r0,%r0,0xA008
   blr

add_fpscr_vxsqrt:
   oris %r0,%r0,0xA000
   ori %r0,%r0,0x0200
   blr

add_fpscr_vxcvi:
   oris %r0,%r0,0xA000
   ori %r0,%r0,0x0100
   blr

   ########################################################################
   # Subroutine to clear FPSCR[FX] flag for use with check_fpu_* and
   # check_fcti*.
   # On entry:
   #     R0 = current set of exception flags
   # On return:
   #     R0 = R0 & ~0x80000000
   ########################################################################

remove_fpscr_fx:
   oris %r0,%r0,0x8000
   xoris %r0,%r0,0x8000
   blr

   ########################################################################
   # Helper routine for check_fpu_* and check_fcti* to compare FPSCR to an
   # expected value and clear exception flags.
   # On entry:
   #     R0 = expected value of FPSCR (with bits 13 and 24-31 clear)
   # On return:
   #     R8 = FPSCR[0:12] || 0 || FPSCR[14:23] || 0000 0000
   #     FPSCR = all exceptions cleared
   #     CR0 = comparison result
   #     CR[4:23] = destroyed
   ########################################################################

check_fpscr:
   mcrfs cr0,0
   mcrfs cr1,1
   mcrfs cr2,2
   mcrfs cr3,3
   mcrfs cr4,4
   mcrfs cr5,5
   mfcr %r8
   oris %r8,%r8,0x0004
   xoris %r8,%r8,0x0004
   ori %r8,%r8,0x00FF
   xori %r8,%r8,0x00FF
   cmpw %r8,%r0
   blr

   ########################################################################
   # Subroutine to check the two values of an FPR in paired-single format.
   # The values stored in F4 and F5 should be rounded or truncated, if
   # necessary, before calling this subroutine.
   # On entry:
   #     F3 = result of instruction (will be treated as paired-single)
   #     F4 = expected result for slot 0
   #     F5 = expected result for slot 1
   # On return:
   #     F4 = destroyed
   #     CR0 = result of comparison (EQ if both values matched)
   ########################################################################

check_ps:
   ps_cmpu0 cr0,%f3,%f4
   ps_merge11 %f4,%f3,%f3
   stfd %f3,8(%r6)
   stfs %f4,16(%r6)
   bne 1f
   ps_cmpu0 cr0,%f4,%f5
   beq 0f
1: addi %r6,%r6,32
0: blr

   ########################################################################
   # Subroutines to validate the result and CR/FPSCR changes from a
   # paired-single instruction.  On failure, the instruction is
   # automatically recorded in the failure table (assuming a previous
   # "bl record" at the appropriate place).  For "inexact" values (*_inex
   # routines), FX, XX, and FI are automatically added to the set of FPSCR
   # flags in R0.
   # On entry:
   #     F3 = result of instruction
   #     F4 = expected result for slot 0
   #     F5 = expected result for slot 1
   #     0(LR) = expected result for slot 0, as a double
   #     8(LR) = expected result for slot 1, as a double
   #     R0 = expected value of FPSCR exceptions (from add_fpscr_*)
   #     CR[24:31] = 0
   # On return:
   #     R0 = 0
   #     R8 = destroyed
   #     R9 = destroyed
   #     F4 = destroyed
   #     F5 = destroyed
   #     FPSCR = all exceptions cleared
   #     CR = 0
   #     0x7000..0x700F(%r4) = destroyed
   ########################################################################

check_ps_pzero:  # Positive zero, FI = 0
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pzero_inex:  # Positive zero, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_nzero:  # Negative zero, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_nzero_inex:  # Negative zero, FI = 1
   oris %r0,%r0,0x8203
   ori %r0,%r0,0x2000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pdenorm:  # Positive denormalized number, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pdenorm_inex:  # Positive denormalized number, FI = 1
   oris %r0,%r0,0x8203
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_ndenorm:  # Negative denormalized number, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_ndenorm_inex:  # Negative denormalized number, FI = 1
   oris %r0,%r0,0x8203
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pnorm:  # Positive normalized number, FI = 0
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pnorm_inex:  # Positive normalized number, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_nnorm:  # Negative normalized number, FI = 0
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_nnorm_inex:  # Negative normalized number, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pinf:  # Positive infinity, FI = 0
   ori %r0,%r0,0x5000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_pinf_inex:  # Positive infinity, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x5000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_ninf:  # Negative infinity, FI = 0
   ori %r0,%r0,0x9000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_ninf_inex:  # Negative infinity, FI = 1
   oris %r0,%r0,0x8202
   ori %r0,%r0,0x9000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 1,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps
   mtlr %r9
   bne 0f
   beq cr1,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_nan:  # Not a Number, FI = 0
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x1000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   stfd %f3,0x7000(%r4)
   stfd %f4,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   bne 1f
   ps_merge11 %f4,%f3,%f3
   stfd %f4,0x7000(%r4)
   stfd %f5,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   beq 0f
1: stfd %f3,8(%r6)
   ps_merge11 %f4,%f3,%f3
   stfs %f4,16(%r6)
   stw %r8,20(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

check_ps_noresult:  # No result (aborted by exception)
   mflr %r9
   bl check_fpscr
   mtlr %r9
   bne 1f
   stfd %f3,0x7000(%r4)
   stfd %f4,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   bne 1f
   ps_merge11 %f4,%f3,%f3
   stfd %f4,0x7000(%r4)
   stfd %f5,0x7008(%r4)
   lwz %r0,0x7000(%r4)
   lwz %r9,0x7008(%r4)
   cmpw %r0,%r9
   bne 1f
   lwz %r0,0x7004(%r4)
   lwz %r9,0x700C(%r4)
   cmpw %r0,%r9
   beq 0f
1: stfd %f3,8(%r6)
   ps_merge11 %f4,%f3,%f3
   stfs %f4,16(%r6)
   stw %r8,20(%r6)
   addi %r6,%r6,32
0: li %r0,0
   mtcr %r0
   blr

   ########################################################################
   # Subroutine to check the two values of an FPR in paired-single format.
   # The values stored in F4 through F7 should be rounded or truncated, if
   # necessary, before calling this subroutine.
   # On entry:
   #     F3 = result of instruction (will be treated as paired-single)
   #     F4 = lower bound of expected ps0 result (inclusive)
   #     F5 = lower bound of expected ps1 result (inclusive)
   #     F6 = upper bound of expected ps0 result (inclusive)
   #     F7 = upper bound of expected ps1 result (inclusive)
   # On return:
   #     F4 = destroyed
   #     CR0 = result of comparison (EQ if both values matched)
   #     CR1 = destroyed
   ########################################################################

check_ps_estimate:
   ps_cmpu0 cr0,%f3,%f4
   ps_cmpu0 cr1,%f3,%f6
   crnor 2,0,5
   ps_merge11 %f4,%f3,%f3
   stfd %f3,8(%r6)
   stfs %f4,16(%r6)
   bne 1f
   ps_cmpu0 cr0,%f4,%f5
   ps_cmpu0 cr1,%f4,%f7
   crnor 2,0,5
   beq 0f
1: addi %r6,%r6,32
0: blr

   ########################################################################
   # Paired-single equivalents of the check_fpu_estimate_* subroutines.
   # On entry:
   #     F3 = result of instruction
   #     F4 = lower bound of expected ps0 result (inclusive)
   #     F5 = lower bound of expected ps1 result (inclusive)
   #     F6 = upper bound of expected ps0 result (inclusive)
   #     F7 = upper bound of expected ps1 result (inclusive)
   #     R0 = expected value of FPSCR exceptions (from add_fpscr_*)
   #     CR[24:31] = 0
   # On return:
   #     R0 = 0
   #     R8 = destroyed
   #     R9 = destroyed
   #     FPSCR = all exceptions cleared
   #     CR = 0
   #     0x7000..0x700F(%r4) = destroyed
   ########################################################################

check_ps_estimate_pnorm:  # Positive normal number
   mtfsb0 6
   mtfsb0 14
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 7,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps_estimate
   mtlr %r9
   bne 0f
   beq cr7,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_estimate_nnorm:  # Negative normal number
   mtfsb0 6
   mtfsb0 14
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 7,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps_estimate
   mtlr %r9
   bne 0f
   beq cr7,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_estimate_pdenorm:  # Positive denormalized number
   mtfsb0 6
   mtfsb0 14
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x4000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 7,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps_estimate
   mtlr %r9
   bne 0f
   beq cr7,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

check_ps_estimate_ndenorm:  # Negative denormalized number
   mtfsb0 6
   mtfsb0 14
   oris %r0,%r0,0x0001
   ori %r0,%r0,0x8000
   mflr %r9
   bl check_fpscr
   mtlr %r9
   mcrf 7,0
   stw %r8,20(%r6)
   addi %r8,%r6,32
   mflr %r9
   bl check_ps_estimate
   mtlr %r9
   bne 0f
   beq cr7,0f
1: mr %r6,%r8
0: li %r0,0
   mtcr %r0
   blr

.if TEST_RECIPROCAL_TABLES

   ########################################################################
   # Software implementation of the fres instruction.
   # On entry:
   #     F1 = 1.0
   #     F3 = fres instruction input
   # On return:
   #     F4 = expected output
   #     F5 = destroyed
   #     R3 = expected FPSCR[OX,UX,FI,FPRF]
   #     R7 = destroyed
   #     R8 = destroyed
   #     R9 = destroyed
   #     R10 = destroyed
   #     R11 = destroyed
   #     CR0 = destroyed
   #     0x7000..0x7007(%r4) = destroyed
   ########################################################################

calc_fres:
   mflr %r9
   bl 1f
1: mflr %r3
   mtlr %r9
   addi %r11,%r3,2f-1b
   b 0f
   # Base and delta values for each of 32 mantissa intervals.
2: .short 0x3FFC,0x3E1, 0x3C1C,0x3A7, 0x3875,0x371, 0x3504,0x340
   .short 0x31C4,0x313, 0x2EB1,0x2EA, 0x2BC8,0x2C4, 0x2904,0x2A0
   .short 0x2664,0x27F, 0x23E5,0x261, 0x2184,0x245, 0x1F40,0x22A
   .short 0x1D16,0x212, 0x1B04,0x1FB, 0x190A,0x1E5, 0x1725,0x1D1
   .short 0x1554,0x1BE, 0x1396,0x1AC, 0x11EB,0x19B, 0x104F,0x18B
   .short 0x0EC4,0x17C, 0x0D48,0x16E, 0x0BD7,0x15B, 0x0A7C,0x15B
   .short 0x0922,0x143, 0x07DF,0x143, 0x069C,0x12D, 0x056F,0x12D
   .short 0x0442,0x11A, 0x0328,0x11A, 0x020E,0x108, 0x0106,0x106

   # Extract the sign bit, exponent, and mantissa, and check for special cases.
0: stfs %f3,0x7000(%r4)
   lwz %r3,0x7000(%r4)
   andis. %r10,%r3,0x8000
   rlwinm %r9,%r3,9,24,31
   clrlwi %r8,%r3,9
   cmpwi %r9,255
   beq 8f
   cmpwi %r9,0
   beq 7f

   # Calculate the new exponent and mantissa.
1: li %r3,253
   subf %r9,%r9,%r3
   rlwinm %r3,%r8,16,25,29  # Lookup table index * 4
   add %r11,%r11,%r3
   lhz %r3,2(%r11)
   rlwinm %r7,%r8,24,22,31
   mullw %r8,%r3,%r7
   lhz %r3,0(%r11)
   slwi %r3,%r3,10
   subf %r8,%r8,%r3
   andi. %r11,%r8,1  # FPSCR[FI] output bit
   srwi %r8,%r8,1

   # Denormalize the result if the exponent is not positive.  This will
   # take at most two steps.
   cmpwi cr1,%r9,0
   bgt cr1,2f
   li %r9,0
   oris %r8,%r8,0x80
   andi. %r3,%r8,1
   srwi %r8,%r8,1
   or %r11,%r11,%r3
   beq cr1,2f
   andi. %r3,%r8,1
   srwi %r8,%r8,1
   or %r11,%r11,%r3

   # Build and return the final value and FPSCR.
2: or %r7,%r10,%r8
   rlwimi %r7,%r9,23,1,8
   slwi %r3,%r11,17
   # Set FPSCR[FX,UX] if tiny (R9=0) and inexact (R11!=0).
   cmpwi %r9,0
   bne 9f
   cmpwi %r11,0
   beq 9f
   oris %r3,%r3,0x8800
   b 9f

   # Handle exponent 0 (zero/denormal).
7: lis %r3,0x20  # Nonzero mantissa < 0x200000 is an overflow condition.
   cmpw %r8,%r3
   bge 3f
   lis %r7,0x7F80  # Return appropriately signed infinity or HUGE_VAL.
   li %r3,0
   cmpwi %r8,0
   beq 2f
   addi %r7,%r7,-1
   lis %r3,0x9002
2: or %r7,%r7,%r10
   b 9f
3: slwi %r8,%r8,1  # Normalize (requires either 1 or 2 shifts).
   andis. %r3,%r8,0x80
   bne 4f
   slwi %r8,%r8,1
   addi %r9,%r9,-1
4: clrlwi %r8,%r8,9  # Clear the high (implied) bit of the mantissa.
   b 1b              # Continue with normal processing.

   # Handle exponent 255 (inf/NaN).
8: cmpwi %r8,0
   bne 1f
   mr %r7,%r10  # Return appropriately signed zero.
   b 9f
1: ori %r7,%r3,0x0040  # Return quieted NaN.
   b 9f

   # Return the final value (currently in R7) and FPSCR (calculating FPRF
   # by multiplying the output value by 1.0).
9: stw %r7,0x7000(%r4)
   lfs %f4,0x7000(%r4)
   fmuls %f5,%f4,%f1
   mffs %f5
   stfd %f5,0x7000(%r4)
   lwz %r7,0x7004(%r4)
   rlwinm %r7,%r7,0,15,19
   or %r3,%r3,%r7
   blr

   ########################################################################
   # Software implementation of the frsqrte instruction.
   # On entry:
   #     F1 = 1.0
   #     F3 = frsqrte instruction input
   # On return:
   #     F4 = expected output
   #     F5 = destroyed
   #     R3 = expected FPSCR[FPRF]
   #     R7 = destroyed
   #     R8 = destroyed
   #     R9 = destroyed
   #     R10 = destroyed
   #     R11 = destroyed
   #     CR0 = destroyed
   #     XER[CA] = destroyed
   #     0x7000..0x7007(%r4) = destroyed
   ########################################################################

calc_frsqrte:
   mflr %r9
   bl 1f
1: mflr %r3
   mtlr %r9
   addi %r11,%r3,2f-1b
   b 0f
   # Base and delta values for each of 32 mantissa intervals.
2: .short 0x7FF4,0x7A4, 0x7852,0x700, 0x7154,0x670, 0x6AE4,0x5F2
   .short 0x64F2,0x584, 0x5F6E,0x524, 0x5A4C,0x4CC, 0x5580,0x47E
   .short 0x5102,0x43A, 0x4CCA,0x3FA, 0x48D0,0x3C2, 0x450E,0x38E
   .short 0x4182,0x35E, 0x3E24,0x332, 0x3AF2,0x30A, 0x37E8,0x2E6
   .short 0x34FD,0x568, 0x2F97,0x4F3, 0x2AA5,0x48D, 0x2618,0x435
   .short 0x21E4,0x3E7, 0x1DFE,0x3A2, 0x1A5C,0x365, 0x16F8,0x32E
   .short 0x13CA,0x2FC, 0x10CE,0x2D0, 0x0DFE,0x2A8, 0x0B57,0x283
   .short 0x08D4,0x261, 0x0673,0x243, 0x0431,0x226, 0x020B,0x20B

   # Extract the sign bit, exponent, and mantissa (high bits), and check for
   # special cases.
0: stfd %f3,0x7000(%r4)
   lwz %r3,0x7000(%r4)
   andis. %r10,%r3,0x8000
   rlwinm %r9,%r3,12,21,31
   clrlwi %r8,%r3,12
   cmpwi %r9,255
   beq 8f
   cmpwi %r9,0
   beq 7f
   cmpwi %r10,0
   beq 1f
   # Negative values are always NaN.
   lis %r7,0x7FF8
   li %r8,0
   b 9f

   # Calculate the new exponent and mantissa.
1: rlwinm %r3,%r8,18,26,29  # Lookup table index * 4
   andi. %r7,%r9,1
   bne 2f
   ori %r3,%r3,64
2: li %r7,3068
   subf %r9,%r9,%r7
   srwi %r9,%r9,1
   add %r11,%r11,%r3
   lhz %r3,2(%r11)
   rlwinm %r7,%r8,27,21,31
   mullw %r8,%r3,%r7
   lhz %r3,0(%r11)
   slwi %r3,%r3,11
   subf %r8,%r8,%r3

   # Build and return the final value and FPSCR.
2: srwi %r7,%r8,6
   slwi %r8,%r8,26
   or %r7,%r7,%r10
   rlwimi %r7,%r9,20,1,11
   b 9f

   # Handle exponent 0 (zero/denormal).
7: cmpwi %r8,0
   lwz %r7,0x7004(%r4)
   bne 2f
   cmpwi %r7,0
   bne 2f
   oris %r7,%r10,0x7FF0  # Return appropriately signed infinity.
   li %r8,0
   b 9f
2: addc %r7,%r7,%r7  # Normalize.
   adde %r8,%r8,%r8
   andis. %r3,%r8,0x10
   bne 3f
   addi %r9,%r9,-1
   b 2b
3: clrlwi %r8,%r8,12  # Clear the high (implied) bit of the mantissa.
   b 1b               # Continue with normal processing.

   # Handle exponent 255 (inf/NaN).
8: cmpwi %r8,0
   bne 2f
   andis. %r7,%r10,0x8000
   beq 1f
   lis %r7,0x7F80  # Negative infinity turns into a NaN.
1: li %r8,0
   b 9f
2: oris %r7,%r3,0x0008  # Return quieted NaN.
   lwz %r8,0x7004(%r4)
   b 9f

   # Return the final value (currently in R7/R8) and FPRF (calculated by
   # multiplying the output value by 1.0).
9: stw %r7,0x7000(%r4)
   stw %r8,0x7004(%r4)
   lfd %f4,0x7000(%r4)
   fmul %f5,%f4,%f1
   mffs %f5
   stfd %f5,0x7000(%r4)
   lwz %r3,0x7004(%r4)
   rlwinm %r3,%r3,0,15,19
   blr

   ########################################################################
   # Software implementation of the ps_rsqrte instruction.
   # On entry:
   #     F1 = 1.0
   #     F3 = ps_rsqrte instruction input (as a scalar value)
   # On return:
   #     F4 = expected output
   #     F5 = destroyed
   #     R3 = expected FPSCR[FPRF]
   #     R7 = destroyed
   #     R8 = destroyed
   #     R9 = destroyed
   #     R10 = destroyed
   #     R11 = destroyed
   #     CR0 = destroyed
   #     XER[CA] = destroyed
   #     0x6FFC..0x7007(%r4) = destroyed
   ########################################################################

calc_ps_rsqrte:
   mflr %r9
   stw %r9,0x6FFC(%r4)
   bl calc_frsqrte
   lwz %r9,0x6FFC(%r4)
   mtlr %r9
   # Truncate to single precision.
   stfd %f4,0x7000(%r4)
   lwz %r7,0x7004(%r4)
   andis. %r7,%r7,0xE000
   stw %r7,0x7004(%r4)
   lfd %f4,0x7000(%r4)
   blr

.endif  # TEST_RECIPROCAL_TABLES
