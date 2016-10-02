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
# the binutils package (built for 32-bit PowerPC targets).

set -e

case "$1" in
    -*|"") echo >&2 "Usage: $0 ppc750cl.s >ppc750cl-bin.i"; exit 1;;
esac

PPC_AS="${PPC_AS-powerpc-eabi-as}"
PPC_LD="${PPC_LD-powerpc-eabi-ld}"
PPC_OBJCOPY="${PPC_OBJCOPY-powerpc-eabi-objcopy}"

tempdir="$(mktemp -d)"; test -n "$tempdir" -a -d "$tempdir"
trap "rm -r '$tempdir'" EXIT SIGHUP SIGINT SIGQUIT SIGTERM

"$PPC_AS" --defsym ESPRESSO=1 -o"$tempdir/ppc750cl.o" "$1"
"$PPC_LD" -Ttext=0x1000000 --defsym _start=0x1000000 -o"$tempdir/ppc750cl" "$tempdir/ppc750cl.o"
"$PPC_OBJCOPY" -O binary "$tempdir/ppc750cl" "$tempdir/ppc750cl.bin"

echo "static const unsigned char ppc750cl_bin[] = {";
perl <"$tempdir/ppc750cl.bin" \
    -e 'while (read(STDIN, $_, 16)) {print join("", map {sprintf("%d,",$_)} unpack("C*", $_)) . "\n"}'
echo "};";
