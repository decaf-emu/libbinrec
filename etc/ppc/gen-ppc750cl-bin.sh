#!/bin/sh
#
# libbinrec: a recompiling translator for machine code
# Copyright (c) 2016 Andrew Church <achurch@achurch.org>
#
# This software may be copied and redistributed under certain conditions;
# see the file "COPYING" in the source code distribution for details.
# NO WARRANTY is provided with this software.
#

# This script generates tests/guest-ppc/exec/ppc750cl-bin.i from ppc750cl.s.
# Run as: $0 etc/ppc/ppc750cl.s >tests/guest-ppc/exec/ppc750cl-bin.i
# If necessary, set the environment variables PPC_AS, PPC_LD, and
# PPC_OBJCOPY to point to the "as", "ld", and "objcopy" executables from
# the binutils package (built for the powerpc-eabi target).
#
# If a "-d FILENAME" option is given, a disassembly of the generated object
# code will be written to FILENAME.  In this case, the binutils "objdump"
# program is also required; set the environment variable PPC_OBJDUMP if
# necessary.

# Set these variables to generate different versions of the test code.
tables=1   # Sets TEST_RECIPROCAL_TABLES.
nofpscr=0  # Sets IGNORE_FPSCR_STATE.

set -e

dumpfile=""
if test "x$1" = "x-d"; then
    shift
    dumpfile="$1"
    shift
fi

case "$1" in
    -*|"")
        echo >&2 "Usage: $0 [-d disassembly.txt] ppc750cl.s >ppc750cl-bin.i"
        exit 1
        ;;
esac

PPC_AS="${PPC_AS-powerpc-eabi-as}"
PPC_LD="${PPC_LD-powerpc-eabi-ld}"
PPC_OBJDUMP="${PPC_OBJDUMP-powerpc-eabi-objdump}"
PPC_OBJCOPY="${PPC_OBJCOPY-powerpc-eabi-objcopy}"

tempdir="$(mktemp -d)"; test -n "$tempdir" -a -d "$tempdir"
trap "rm -r '$tempdir'" EXIT SIGHUP SIGINT SIGQUIT SIGTERM

(
    set -x
    "$PPC_AS" --defsym ESPRESSO=1 --defsym TEST_RECIPROCAL_TABLES=$tables \
        --defsym IGNORE_FPSCR_STATE=$nofpscr -o"$tempdir/ppc750cl.o" "$1"
    "$PPC_LD" -Ttext=0x1000000 --defsym _start=0x1000000 \
        -o"$tempdir/ppc750cl" "$tempdir/ppc750cl.o"
    if test -n "$dumpfile"; then
        "$PPC_OBJDUMP" -M750cl -d "$tempdir/ppc750cl" >"$dumpfile"
    fi
    "$PPC_OBJCOPY" -O binary "$tempdir/ppc750cl" "$tempdir/ppc750cl.bin"
)

echo "static const unsigned char ppc750cl_bin[] = {";
perl <"$tempdir/ppc750cl.bin" \
    -e 'while (read(STDIN, $_, 16)) {print join("", map {sprintf("%d,",$_)} unpack("C*", $_)) . "\n"}'
echo "};";
