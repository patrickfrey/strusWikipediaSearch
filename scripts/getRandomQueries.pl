#!/usr/bin/perl
use strict;
use warnings;

my $nofTerms = $ARGV[0];
my $scheme = $ARGV[1];
my $outputprefix = $ARGV[2];
my $dim = $ARGV[3];
if (!$dim || $dim > 2)
{
	$dim = 2;
}
my $range = 1000;
my $ii = 0;

print '#!/bin/sh' . "\n\n";

for (; $ii<$nofTerms; ++$ii)
{
	my $random_number = int(rand($range));
	my $ri = 0;
	for (; $ri < $random_number && <STDIN>; ++$ri){}
	my $term1 = "";
	my $term2 = "";
	while (<STDIN>)
	{
		if (/[0-9]+[ ]["]([A-Z][a-z]+)["][ ][0-9]+[ ]["]([A-Z][a-z]+)["]/)
		{
			$term1 = $1;
			$term2 = $2;
			last;
		}
	}
	if ($dim == 1)
	{
		print "wget --quiet -O tmp/output.$outputprefix.$ii.htm 'http://demo.project-strus.net/query?s=$scheme&q=$term1'\n";
	}
	else
	{
		print "wget --quiet -O tmp/output.$outputprefix.$ii.htm 'http://demo.project-strus.net/query?s=$scheme&q=$term1\+$term2'\n";
	}
}

