<!DOCTYPE html>
<html>
<head>
<title>Search Wikipedia with Strus</title>
<link href="strus.css" rel="stylesheet" type="text/css">
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
</head>
<body>
<h1>Project Strus: A demo search engine for Wikipedia (english)</h1>
<div id="search_form">
<div id="search_elements">
<div id="search_logo">
 <a target="_blank" href="http://project-strus.net">
  <img style="display:block;" width="100%" src="strus_logo.jpg" alt="strus logo"/>
<!-- Copyright: <a href='http://www.123rf.com/profile_guarding123'>guarding123 / 123RF Stock Photo</a>
-->
 </a>
</div>

<?php
require "strus.php";

function evalQueryText( $context, $scheme, $queryString, $minRank, $maxNofRanks, $restrictdoc)
{
	$storage = $context->createStorageClient( "" );
	$analyzer = $context->createQueryAnalyzer();
	$queryeval = $context->createQueryEval();

	$analyzer->definePhraseType( "text", "stem", "word", 
			array( "lc",
			array( "dictmap", "irregular_verbs_en.txt" ),
			array( "stem", "en" ),
			array("convdia", "en" ),
			"lc"));

	$queryeval->addTerm( "sentence", "sent", "");
	$queryeval->addTerm( "para", "para", "");
	if (!$scheme || $scheme == 'BM25')
	{
		$queryeval->addWeightingFunction( 1.0, "BM25", array(
				"k1" => 0.75, "b" => 2.1, "avgdoclen" => 500,
				".match" => "docfeat" ));
	}
	elseif ($scheme == 'BM25pff')
	{
		$queryeval->addWeightingFunction( 1.0, "BM25pff", array(
				"k1" => 1.5, "b" => 0.75, "avgdoclen" => 500,
				"metadata_title_maxpos" => "maxpos_title", "metadata_doclen" => "doclen",
				"titleinc" => 2.4, "windowsize" => 40, cardinality => 0, "ffbase" => 0.4,
				"maxdf" => 0.4,
				".para" => "para", ".struct" => "sentence", ".match" => "docfeat" ));
	}
	$queryeval->addWeightingFunction( 2.0, "metadata", array( "name" => "pageweight" ) );

	$queryeval->addSummarizer( "TITLE", "attribute", array( "name" => "title" ) );
	$queryeval->addSummarizer( "CONTENT", "matchphrase", array(
			"type" => "orig", "metadata_title_maxpos" => "maxpos_title",
			"windowsize" => 40, "sentencesize" => 100, cardinality => 3,
			"matchmark" => '$#HL#$#/HL#',
			".para" => "para", ".struct" => "sentence", ".match" => "docfeat" ) );
	$queryeval->addSelectionFeature( "selfeat");

	$query = $queryeval->createQuery( $storage);
	$terms = $analyzer->analyzePhrase( "text", $queryString);
	if (count( $terms) > 0)
	{
		$selexpr = array( "contains");
		foreach ($terms as &$term)
		{
			$query->defineFeature( "docfeat", array( $term->type, $term->value ), 1.0);
			$selexpr[] = array( $term->type, $term->value );
		}
		$query->defineFeature( "selfeat", $selexpr, 1.0);
	}
	$query->setMaxNofRanks( $maxNofRanks);
	$query->setMinRank( $minRank);
	if ($restrictdoc)
	{
		$query->addDocumentEvaluationSet( [ $restrictdoc ] );
	}
	return $query->evaluate();
}

function getPermutations( $nn)
{
	$rt = array();
	$ii=0;
	while ($ii<$nn)
	{
		$kk=0;
		while ($kk<$nn)
		{
			if ($ii != $kk)
			{
				array_push( $rt, array( $ii, $kk));
			}
			++$kk;
		}
		++$ii;
	}
	return $rt;
}

// The binary node class implementation has been inspired by 'http://www.sitepoint.com/data-structures-2'
class BinaryNode
{
	public $link;		// Id of the link
	public $weight;		// weight of the link
	public $children;	// number of children
	public $left;		// the left child BinaryNode
	public $right;		// the right child BinaryNode

	public function __construct( $link, $weight) {
		$this->link = $link;
		$this->weight = $weight;
		$this->left = null;
		$this->right = null;
		$this->children = 0;
	}
}

class Link
{
	public $title;
	public $weight;

	public function __construct( $title, $weight) {
		$this->title = $title;
		$this->weight = $weight;
	}
}

class LinkSet
{
	private $root;		// the root node of our tree
	private $maxsize;	// maximum number of elements
	private $minRank;	// start of ranklist
	private $nofRanks;	// size of ranklist

