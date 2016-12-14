#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;

if ($#ARGV < 0 || $#ARGV > 2)
{
	print STDERR "usage: createConceptRules.pl <infile> [<lexem>] [<nampref>]\n";
	print STDERR "       <infile>  :file ('-' for stdin) with lines starting with concept no followed\n";
	print STDERR "                  by a colon and a list of multivalue features separated by spaces,\n";
	print STDERR "                  the feature items separated by underscores.\n";
	print STDERR "       <nampref> :name prefix for concepts created (default '').\n";
	print STDERR "       <lexem>   :lexem term type name (default 'lexem').\n";
	exit;
}

my $infilename = $ARGV[0];
my $lexemtype = "lexem";
my $nameprefix = "C";
if ($#ARGV >= 1)
{
	$lexemtype = $ARGV[1];
}
if ($#ARGV >= 2)
{
	$nameprefix = $ARGV[2];
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
	elsif ($ln =~ m/^([0-9]+)[:](.*)$/)
	{
		my $conceptno = $1;
		my $rulestr = $2;
		my @featar = split( /\s+/, trim( $rulestr));
		my $fidx = 0;
		my $code = "$nameprefix$conceptno = ";
		if ($#featar > 0)
		{
			$code .= "any( "
		}
		foreach my $feat( @featar)
		{
			if ($fidx > 0)
			{
				$code .= ", ";
			}
			$fidx += 1;
			my @terms = split( /_+/, $feat);
			my $tidx = 0;
			if ($#terms > 0)
			{
				$code .= "sequence_imm( ";
			}
			foreach my $term( @terms)
			{
				if ($tidx > 0)
				{
					$code .= ", ";
				}
				$tidx += 1;
				$code .= "$lexemtype \"$term\"";
			}
			if ($#terms > 0)
			{
				$code .= " )";
			}
		}
		if ($#featar > 0)
		{
			$code .= " );";
		}
		else
		{
			$code .= ";";
		}
		print "$code\n";
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


