#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use utf8;
use utf8; binmode STDOUT, ":utf8";
use open ':std', ':encoding(UTF-8)';
use locale;

my $num_args = $#ARGV + 1;
if ($num_args == 0)
{
	print STDERR "Usage: removeDymDuplicates.pl [<inputfile>]\n";
	print STDERR "<inputfile>: name of input file to process ('-' for stdin)\n";
	exit;
}

my %dict = ();
sub loadLine
{
	my ($line) = @_;
	if (length($line) > 100)
	{
		return;
	}
	my $key = lc($line);
	$key =~ s/s\b//g;
	$key =~ s/a+\s//g;
	$key =~ s/(.)(.*)\1/$1/g;
	$key =~ s/\s+$//;
	$key =~ s/^\s+//;
	unless ($key =~ /[A-Za-z ]{2,6}/)
	{
		if (length("$line") < 5)
		{
			return;
		}
	}

	if ($dict{"$key"})
	{
		if (length($line) > length($dict{"$key"}))
		{
			$dict{"$key"} = $line;
		}
		my $uc_cnt_old = () = $dict{"$key"} =~ m/\p{Uppercase}/g;
		my $uc_cnt_new = () = $line =~ m/\p{Uppercase}/g;
		if ($uc_cnt_old < $uc_cnt_new)
		{
			$dict{"$key"} = $line;
		}
	}
	else
	{
		$dict{"$key"} = $line;
	}
}

my $inputfile = $ARGV[0];
if ($inputfile eq '-')
{
	while (<>)
	{
		chomp;
		loadLine( $_);
	}
}
else
{
	open( INPUT, "<$inputfile") or die "Couldn't open file $inputfile, $!";
	while (<INPUT>)
	{
		chomp;
		loadLine( $_);
	}
}

my @lines = ();
foreach my $line (values %dict) {
	push( @lines, $line);
}
my @output = sort @lines;
my $oldline = "";
foreach my $line( @output) {
	if ($line ne $oldline)
	{
		$oldline = $line;
		print "$line\n";
	}
}