	public function __construct( $minRank, $nofRanks) {
		$this->root = null;
		$this->maxsize = $minRank + $nofRanks;
		$this->minRank = $minRank;
		$this->nofRanks = $nofRanks;
	}
	public function isEmpty() {
		return $this->root === null;
	}
	public function size() {
		if ($this->root === null)
		{
			return 0;
		}
		else
		{
			return $this->root->children;
		}
	}
	public function getBestLinks()
	{
		$rt = array();
		if ($this->root)
		{
			$this->getBestLinksNode( $rt, $this->root);
		}
		$nn = count( $rt) - $this->minRank;
		if ($nn <= 0)
		{
			return array();
		}
		if ($nn > $this->nofRanks)
		{
			$nn = $this->nofRanks;
		}
		return array_slice( $rt, $this->minRank, $nn);
	}
	public function insert( $link, $weight) {
		$node = new BinaryNode( $link, $weight);
		if ($this->isEmpty())
		{
			$this->root = $node;
		}
		else
		{
			$this->insertNode( $node, $this->root);
		}
		if ($this->root->left && $this->root->left->children > $this->maxsize)
		{
			$this->root = $this->root->left;
		}
	}

	private function getBestLinksNode( &$ar, $subtree)
	{
		if ($subtree->left)
		{
			$this->getBestLinksNode( $ar, $subtree->left);
		}
		array_push( $ar, new Link( $subtree->link, $subtree->weight));
		if ($subtree->right)
		{
			$this->getBestLinksNode( $ar, $subtree->right);
		}
	}
	private function insertNode($node, &$subtree) {
		if ($subtree === null) {
			$subtree = $node;
			$subtree->children = 1;
		}
		else {
			if ($node->weight <= $subtree->weight) {
				$this->insertNode( $node, $subtree->right);
			}
			else
			{
				$this->insertNode( $node, $subtree->left);
			}
			$subtree->children += 1;
		}
	}
}

