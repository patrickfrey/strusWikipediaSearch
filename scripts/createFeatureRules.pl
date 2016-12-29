#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;
use feature qw( unicode_strings );
use open qw/:std :utf8/;

if ($#ARGV < 0 || $#ARGV > 3)
{
	print STDERR "usage: createFeatureRules.pl <infile> [<lexem>] [<restype>] [<normop>] [<stopwordfile>]\n";
	print STDERR "       <infile>       :file ('-' for stdin) with lines starting with concept no followed\n";
	print STDERR "                       by a colon and a list of multivalue features separated by spaces,\n";
	print STDERR "                       the feature items separated by underscores.\n";
	print STDERR "       <stopwordfile> :file with terms (stop words) not to use.\n";
	print STDERR "       <lexem>        :lexem term type name (default 'lexem').\n";
	print STDERR "       <restype>      :result type name 'name' or 'idx' (default 'name').\n";
	print STDERR "       <normop>       :normalizer of tokens 'lc' or '' (default '').\n";
	exit;
}

my $infilename = $ARGV[0];
my $lexemtype = "lexem";
my $restype = "name";
my $normop = "";
my %stopword_dict = ();
sub feedStopwordLine
{
	my ($ln) = @_;
	my ($term,$cnt) = split /\s/, $ln;
	if (defined $stopword_dict{ $term })
	{
		$stopword_dict{ $term } += $cnt;
	}
	else
	{
		$stopword_dict{ $term } = $cnt;
	}
}

if ($#ARGV >= 1)
{
	$lexemtype = $ARGV[1];
}
if ($#ARGV >= 2)
{
	$restype = $ARGV[2];
}
if ($#ARGV >= 3)
{
	$normop = $ARGV[3];
}
if ($#ARGV >= 4)
{
	open my $stopwordfile, "<$ARGV[4]" or die "failed to open file $ARGV[0] for reading ($!)\n";
	my $ln = readline ($stopwordfile);
	while ($ln)
	{
		feedStopwordLine( $ln);
		$ln = readline ($stopwordfile);
	}
	close $stopwordfile;
}

my $infile;
if ($infilename eq '-')
{
	$infile = *STDIN;
}
else
{
	open $infile, "<$infilename" or die "failed to open file $infilename for reading ($!)\n";
}

sub trim
{
	my $s = shift; $s =~ s/^\s+|\s+$//g; return $s;
}

sub processLine
{
	my ($ln) = @_;
	if (trim( $ln) eq "")
	{
	}
	elsif ($ln =~ m/^([0-9]+)\s(.*)$/)
	{
		my $featno = $1;
		my $feat = $2;
		my $code = "";
		if (defined $stopword_dict{ $feat })
		{
			return;
		}
		if ($restype eq "name")
		{
			my $featstr = $feat;
			$featstr =~ s/^_+//g;
			$featstr =~ s/_+$//g;
			$featstr =~ s/_+/ /g;
			$code = "\"$featstr\" = ";
		}
		else
		{
			$code = "$restype$featno = ";
		}
		$feat =~ s/[\\\.'"]//g;
		if ($feat ne '')
		{
			my @terms = split( /_+/, $feat);
			my $tidx = 0;
			if ($#terms > 0)
			{
				$code .= "sequence_imm( ";
				foreach my $term( @terms)
				{
					next if ($term eq '');
					if ($tidx > 0)
					{
						$code .= ", ";
					}
					$tidx += 1;
					if ($normop eq "lc")
					{
						$code .= "$lexemtype \"" . lc($term) . "\"";
					}
					elsif ($normop eq "")
					{
						$code .= "$lexemtype \"$term\"";
					}
					else
					{
						die "unknown norm op parameter passed";
					}
				}
				$code .= " );";
				print "$code\n";
			}
		}
	}
	else
	{
		die "syntax error in rule line: $ln";
	}
}

print '%lexem ' . "$lexemtype\n";
print '%exclusive' . "\n";

while ($_ = <$infile>)
{
	chomp;
	processLine( $_);
}


