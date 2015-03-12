#!/usr/bin/perl

use strict;
use warnings;

while (<>)
{
	chomp;
	my @line = split /[ \/]/;
	my $first = lc( shift @line);
	my $elem;
	foreach $elem( @line)
	{
		my $word = lc( $elem);
		if ($word =~ /[a-z]/)
		{
			print $first . " " . $word . "\n";
		}
	}
}
