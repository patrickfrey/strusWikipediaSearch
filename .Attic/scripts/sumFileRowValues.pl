#!/usr/bin/perl
use strict;
use warnings;

my @sum = ();
my $argidx = 0;
while ($argidx <= $#ARGV)
{
	open( INPUT, "<$ARGV[$argidx]") or die "Couldn't open file $ARGV[$argidx], $!";
	my $linecnt = 0;
	while (<INPUT>)
	{
		if (/[0-9]/)
		{
			chomp;
			my $num = int($_);
			if ($linecnt > $#sum)
			{
				$sum[ $linecnt] = $num;
			}
			else
			{
				$sum[ $linecnt] += $num;
			}
			$linecnt++;
		}
	}
	close( INPUT);
	$argidx++;
}

foreach my $val( @sum)
{
	print "$val\n";
}

