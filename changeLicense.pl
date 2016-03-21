#!/usr/bin/perl

use strict;
use warnings;
use 5.010;

my $license = "";
open( LICENSE, "<LICENSEHDR") or die "Couldn't open file LICENSEHDR, $!";
while (<LICENSE>)
{
	$license .= $_;
}
close LICENSE;

my $num_args = $#ARGV + 1;
if ($num_args == 0)
{
	print STDERR "Usage: changeLicense.pl {<inputfile>}\n";
	print STDERR "<inputfile>: name of input file to process\n";
	exit;
}

my @inputfiles = @ARGV;
my $inputfile;

foreach $inputfile( @inputfiles)
{
	print STDERR "process file $inputfile\n";
	open( INPUT, "<$inputfile") or die "Couldn't open file $inputfile, $!";
	my $state = 0;
	my $output = undef;
	my $linecnt = 0;

	while (<INPUT>)
	{
		$linecnt = $linecnt + 1;
		my $line = $_;
		if ($state == 0 && $linecnt == 1)
		{
			if ($line =~ m/^[ ]*([\/][\*].*)/)
			{
				$state = 1;
			}
			else
			{
				last;
			}
		}
		elsif ($state == 1)
		{
			if ($line =~ m/([\*][\/])/)
			{
				last;
			}
			elsif ($line =~ m/License/)
			{
				$state = 2;
			}
		}
		elsif ($state == 2)
		{
			if ($line =~ m/([\*][\/])/)
			{
				$state = 3;
				$output = $license;
			}
		}
		elsif ($state == 3)
		{
			$output .= $line;
		}
	}
	close INPUT;

	if ($output)
	{
		open( OUTPUT, ">$inputfile") or die "Couldn't open file $inputfile, $!";
		print OUTPUT $output;
		close OUTPUT;
	}
	else
	{
		print STDERR "no license header in file $inputfile\n";
	}
}

