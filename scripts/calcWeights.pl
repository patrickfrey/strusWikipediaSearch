#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Math::Trig;

if ($#ARGV <= 0 || $#ARGV > 2)
{
	print STDERR "usage: calcWeights.pl <file> <formula> [<varname>]\n";
	print STDERR "       <file>    :file to process\n";
	print STDERR "       <formula> :formula to calculate\n";
	print STDERR "       <varname> :name of argument variable in formula\n";
	print STDERR "                  (one of x,y,z,a,b,c; default x)\n";
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

open my $file, "<$ARGV[0]" or die "failed to open file $ARGV[0] for reading ($!)\n";
my $evalFormula = $ARGV[1];
my $var = 'x';
if ($#ARGV == 2)
{
	$var = $ARGV[2];
}
$evalFormula =~ s/$var/\$$var/g;

my ($id,$weight,$hasQuot) = fetchLine( $file);

while ($id)
{
	my $x = $weight;
	my $y = $weight;
	my $z = $weight;
	my $a = $weight;
	my $b = $weight;
	my $c = $weight;
	$weight = eval $evalFormula;
	printLine( $id, $weight, $hasQuot);
	($id,$weight,$hasQuot) = fetchLine( $file);
}

close $file;


