#!/usr/bin/perl

use strict;
use warnings;
use 5.014;

if ($#ARGV <= 0 || $#ARGV > 2)
{
	print STDERR "usage: calcDocidRefs.pl <docid_list> <linkid_list>\n";
	print STDERR "       <docid_list>   :file with list of tuples <docno> <docid>\n";
	print STDERR "                       created with strusInspect -s <storage> attribute title\n";
	print STDERR "       <linkid_list>  :file with list of tuples <docid> <nofrefs>\n";
	print STDERR "                       created with strusInspect -s <storage> fwstats linkid\n";
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

sub normalizeLinkDocid
{
	my ($id) = @_;
	$id =~ s/^[ ]+(.*)$/$1/;	# remove leading spaces
	$id =~ s/^[\#](.*)$/$1/;	# remove leading '#'
	$id =~ s/^([^\#]*)[\#].*$/$1/;	# remove sub page references
	$id =~ s/^(.*)[ ]+$/$1/;	# remove trailing spaces
	return $id;
}

sub parseLinkLine
{
	my ($ln) = @_;
	if ($ln =~ m/^[\'](.*)[\'][ ]([0-9]+)$/)
	{
		my $docid = normalizeLinkDocid($1);
		my $cnt = $2;
		return ($docid,$cnt);
	}
	else
	{
		die "failed to parse line '$ln'";
	}
}

sub printDocnoCountLine
{
	my ($id,$cnt) = @_;
	print "$id $cnt\n";
}

sub fetchDocidLine
{
	my ($file) = @_;
	my $ln = readline ($file);
	while ($ln && $ln =~ m/^\s*$/)
	{
		$ln = readline ($file);
	}
	if ($ln)
	{
		return parseDocidLine( $ln);
	}
	else
	{
		return (undef,undef,0);
	}
}

sub fetchLinkLine
{
	my ($file) = @_;
	my $ln = readline ($file);
	while ($ln && $ln =~ m/^\s*$/)
	{
		$ln = readline ($file);
	}
	if ($ln)
	{
		return parseLinkLine( $ln);
	}
	else
	{
		return (undef,undef,0);
	}
}


open my $docidfile, "<$ARGV[0]" or die "failed to open file $ARGV[0] for reading ($!)\n";
open my $linkfile, "<$ARGV[1]" or die "failed to open file $ARGV[1] for reading ($!)\n";

my %docidtab = ();
my ($docid,$docno) = fetchDocidLine( $docidfile);
while ($docid)
{
	$docidtab{ $docid} = $docno;
	($docid,$docno) = fetchDocidLine( $docidfile);
}

my %linktab = (); # map docno -> refcnt
my ($linkid,$linkcnt) = fetchLinkLine( $linkfile);
while ($linkid)
{
	$docno = $docidtab{ $linkid};
	if ($docno)
	{
		if ($linktab{ $docno})
		{
			$linktab{ $docno} += $linkcnt;
		}
		else
		{
			$linktab{ $docno} = $linkcnt;
		}
	}
	($linkid,$linkcnt) = fetchLinkLine( $linkfile);
}

close $docidfile;
close $linkfile;

foreach $docno (sort(keys %linktab))
{
	printDocnoCountLine( $docno, $linktab{ $docno});
}




