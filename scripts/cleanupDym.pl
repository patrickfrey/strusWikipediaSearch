#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use utf8;
use open ':std', ':encoding(UTF-8)';

my $num_args = $#ARGV + 1;
if ($num_args == 0)
{
	print STDERR "Usage: cleanupDym.pl [<inputfile>]\n";
	print STDERR "<inputfile>: name of input file to process ('-' for stdin)\n";
	exit;
}

my @lines = ();
sub processLine
{
	my ($line) = @_;
	if ($line =~ /^\s*[0-9]+\s*$/)
	{
		return;
	}
	if ($line =~ /^\s*[A-Za-z][\.]*\s*$/)
	{
		return;
	}
	elsif ($line =~ /^(.*)[\(](.*)[\)](.*)$/)
	{
		processLine( "$2");
		processLine( "$1 $3");
		return;
	}
	$line =~ s/^[.,-]+//g;
	$line =~ s/[\\~?!\%\*\$\=\:\;\|\^\&\#\@'"”“\-–\/\)\(\[\]\}\{]/ /g;
	$line =~ s/\s\s+/ /g;
	$line =~ s/^\s+//g;
	$line =~ s/\s+$//g;
	while ($line =~ /^[a-z]{1,2}\s/ || $line =~ /^[a-z]{1,2}$/)
	{
		$line =~ s/^[a-z]{1,2}\s//;
	}
	push( @lines, $line);
}

my $inputfile = $ARGV[0];
if ($inputfile eq '-')
{
	while (<>)
	{
		chomp;
		processLine( $_);
	}
}
else
{
	open( INPUT, "<$inputfile") or die "Couldn't open file $inputfile, $!";
	while (<INPUT>)
	{
		chomp;
		processLine( $_);
	}
}

my @output = sort @lines;
my $oldline = "";
foreach my $line( @output) {
	if ($line ne $oldline)
	{
		$oldline = $line;
		print "$line\n";
	}
}



