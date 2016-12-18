#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;
use feature qw( unicode_strings );
use open qw/:std :utf8/;

if ($#ARGV < 0 || $#ARGV > 2)
{
	print STDERR "usage: createFeatureRules.pl <infile> [<lexem>] [<restype>] [<normop>]\n";
	print STDERR "       <infile>  :file ('-' for stdin) with lines starting with concept no followed\n";
	print STDERR "                  by a colon and a list of multivalue features separated by spaces,\n";
	print STDERR "                  the feature items separated by underscores.\n";
	print STDERR "       <lexem>   :lexem term type name (default 'lexem').\n";
	print STDERR "       <restype> :result type name 'name' or 'idx' (default 'name').\n";
	print STDERR "       <normop>  :normalizer of tokens 'lc' or '' (default '').\n";
	exit;
}

my $infilename = $ARGV[0];
my $lexemtype = "lexem";
my $restype = "name";
my $normop = "";
if ($#ARGV >= 1)
{
	$lexemtype = $ARGV[1];
}
if ($#ARGV >= 2)
{
	$restype = $ARGV[2];
}
if ($#ARGV >= 3)
{
	$normop = $ARGV[3];
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
		my $featno = $1;
		my $feat = $2;
		my $code;
		if ($restype eq "name")
		{
			$featstr = $feat:
			$feat =~ s/_/ /g;
			$code = "\"$featstr\" = ";
		}
		else
		{
			$code = "$featname$featno = ";
		}
		$feat =~ s/['"]//g;
		next if ($feat eq '');

		my @terms = split( /_+/, $feat);
		my $tidx = 0;
		if ($#terms > 0)
		{
			$code .= "sequence_imm( ";
		}
		foreach my $term( @terms)
		{
			next if ($term eq '');
			if ($tidx > 0)
			{
				$code .= ", ";
			}
			$tidx += 1;
			if ($normop eq "lc")
			{
				$code .= "$lexemtype \"" . lc($term) . "\"";
			}
			else if ($normop eq "")
			{
				$code .= "$lexemtype \"$term\"";
			}
			else
			{
				die "unknown norm op parameter passed";
			}
		}
		if ($#terms > 0)
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


