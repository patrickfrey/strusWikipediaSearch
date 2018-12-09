#!/usr/bin/perl

use strict;
use warnings;
use 5.010;

my %term_dict = ();

sub processLine
{
	my ($ln) = @_;
	my @terms = split /\s/, $ln;
	foreach my $term( @terms)
	{
		if (defined $term_dict{ $term })
		{
			$term_dict{ $term } += 1;
		}
		else
		{
			$term_dict{ $term } = 1;
		}
	}
}

while (<STDIN>)
{
	chomp;
	processLine( $_);
}

foreach my $term( keys %term_dict)
{
	print "$term $term_dict{ $term }\n";
}



