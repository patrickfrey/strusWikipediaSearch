#!/usr/bin/perl

use strict;
use warnings;
use 5.010;
use Text::Unidecode;
use feature qw( unicode_strings );
use open qw/:std :utf8/;

if ($#ARGV < 1 || $#ARGV > 5)
{
	print STDERR "usage: createTitleRules.pl <infile> <redirectsfile> [<restype>] [<normop>] [<stopwordfile>]\n";
	print STDERR "       <infile>        :file ('-' for stdin) with title lines to process\n";
	print STDERR "       <redirectsfile> :file with the redirects to process\n";
	print STDERR "       <lexem>         :lexem term type name (default 'lexem').\n";
	print STDERR "       <restype>       :result type name 'name' or prefix (default 'name').\n";
	print STDERR "       <normop>        :normalizer of tokens 'lc' or '' (default '').\n";
	print STDERR "       <stopwordfile>  :file with terms (stop words) not to use.\n";
	exit;
}

my $infilename = $ARGV[0];
my $redirfilename = $ARGV[1];
my $lexemtype = "lexem";
my $restype = "name";
my $normop = "";
my %stopword_dict = ();
my %redir_dict = ();

sub trim
{
	my $s = shift; $s =~ s/^\s+|\s+$//g; return $s;
}

sub getTermKey
{
	my ($term) = @_;
	if ($normop eq "lc")
	{
		return lc($term);
	}
	elsif ($normop eq "")
	{
		return $term;
	}
	else
	{
		die "unknown norm op parameter passed";
	}
}

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

if ($#ARGV >= 2)
{
	$lexemtype = $ARGV[2];
}
if ($#ARGV >= 3)
{
	$restype = $ARGV[3];
}
if ($#ARGV >= 4)
{
	$normop = $ARGV[4];
}
if ($#ARGV >= 5)
{
	open my $stopwordfile, "<$ARGV[5]" or die "failed to open file $ARGV[5] for reading ($!)\n";
	my $ln = readline ($stopwordfile);
	while ($ln)
	{
		feedStopwordLine( $ln);
		$ln = readline ($stopwordfile);
	}
	close $stopwordfile;
}

sub feedRedirLine
{
	my ($ln) = @_;
	my ($key,$dest) = split /\s/, $ln;
	$dest = getTermKey( trim( $dest));
	$key = trim( $key );
	if (defined $redir_dict{ $dest })
	{
		$redir_dict{ $dest } .= "\n" . $key;
	}
	else
	{
		$redir_dict{ $dest } = $key;
	}
}

open my $redirfile, "<$redirfilename" or die "failed to open file $redirfilename for reading ($!)\n";
my $ln = readline ($redirfile);
while ($ln)
{
	feedRedirLine( $ln);
	$ln = readline ($redirfile);
}
close $redirfile;


my $infile;
if ($infilename eq '-')
{
	$infile = *STDIN;
}
else
{
	open $infile, "<$infilename" or die "failed to open file $infilename for reading ($!)\n";
}

my %rule_dict = ();
my $featno = 0;

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
	my ($result,$arg1,$arg2,$range) = @_;
	my $lexem2 = getLexem( $arg2);
	print "$result = sequence_imm( $arg1, $lexem2 | $range);\n"
}

my %sub_pattern_map = ();
my %sub_pattern_length_map = ();
my $sub_pattern_cnt = 0;
my $sub_pattern_lastcnt = 0;
sub getSubPatternId
{
	my ($key) = @_;
	if (defined $sub_pattern_map{ $key })
	{
		return $sub_pattern_map{ $key }
	}
	else
	{
		$sub_pattern_cnt += 1;
		$sub_pattern_map{ $key } = $sub_pattern_cnt;
		return $sub_pattern_cnt;
	}
}

sub printRules
{
	my ($result,$feat) = @_;
	$feat =~ s/^_[_]*//g;
	$feat =~ s/_[_]*$//g;
	$feat =~ s/_[_]*/_/g;
	$feat =~ s/_[_]*/_/g;
	my @terms = split( /_/, $feat);
	my $tidx = 0;
	if ($#terms >= 0)
	{
		my $termkey = getTermKey( join( "_", @terms));
		if (defined $stopword_dict{ $termkey })
		{
			return;
		}
		if (defined $rule_dict{ $termkey })
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
			my $patternid = getSubPatternId( getTermKey($terms[0]) . "_" . getTermKey($terms[1]));
			if ($patternid == $sub_pattern_cnt && $patternid != $sub_pattern_lastcnt)
			{
				$sub_pattern_length_map{ $patternid } = 2;
				printTermRule( "._$patternid", $terms[0], $terms[1]);
				$sub_pattern_lastcnt = $patternid;
			}
			my $hi = 2;
			while ($hi < $#terms)
			{
				my $nonterminal = "_$patternid";
				my $nonterminal_length = $sub_pattern_length_map{ $patternid };

				$patternid = getSubPatternId( $nonterminal . "_" . getTermKey($terms[$hi]));
				if ($patternid == $sub_pattern_cnt && $patternid != $sub_pattern_lastcnt)
				{
					$sub_pattern_length_map{ $patternid } = $nonterminal_length + 1;
					printNonTermRule( "._$patternid", $nonterminal, $terms[$hi], $nonterminal_length + 1);
					$sub_pattern_lastcnt = $patternid;
				}
				$hi += 1;
			}
			my $nonterminal_length = $sub_pattern_length_map{ $patternid };
			printNonTermRule( "$result", "_$patternid", $terms[$hi], $nonterminal_length + 1);
		}
		if (defined $redir_dict{ $termkey })
		{
			my @redirsources = split( /\n/, $redir_dict{ $termkey } );
			foreach my $rsrc (@redirsources)
			{
				printRules( $result, $rsrc );
			}
		}
	}
}

sub getResultId
{
	my ($featno,$feat) = @_;

	$feat =~ s/^_[_]*//g;
	$feat =~ s/_[_]*$//g;
	if ($restype eq "name")
	{
		my $featstr = $feat;
		$featstr =~ s/_[_]*/ /g;
		$featstr =~ s/[ ][ ]*/ /g;
		return "\"$featstr\"";
	}
	else
	{
		return "$restype$featno";
	}
}

sub processLine
{
	my ($ln) = @_;
	if (trim( $ln) eq "")
	{
		return;
	}
	my $feat = $ln;
	$feat =~ s/\s.*$//;	# ... cut away everything after first space
	$featno = $featno + 1;
	$feat =~ s/[\\\.'"]//g;
	my $result = getResultId( $featno, $feat);
	if ($feat ne '')
	{
		printRules( $result, $feat);
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


