#!/usr/bin/perl

use strict;
use warnings;
use 5.010;

my $num_args = $#ARGV + 1;
if ($num_args == 0)
{
	print STDERR "Usage: cleanupDym.pl [<inputfile>]\n";
	print STDERR "<inputfile>: name of input file to process ('-' for stdin)\n";
	exit;
}

@lines = ()
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
	elsif (/^(.*)[\(](.*)[\)](.*)$/)
	{
		processLine( $2);
		processLine( "$1 $3");
		return;
	}
	$line =~ s/[\\~?!\#\@'"\-–\/\)\(\[\]]\}\{/ /g;
	$line =~ s/\s\s+/ /g;
	@push( @lines, $line);
}

$inputfile = $ARGV[0];
if ($inputfile == '-')
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
$oldline = "";
foreach $line( @output) {
	if ($line ne $oldline)
	{
		$oldline = $line;
		print "$line\n";
	}
}