function evalQueryNBLNK( $context, $queryString, $minRank, $maxNofRanks)
{
	$storage = $context->createStorageClient( "" );
	$analyzer = $context->createQueryAnalyzer();
	$queryeval = $context->createQueryEval();
	
	$analyzer->definePhraseType( "text", "stem", "word", 
			array( "lc",
			array( "dictmap", "irregular_verbs_en.txt" ),
			array( "stem", "en" ),
			array("convdia", "en" ),
			"lc"));
	
	$queryeval->addWeightingFunction( 1.0, "BM25", array(
			"k1" => 0.75, "b" => 2.1, "avgdoclen" => 500,
			".match" => "docfeat" ));
	$queryeval->addWeightingFunction( 2.0, "metadata", array( "name" => "pageweight" ) );

	$queryeval->addSummarizer(
			"LINK", "accuvariable", array(
				".match" => "sumfeat",
				"var" => "LINK",
				"type" => "linkid"
			) );
	$queryeval->addSelectionFeature( "selfeat");
	
	$query = $queryeval->createQuery( $storage);
	$terms = $analyzer->analyzePhrase( "text", $queryString);
	if (count( $terms) > 0)
	{
		if (count( $terms) > 1)
		{
			$pairs = getPermutations( count( $terms));
			foreach( $pairs as &$pair)
			{
				$term1 = $terms[ $pair[0]];
				$term2 = $terms[ $pair[1]];

				if ($pair[0]+1 == $pair[1])
				{
					$expr = array(
						array( "sequence_struct", 3,
							array( "sent"),
							array( $term1->type, $term1->value),
							array( $term2->type, $term2->value)
						),
						array( "sequence_struct", 3,
							array( "sent"),
							array( $term2->type, $term2->value),
							array( $term1->type, $term1->value)
						),
						array( "within_struct", 5,
							array( "sent"),
							array( $term1->type, $term1->value),
							array( $term2->type, $term2->value)
						),
						array( "within_struct", 20,
							array( "sent"),
							array( $term1->type, $term1->value),
							array( $term2->type, $term2->value)
						)
					);
					$weight = array( 3.0, 2.0, 2.0, 1.5 );
					$ii = 0;
					while ($ii < 4)
					{
						$sumexpr = array( "chain_struct", 50, array( "sent"),
								array( "=LINK", "linkvar"), $expr[ $ii] );
						$query->defineFeature( "sumfeat", $sumexpr, $weight[ $ii] );
						$sumexpr = array( "sequence_struct", -50, array( "sent"),
								$expr[ $ii], array( "=LINK", "linkvar") );
						$query->defineFeature( "sumfeat", $sumexpr, $weight[ $ii] );
						++$ii;
					}
				}
				elseif ($pair[0]+2 < $pair[1])
				{
					$expr = array( "within_struct", 20,
							array( "sent"),
							array( $term1->type, $term1->value),
							array( $term2->type, $term2->value)
						);
					$weight = 1.1;

					$sumexpr = array( "inrange_struct", 50, array( "sent"),
							array( "=LINK", "linkvar"), $expr );
					$query->defineFeature( "sumfeat", $sumexpr, $weight );
				}
				elseif ($pair[0] < $pair[1])
				{
					$expr = array(
							array( "within_struct", 5,
								array( "sent"),
								array( $term1->type, $term1->value),
								array( $term2->type, $term2->value)
							),
							array( "within_struct", 20,
								array( "sent"),
								array( $term1->type, $term1->value),
								array( $term2->type, $term2->value)
							)
					);
					$weight = array( 1.6, 1.2 );
					$ii = 0;
					while ($ii < 2)
					{
						# The summarization expression attaches a variable 
						# LINK ("=LINK") to links (terms of type 'linkvar'):
						$sumexpr = array( "chain_struct", 50, array( "sent"),
								array( "=LINK", "linkvar"), $expr[ $ii] );
						$query->defineFeature( "sumfeat", $sumexpr, $weight[ $ii] );
						$sumexpr = array( "sequence_struct", -50, array( "sent"),
								$expr[ $ii], array( "=LINK", "linkvar") );
						$query->defineFeature( "sumfeat", $sumexpr, $weight[ $ii] );
						++$ii;
					}
				}
			}
		}
		else
		{
			$expr = array( $terms[0]->type, $terms[0]->value);
			# The summarization expression attaches a variable 
			# LINK ("=LINK") to links (terms of type 'linkvar'):
			$sumexpr = array( "chain_struct", 50, array( "sent"),
					array( "=LINK", "linkvar"), $expr );
			$query->defineFeature( "sumfeat", $sumexpr, 1.0 );
			$sumexpr = array( "sequence_struct", -50, array( "sent"),
					$expr, array( "=LINK", "linkvar") );
			$query->defineFeature( "sumfeat", $sumexpr, 1.0 );
		}
		$selexpr = array( "contains");
		foreach ($terms as &$term)
		{
			$selexpr[] = array( $term->type, $term->value );
			$query->defineFeature( "docfeat", array( $term->type, $term->value), 1.0 );
		}
		$query->defineFeature( "selfeat", $selexpr, 1.0);
	}
	$query->setMaxNofRanks( 300);
	$query->setMinRank( 0);

	$candidates = $query->evaluate();

	$linktab = array();
	foreach ($candidates as &$candidate)
	{
		foreach( $candidate->attributes as &$attrib)
		{
			if( strcmp( $attrib->name, 'LINK' ) == 0 )
			{
				$lnkid = trim( $attrib->value);
				if (array_key_exists( $lnkid, $linktab))
				{
					$linktab[ $lnkid] = 0.0 + $linktab[ $lnkid] + $attrib->weight * $candidate->weight;
				}
				else
				{
					$linktab[ $lnkid] = 0.0 + $attrib->weight * $candidate->weight;
				}
			}
		}
	}

	# Extract the top weighted documents in the linktable as result:
	$bestn = new LinkSet( $minRank, $maxNofRanks);
	if (empty( $linktab))
	{
		return array();
	}
	foreach ($linktab as $link => $weight)
	{
		$bestn->insert( $link, $weight);
	}

	$rt = $bestn->getBestLinks();
	return $rt;
}

