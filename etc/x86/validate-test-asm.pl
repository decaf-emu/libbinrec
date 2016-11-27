#!/usr/bin/perl
#
# libbinrec: a recompiling translator for machine code
# Copyright (c) 2016 Andrew Church <achurch@achurch.org>
#
# This software may be copied and redistributed under certain conditions;
# see the file "COPYING" in the source code distribution for details.
# NO WARRANTY is provided with this software.
#

# This script can be used to check that x86 assembly in comments in test
# sources matches the associated machine code.  Run from the top directory:
# etc/x86/validate-test-asm.pl tests/host-x86/*/*.c

use strict;
use warnings;

my $tmpdir = "/tmp/bvta.$$";
mkdir $tmpdir, 0700 or die "mkdir($tmpdir): $!\n";
sub rm_tmpdir { system "rm", "-rf", $tmpdir; }
END { &rm_tmpdir; }
$SIG{HUP} = sub {exit 1;};
$SIG{INT} = sub {exit 1;};
$SIG{TERM} = sub {exit 1;};

undef $/;

while (<>) {
    s|/\*[\0-\377]*?\*/||g;
    s|^\s*//.*\n||g;
    /static const uint8_t expected_code\[\] = \{\n*((?:.*\n)*)\};/ or next;
    my $expected_code = $1;
    my $code = "";
    my $disasm = "";
    while ($expected_code =~ s|^\s*(0x\S+),\s*(?:// (.+))?\n||) {
        my ($a, $b) = ($1, $2);
        next if defined($b) && ($b eq "(data)" || $b eq "(padding)");
        $code .= ".byte $a\n";
        $disasm .= "$b\n" if $b;
    }
    die "$ARGV: invalid format in expected_code:\n$expected_code"
        if $expected_code ne "";

    open F, "|as -o '$tmpdir/a.o'" or die "pipe(as): $!\n";
    print F $code;
    close F and -f "$tmpdir/a.o" or die "as failed\n";
    open F, "objdump -d '$tmpdir/a.o' |" or die "pipe(objdump): $!\n";
    $_ = <F>;
    close F;
    s/^[\0-\377]*\<\.text\>:\n//;
    my $objdump = "";
    my $a = $disasm;
    while (s/^(.*)\n//) {
        my $line = $1;
        $line =~ s/\s*\#.*//;
        $line =~ s/\s+/ /g;
        next if $line !~ s/^\s*[0-9a-f]+:\s//;
        1 while $line =~ s/^[0-9a-f]{2}\s//;
        $line =~ s/\s+$//;
        next if !$line;
        $line =~ s/0x([0-9a-f]+)/"0x".uc($1)/eg;
        $line =~ s/callq/call/;
        $line =~ s/cltd/cdq/;
        $line =~ s/cmov(n?)e/cmov$1z/;
        $line =~ s/cqto/cqo/;
        $line =~ s/j(n?)e/j$1z/;
        $line =~ s/jmpq/jmp/;
        $line =~ s/movabs/mov/;
        $line =~ s/retq/ret/;
        $line =~ s/set(n?)e/set$1z/;
        if ($a =~ s/^(.+)\n//) {
            my $dline = $1;

            if ($dline =~ /jnc/) {
                $line =~ s/jae/jnc/;
            }
            if ($dline =~ /jc/) {
                $line =~ s/jb(?!e)/jc/;
            }
            if ($dline =~ /setc/) {
                $line =~ s/setb/setc/;
            }
            if ($dline =~ /(\s*\#.*)$/) {
                $line .= $1;
            }
            if ($dline =~ /^((?:L\d+|epilogue):\s*)/) {
                $line = "$1$line";
            }
            if ($dline =~ /(jmp|jn?[cpsz]|j[bagl]e?) (L\d+|epilogue)/) {
                my ($insn, $label) = ($1, $2);
                $line =~ s/$insn 0x[0-9A-F]+/$insn $label/;
            }
            if ($line =~ /\(.*,1\)/) {
                my $tmp = $line;
                $tmp =~ s/,1\)/\)/;
                my $tmp_cut = $tmp;
                $tmp_cut =~ s/.*\(//;
                my $dline_cut = $dline;
                $dline_cut =~ s/.*\(//;
                if ($tmp_cut eq $dline_cut) {
                    $line = $tmp;
                }
            }
            $line =~ s/andl \$0xFFFFFFFE,/andl \$-2,/;
            $line =~ s/andl \$0xFFFFFFC0,/andl \$-64,/;
            if ($line =~ /0x(?:FFFFFFFF)?([89A-F][0-9A-F]{7})(?=\W)/) {
                my $tmp = $line;
                $tmp =~ s/0x(?:FFFFFFFF)?([89A-F][0-9A-F]{7})(?=\W)/sprintf("-%d",4294967296-hex($1))/e;
                if ($tmp eq $dline) {
                    $line = $tmp;
                } else {
                    $tmp = $line;
                    $tmp =~ s/0x(?:FFFFFFFF)?([89A-F][0-9A-F]{7})(?=\W)/sprintf("-0x%X",4294967296-hex($1))/e;
                    if ($tmp eq $dline) {
                        $line = $tmp;
                    }
                }
            }
            if ($line =~ /0xFFFFFFFF([89A-F][0-9A-F]{7})(?=\W)/) {
                my $tmp = $line;
                $tmp =~ s/0xFFFFFFFF([89A-F][0-9A-F]{7})(?=\W)/0x$1/;
                if ($tmp eq $dline) {
                    $line = $tmp;
                }
            }
            if ($line =~ /0x/) {
                my $tmp = $line;
                $tmp =~ s/0x([0-9A-F]{1,8}(?=\W))/hex($1)/eg;
                if ($tmp eq $dline) {
                    $line = $tmp;
                } else {
                    for (my $i = 1; $i < 16; $i++) {
                        $tmp = $line;
                        $tmp =~ s/0x/"0x".("0"x$i)/e;
                        if ($tmp eq $dline) {
                            $line = $tmp;
                            last;
                        }
                    }
                }
            }
            if ($line eq 'testb $0x20,0x4(%rsp)') {
                $line = 'testb $0x20,4(%rsp)';
            }
            if ($line eq 'nopl -0x5310FF3(%rip)') {
                $line = 'nopl (0xFACEF00D-0x100000000)(%rip)';
            }
            if ($line eq 'nopl -0x65432110(%rip)') {
                $line = 'nopl (0x9ABCDEF0-0x100000000)(%rip)';
            }
        }
        $objdump .= "$line\n";
    }

    open F, ">$tmpdir/source" or die "$tmpdir/source: $!\n";
    print F $disasm;
    close F;
    open F, ">$tmpdir/objdump" or die "$tmpdir/objdump: $!\n";
    print F $objdump;
    close F;
    open F, "diff -u '$tmpdir/source' '$tmpdir/objdump' |"
        or die "pipe(diff): $!\n";
    my $diff = <F>;
    close F;
    if ($diff) {
        print "Difference in $ARGV:\n$diff";
    }
}
