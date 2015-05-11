#!/usr/bin/perl

use strict;
use warnings;
use 5.014;

if ($#ARGV <= 0)
{
	print STDERR "usage: mergeWeights.pl <file1> <file2>\n";
	print STDERR "descr: Merges two files with lines <docid> <nofrefs>\n";
	exit;
}
elsif ($#ARGV > 1)
{
	print STDERR "too many arguments\n";
	print STDERR "usage: mergeWeights.pl <file1> <file2>\n";
	exit;
}

sub parseLine
{
	my ($ln) = @_;
	if ($ln =~ m/^[\"]([^\"]*)[\"][ ]([0-9]*[\.]{0,1}[0-9]+)$/)
	{
		return ($1,$2,1);
	}
	elsif ($ln =~ m/^([^ ]+)[ ]([0-9]*[\.]{0,1}[0-9]+)$/)
	{
		return ($1,$2,0);
	}
	elsif ($ln =~ m/^([\'\"].+)[ ]([0-9]*[\.]{0,1}[0-9]+)$/)
	{
		return (substr($1,1,-1),$2,1);
	}
	else
	{
		die "failed to parse line '$ln'";
	}
}

sub printLine
{
	my ($id,$weight,$hasQuot) = @_;
	my $qt = '';
	if ($hasQuot)
	{
		$qt = '"';
	}
	print "$qt$id$qt $weight\n";
}

sub fetchLine
{
	my ($file) = @_;
	my $ln = readline ($file);
	while ($ln && $ln =~ m/^\s*$/)
	{
		$ln = readline ($file);
	}
	if ($ln)
	{
		return parseLine( $ln);
	}
	else
	{
		return (undef,undef,0);
	}
}

open my $file1, "<$ARGV[0]" or die "failed to open file $ARGV[0] for reading ($!)\n";
open my $file2, "<$ARGV[1]" or die "failed to open file $ARGV[1] for reading ($!)\n";

my ($id1,$weight1,$hasQuot1) = fetchLine( $file1);
my ($id2,$weight2,$hasQuot2) = fetchLine( $file2);

while ($id1 && $id2)
{
	if ($id1 eq $id2)
	{
		printLine( $id1, 0 + $weight1 + $weight2, ($hasQuot1 || $hasQuot2));
		($id1,$weight1,$hasQuot1) = fetchLine( $file1);
		($id2,$weight2,$hasQuot2) = fetchLine( $file2);
	}
	elsif ($id1 lt $id2)
	{
		printLine( $id1, $weight1, $hasQuot1);
		($id1,$weight1,$hasQuot1) = fetchLine( $file1);
	}
	else
	{
		printLine( $id2, $weight2, $hasQuot2);
		($id2,$weight2,$hasQuot2) = fetchLine( $file2);
	}
}

while ($id1)
{
	printLine( $id1, $weight1, $hasQuot1);
	($id1,$weight1,$hasQuot1) = fetchLine( $file1);
}

while ($id2)
{
	printLine( $id2, $weight2, $hasQuot2);
	($id2,$weight2,$hasQuot2) = fetchLine( $file2);
}

close $file1;
close $file2;


