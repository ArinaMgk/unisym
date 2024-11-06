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
my $uinc = $ENV{'uincpath'};
my $ubin = $ENV{'ubinpath'};
my $uobj = $ENV{'uobjpath'};
if (length($ulib) == 0) {
	print "Please set ulibpath.\n";
	exit -1;
} else {
	print "ulibpath: " .$ulib. "\n";
}
if (length($uinc) == 0) {
	print "Please set uincpath.\n";
	exit -1;
} else {
	print "uincpath: " .$uinc. "\n";
}
if (length($ubin) == 0) {
	print "Please set ubinpath.\n";
	exit -1;
} else {
	print "ubinpath: " .$ubin. "\n";
}
if (length($uobj) == 0) {
	print "Please set uobjpath.\n";
	exit -1;
} else {
	print "uobjpath: " .$uobj. "\n";
}
my $python_path = `which python`;
chomp $python_path;
if (!defined $python_path || length($python_path) == 0) {
    $python_path = `which python3`;
    chomp $python_path;
    if (!defined $python_path || length($python_path) == 0) {
        print "Python or python3 is not found in PATH.\n";
        exit -1;
    }
}

my $result = system($python_path, './lib/Script/Makefile/makemake.py');
if ($result != 0) {
    print "Failed to execute makemake.py.\n";
    exit -1;
}
