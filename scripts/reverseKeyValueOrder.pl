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
	print "$key $value\n";
}

while (<STDIN>)
{
	chomp;
	processLine( $_);
}

