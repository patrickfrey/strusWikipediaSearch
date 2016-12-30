#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;
use feature qw( unicode_strings );
use open qw/:std :utf8/;

if ($#ARGV < 0 || $#ARGV > 2)
{
	print STDERR "usage: createConceptRules.pl <infile> [<restype>] [<srctype>]\n";
	print STDERR "       <infile>  :file ('-' for stdin) with lines starting with concept number\n";
	print STDERR "                  followed by feature numbers.\n";
	print STDERR "       <restype> :result type prefix (default 'C').\n";
	print STDERR "       <srctype> :source type prefix (default 'F').\n";
	exit;
}

my $infilename = $ARGV[0];
my $restype = "C";
my $srctype = "F";
if ($#ARGV >= 1)
{
	$restype = $ARGV[1];
}
if ($#ARGV >= 2)
{
	$srctype = $ARGV[2];
}

my $infile;
if ($infilename eq '-')
{
	$infile = *STDIN;
}
else
{
	open $infile, "<$infilename" or die "failed to open file $infilename for reading ($!)\n";
}

sub trim
{
	my $s = shift; $s =~ s/^\s+|\s+$//g; return $s;
}

sub processLine
{
	my ($ln) = @_;
	if (trim( $ln) eq "")
	{
	}
	elsif ($ln =~ m/^([0-9]+)\s(.*)$/)
	{
		my $conceptno = $1;
		my $rulestr = $2;
		my @featar = split( /\s+/, trim( $rulestr));
		foreach my $feat( @featar)
		{
			print "$restype$conceptno = $srctype$feat;\n";
		}
	}
	else
	{
		die "syntax error in rule line: $ln";
	}
}

while ($_ = <$infile>)
{
	chomp;
	processLine( $_);
}


