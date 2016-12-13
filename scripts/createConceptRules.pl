#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;

if ($#ARGV <= 0 || $#ARGV > 2)
{
	print STDERR "usage: createConceptRules.pl <prefix> <infile> <outfile>\n";
	print STDERR "       <infile>       :file with lines starting with concept no folowed by colon\n";
	print STDERR "                       and a list of multivalue features separated by spaces,\n";
	print STDERR "                       the feature items separated by underscores.\n";
	print STDERR "       <outfile>      :the pattern matching rule file to write\n";
	exit;
}

sub parseDocidLine
{
	my ($ln) = @_;
	if ($ln =~ m/^([0-9]+)[ ](.*)$/)
	{
		return ($2,$1);
	}
	else
	{
		die "failed to parse docid line '$ln'";
	}
}



C123 = any( sequence_imm( lexem "A", lexem "B" | 3), sequence_imm( lexem "C", lexem "D" | 3) ;
