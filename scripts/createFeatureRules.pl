#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;
use feature qw( unicode_strings );
use open qw/:std :utf8/;

if ($#ARGV < 0 || $#ARGV > 4)
{
	print STDERR "usage: createFeatureRules.pl <infile> [<lexem>] [<restype>] [<normop>] [<stopwordfile>]\n";
	print STDERR "       <infile>       :file ('-' for stdin) with lines starting with concept no followed\n";
	print STDERR "                       by a colon and a list of multivalue features separated by spaces,\n";
	print STDERR "                       the feature items separated by underscores.\n";
	print STDERR "       <lexem>        :lexem term type name (default 'lexem').\n";
	print STDERR "       <restype>      :result type name 'name' or prefix (default 'name').\n";
	print STDERR "       <normop>       :normalizer of tokens 'lc' or '' (default '').\n";
	print STDERR "       <stopwordfile> :file with terms (stop words) not to use.\n";
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

my %rule_dict = ();
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
	}
	elsif ($ln =~ m/^([0-9]+)\s(.*)$/)
	{
		my $featno = $1;
		my $feat = $2;
		$feat =~ s/^_[_]*//g;
		$feat =~ s/_[_]*$//g;
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
		if (defined $stopword_dict{ $feat })
		{
			return;
		}
		if ($feat ne '')
		{
			my @terms = split( /_/, $feat);
			my $tidx = 0;
			if ($#terms >= 0)
			{
				if ($#terms == 0)
				{
					if (defined $stopword_dict{ $terms[0] })
					{
						return;
					}
				}
				my $termkey = join( '_', @terms);
				if ($normop eq "lc")
				{
					$termkey = lc( $termkey);
				}
				elsif (defined $rule_dict{ $termkey })
				{
					# ... duplicate elimination only if no normop defined
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
	else
	{
		die "syntax error in rule line: $ln";
	}
}

print '%lexem ' . "$lexemtype\n";
print '%exclusive' . "\n";
print '%stopwordOccurrenceFactor = 0.001' . "\n";

while ($_ = <$infile>)
{
	chomp;
	processLine( $_);
}