try {
	$minRank = 0;
	$nofRanks = 20;
	$scheme = NULL;
	$restrictdoc = NULL;
	parse_str( getenv('QUERY_STRING'), $_GET);
	$queryString = $_GET['q'];
	if (array_key_exists( 'n', $_GET))
	{
		$nofRanks = intval( $_GET['n']);
	}
	if (array_key_exists( 'i', $_GET))
	{
		$minRank = intval( $_GET['i']);
	}
	if (array_key_exists( 's', $_GET))
	{
		$scheme = $_GET['s'];
	}
	if (array_key_exists( 'd', $_GET))
	{
		$restrictdoc = intval( $_GET['d']);
	}
	$context = new StrusContext( "localhost:7181" );
	$storage = $context->createStorageClient( "" );

	$BM25_checked = "";
	$BM25pff_checked = "";
	$NBLNK_checked = "";
	if (!$scheme || $scheme == 'BM25')
	{
		$BM25_checked = "checked";
	}
	else if ($scheme == 'BM25pff')
	{
		$BM25pff_checked = "checked";
	}
	else if ($scheme == 'NBLNK')
	{
		$NBLNK_checked = "checked";
	}
	$time_start = microtime(true);
	if (!$scheme || $scheme == 'BM25' || $scheme == 'BM25pff')
	{
		$results = evalQueryText( $context, $scheme, $queryString, $minRank, $nofRanks, $restrictdoc);
	}
	else
	{
		$results = evalQueryNBLNK( $context, $queryString, $minRank, $nofRanks);
	}
	$time_end = microtime(true);
	$query_answer_time = number_format( $time_end - $time_start, 3);

	echo '<form name="search" class method="GET" action="evalQuery.php">';
	echo "<input id=\"search_input\" class=\"textinput\" type=\"text\" maxlength=\"256\" size=\"32\" name=\"q\" tabindex=\"1\" value=\"$queryString\"/>";
	echo "<input type=\"hidden\" name=\"n\" value=\"$nofRanks\"/>";
	echo "<input type=\"hidden\" name=\"d\" value=\"$restrictdoc\"/>";
	echo "<input type=\"radio\" name=\"s\" value=\"NBLNK\" $NBLNK_checked/>NBLNK";
	echo "<input type=\"radio\" name=\"s\" value=\"BM25\" $BM25_checked/>BM25";
	echo "<input type=\"radio\" name=\"s\" value=\"BM25pff\" $BM25pff_checked/>BM25(proximity weighted ff)";
	echo '<input id="search_button" type="image" src="search_button.jpg" tabindex="2"/>';
	echo '</form>';
	echo '</div>';
	echo '</div>';
	echo "<p>query answering time: $query_answer_time seconds</p>";
	if (!$scheme || $scheme == 'BM25' || $scheme == 'BM25pff')
	{
		foreach ($results as &$result)
		{
			$highlight = array( "#HL#", "#/HL#");
			$hlmarkup  = array( "<b>", "</b>");
			$content = '';
			$title = '';
			foreach( $result->attributes as &$attrib ) {
				if( strcmp( $attrib->name, 'TITLE' ) == 0 ) {
					$title = $attrib->value;
				}
				if( strcmp( $attrib->name, 'CONTENT' ) == 0 ) {
					if( strcmp( $content, "" ) != 0 ) {
						$content .= " --- ";
					}
					$content .= str_replace( $highlight, $hlmarkup, htmlspecialchars( $attrib->value));
				}
				$link = strtr ($title, array (' ' => '_'));
			}
			echo '<div id="search_rank">';
				echo '<div id="rank_docno">' . "$result->docno</div>";
				echo '<div id="rank_weight">' . number_format( $result->weight, 4) . "</div>";
				echo '<div id="rank_content">';
					echo '<div id="rank_title">' . "<a href=\"http://en.wikipedia.org/wiki/$link\">$title</a></div>";
					echo '<div id="rank_summary">' . "$content</div>";
				echo '</div>';
			echo '</div>';
		}
	}
	else
	{
		foreach ($results as &$result)
		{
			$link = strtr ($result->title, array (' ' => '_'));
			echo '<div id="search_rank">';
				echo '<div id="rank_weight">' . number_format( $result->weight, 4) . "</div>";
				echo '<div id="rank_content">';
					echo '<div id="rank_title">' . "<a href=\"http://en.wikipedia.org/wiki/$link\">$result->title</a></div>";
				echo '</div>';
			echo '</div>';
		}
	}
	echo '</div>';
	echo '<div id="navigation_form">';
	echo '<div id="navigation_elements">';
	$nextMinRank = $minRank + $nofRanks;
	$prevMinRank = $minRank - $nofRanks;
	if ($prevMinRank >= 0)
	{
		echo '<form name="nav_prev" class method="GET" action="evalQuery.php">';
		echo "<input type=\"hidden\" name=\"q\" value=\"$queryString\"/>";
		echo "<input type=\"hidden\" name=\"n\" value=\"$nofRanks\"/>";
		echo "<input type=\"hidden\" name=\"i\" value=\"$prevMinRank\"/>";
		echo "<input type=\"hidden\" name=\"s\" value=\"$scheme\"/>";
		echo '<input id="navigation_prev" type="image" src="arrow-up.png" tabindex="2"/>';
		echo '</form>';
	}
	if (count( $results) >= $nofRanks)
	{
		echo '<form name="nav_next" class method="GET" action="evalQuery.php">';
		echo "<input type=\"hidden\" name=\"q\" value=\"$queryString\">";
		echo "<input type=\"hidden\" name=\"n\" value=\"$nofRanks\">";
		echo "<input type=\"hidden\" name=\"i\" value=\"$nextMinRank\">";
		echo "<input type=\"hidden\" name=\"s\" value=\"$scheme\">";
		echo '<input id="navigation_next" type="image" src="arrow-down.png" tabindex="2"/>';
		echo '</form>';
	}
	$context->close();
	echo '</div>';
	echo '</div>';
}
catch( Exception $e ) {
	echo '<p>';
	echo '<font color="red">';
	echo 'Error: ',  $e->getMessage(), "\n";
	echo '</font>';
	echo '</p>';
}
?>

</body>
</html> 


