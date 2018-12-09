#!/usr/bin/perl

use strict;
use warnings;
use 5.010;

my %term_dict = ();

sub trim
{
	my $s = shift; $s =~ s/^\s+|\s+$//g; return $s;
}

sub processLine
{
	my ($ln) = @_;
	my ($value, $key) = split /\s/, $ln;
	$key = trim( $key);
	$value = trim( $value);
	if (defined $term_dict{ $key })
	{
		if ($term_dict{ $key } ne $value)
		{
			die "different values '$term_dict{ $key }' and '$value' for same key '$key'";
		}
	}
	else
	{
		$term_dict{ $key } = $value;
	}
}

while (<STDIN>)
{
	chomp;
	processLine( $_);
}

my @output = ();
foreach my $key( keys %term_dict)
{
	push @output, "$term_dict{ $key } $key";
}
@output = sort @output;
foreach my $line( @output)
{
	print "$line\n";
}


