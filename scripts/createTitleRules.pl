#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;
use feature qw( unicode_strings );
use open qw/:std :utf8/;

if ($#ARGV < 0 || $#ARGV > 4)
{
	print STDERR "usage: createTitleRules.pl <infile> [<restype>] [<normop>]\n";
	print STDERR "       <infile>       :file ('-' for stdin) with title lines to process\n";
	print STDERR "       <lexem>        :lexem term type name (default 'lexem').\n";
	print STDERR "       <restype>      :result type name 'name' or prefix (default 'name').\n";
	print STDERR "       <normop>       :normalizer of tokens 'lc' or '' (default '').\n";
	exit;
}

my $infilename = $ARGV[0];
my $lexemtype = "lexem";
my $restype = "name";
my $normop = "";

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

my %rule_dict = ();
my $featno = 0;
my $hsymcnt = 0;

sub getLexem
{
	my ($term) = @_;
	if ($normop eq "lc")
	{
		return "$lexemtype \"" . lc($term) . "\"";
	}
	elsif ($normop eq "")
	{
		return "$lexemtype \"$term\"";
	}
	else
	{
		die "unknown norm op parameter passed";
	}
}

sub printSingleTermRule
{
	my ($result,$arg1) = @_;
	my $lexem1 = getLexem( $arg1);
	print "$result = $lexem1;\n"
}

sub printTermRule
{
	my ($result,$arg1,$arg2) = @_;
	my $lexem1 = getLexem( $arg1);
	my $lexem2 = getLexem( $arg2);
	print "$result = sequence_imm( $lexem1, $lexem2);\n"
}

sub printNonTermRule
{
	my ($result,$arg1,$arg2) = @_;
	my $lexem2 = getLexem( $arg2);
	print "$result = sequence_imm( $arg1, $lexem2);\n"
}

sub processLine
{
	my ($ln) = @_;
	if (trim( $ln) eq "")
	{
		return;
	}
	my $feat = $ln;
	$feat =~ s/^_[_]*//g;
	$feat =~ s/_[_]*$//g;
	$featno = $featno + 1;
	my $result = "";
	if ($restype eq "name")
	{
		my $featstr = $feat;
		$featstr =~ s/_[_]*/ /g;
		$result = "\"$featstr\"";
	}
	else
	{
		$result = "$restype$featno";
	}
	$feat =~ s/[\\\.'"]//g;
	if ($feat ne '')
	{
		my @terms = split( /_/, $feat);
		my $tidx = 0;
		if ($#terms >= 0)
		{
			my $termkey = join( '_', @terms);
			if ($normop eq "lc")
			{
				$termkey = lc( $termkey);
			}
			if (defined $rule_dict{ $termkey })
			{
				return;
			}
			else
			{
				$rule_dict{ $termkey } = 1;
			}
			if ($#terms == 0)
			{
				printSingleTermRule( $result, $terms[0]);
			}
			elsif ($#terms == 1)
			{
				printTermRule( $result, $terms[0], $terms[1]);
			}
			else
			{
				$hsymcnt += 1;
				printTermRule( "._$hsymcnt", $terms[0], $terms[1]);
				my $hi = 2;
				while ($hi < $#terms)
				{
					my $nonterminal = "_$hsymcnt";
					$hsymcnt += 1;
					printNonTermRule( "._$hsymcnt", $nonterminal, $terms[$hi]);
					$hi += 1;
				}
				printNonTermRule( "$result", "_$hsymcnt", $terms[$hi]);
			}
		}
	}
}

print '%lexem ' . "$lexemtype\n";
print '%exclusive' . "\n";

while ($_ = <$infile>)
{
	chomp;
	processLine( $_);
}


