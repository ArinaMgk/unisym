#!/usr/bin/perl
# ASCII Perl TAB4 CRLF
# LastCheck: 20240520
# AllAuthor: @dosconio
# ModuTitle: Makefile Script Generator Script of Perl
# Copyright: ArinaMgk UniSym, Apache License Version 2.0
# Parallel : ./makemake.py
# Depend   : ../../doc/catalog.csv
# Run-in   : `usl/`
print "UNISYM Makemaker\n";
#
my $ulib = $ENV{'ulibpath'};
my $uinc = $ENV{'uincpath'};#e.g. export uincpath=/mnt/hgfs/unisym/inc in OS
my $ubin = $ENV{'ubinpath'};
print $ulib;
#{} check existence of `python` and `python3`
system("python ./lib/Script/Makefile/makemake.py")
