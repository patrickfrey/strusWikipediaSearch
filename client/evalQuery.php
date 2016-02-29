<!DOCTYPE html>
<html>
<head>
<title>Wikipedia search with Strus</title>
<link href="strus.css" rel="stylesheet" type="text/css">
<link rel="icon" href="favicon.ico?v=2" type="image/x-icon">
<meta http-equiv="content-type" content="text/html; charset=utf-8">
</head>
<body>
<div id="navigation">
<div id="logo">
 <a target="_blank" href="http://project-strus.net">
  <img width="100%" src="strus_logo.jpg" alt="strus logo"/>
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
				"k1" => 1.2, "b" => 0.75, "avgdoclen" => 500,
				"metadata_doclen" => "doclen",
				".match" => "docfeat" ));
	}
	elseif ($scheme == 'BM25pff')
	{
		$queryeval->addWeightingFunction( 1.0, "BM25pff", array(
				"k1" => 1.2, "b" => 0.75, "avgdoclen" => 500,
				"metadata_title_maxpos" => "maxpos_title", "metadata_doclen" => "doclen",
				"titleinc" => 2.4, "windowsize" => 40, 'cardinality' => 0, "ffbase" => 0.25,
				"maxdf" => 0.4,
				".para" => "para", ".struct" => "sentence", ".match" => "docfeat" ));
	}
	$queryeval->addWeightingFunction( 1.0, "metadata", array( "name" => "pageweight" ) );

	$queryeval->addSummarizer( "attribute", array( "name" => "docid" ) );
	$queryeval->addSummarizer( "matchphrase", array(
			"type" => "orig", "metadata_title_maxpos" => "maxpos_title",
			"windowsize" => 40, "sentencesize" => 100, 'cardinality' => 2,
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

function evalQueryNBLNK( $context, $queryString, $minRank, $maxNofRanks, $restrictdoc)
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
			"k1" => 1.2, "b" => 0.75, "avgdoclen" => 500,
			".match" => "docfeat" ));
	$queryeval->addWeightingFunction( 1.0, "metadata", array( "name" => "pageweight" ) );

	$queryeval->addSummarizer(
			"accuvariable", array(
				".match" => "sumfeat",
				"norm" => 0.001,
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
	if ($restrictdoc)
	{
		$query->addDocumentEvaluationSet( [ $restrictdoc ] );
	}
	$candidates = $query->evaluate();

	$linktab = array();
	foreach ($candidates->ranks as &$candidate)
	{
		foreach( $candidate->summaryElements as &$sumelem)
		{
			if( strcmp( $sumelem->name, 'LINK' ) == 0 )
			{
				$lnkid = trim( $sumelem->value);
				if (array_key_exists( $lnkid, $linktab))
				{
					$linktab[ $lnkid] = 0.0 + $linktab[ $lnkid] + $sumelem->weight * $candidate->weight;
				}
				else
				{
					$linktab[ $lnkid] = 0.0 + $sumelem->weight * $candidate->weight;
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

function mapResultHeader( $queryString, $duration, $scheme, $docno, $minRank, $maxNofRanks, $hasNext)
{
	if ($docno != 0)
	{
		$minRank = 0;
	}
	$selected_scheme = array("","","");
	if ($scheme == "NBLNK")
	{
		$selected_scheme[0] = " selected";
	}
	elseif ($scheme == "BM25")
	{
		$selected_scheme[1] = " selected";
	}
	elseif ($scheme == "BM25pff")
	{
		$selected_scheme[2] = " selected";
	}
	echo "<div id=\"toolbar\">";
	echo "<form id=\"searchbox\" name=\"search\" class method=\"GET\" action=\"evalQuery.php\">";
	echo "<select name=\"s\" id=\"scheme\">";
	echo " <option$selected_scheme[0]>NBLNK</option>";
	echo " <option$selected_scheme[1]>BM25</option>";
	echo " <option$selected_scheme[2]>BM25pff</option>";
	echo "</select>";
	echo "<input id=\"search\" class=\"textinput\" type=\"text\" maxlength=\"256\" size=\"32\" name=\"q\" tabindex=\"1\" value=\"$queryString\"/>";
	echo "<input id=\"submit\" type=\"submit\" value=\"Search\" />";
	echo "<input type=\"hidden\" name=\"n\" value=\"$maxNofRanks\"/>";
	echo "</form>";
	if ($minRank > 0 && $docno == 0)
	{
		$prevRank = $minRank - $maxNofRanks;
		if ($prevRank < 0)
		{
			$prevRank = 0;
		}
		echo "<form id=\"navprev\" name=\"prev\" class method=\"GET\" action=\"evalQuery.php\">";
		echo "<input type=\"hidden\" name=\"s\" value=\"$scheme\"/>";
		echo "<input type=\"hidden\" name=\"q\" value=\"$queryString\"/>";
		echo "<input id=\"submit\" type=\"submit\" value=\"<<\" />";
		echo "<input type=\"hidden\" name=\"i\" value=\"$prevRank\"/>";
		echo "<input type=\"hidden\" name=\"n\" value=\"$maxNofRanks\"/>";
		echo "</form>";
	}
	if ($hasNext != 0 && $docno == 0)
	{
		$nextRank = $minRank + $maxNofRanks;
		echo "<form id=\"navnext\" name=\"next\" class method=\"GET\" action=\"evalQuery.php\">";
		echo "<input type=\"hidden\" name=\"s\" value=\"$scheme\"/>";
		echo "<input type=\"hidden\" name=\"q\" value=\"$queryString\"/>";
		echo "<input id=\"submit\" type=\"submit\" value=\">>\" />";
		echo "<input type=\"hidden\" name=\"i\" value=\"$nextRank\"/>";
		echo "<input type=\"hidden\" name=\"n\" value=\"$maxNofRanks\"/>";
		echo "</form>";
	}
	echo "</div>";
	echo "</div>";
	echo "<div id=\"searchinfo\">";
	echo "<p>Query answering time: $duration seconds</p>";
	echo "</div>";
}

try {
	$minRank = 0;
	$maxNofRanks = 6;
	$scheme = NULL;
	$restrictdoc = NULL;
	parse_str( getenv('QUERY_STRING'), $_GET);
	$queryString = $_GET['q'];
	if (array_key_exists( 'n', $_GET))
	{
		$maxNofRanks = intval( $_GET['n']);
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

	$time_start = microtime(true);
	if (!$scheme || $scheme == 'BM25' || $scheme == 'BM25pff')
	{
		$result = evalQueryText( $context, $scheme, $queryString, $minRank, $maxNofRanks, $restrictdoc);
		$nof_results = count( $result->ranks );
	}
	else
	{
		$result = evalQueryNBLNK( $context, $queryString, $minRank, $maxNofRanks, $restrictdoc);
		$nof_results = count( $result );
	}
	$hasNext = ($nof_results == $maxNofRanks);
	$time_end = microtime(true);
	$query_answer_time = number_format( $time_end - $time_start, 3);

	mapResultHeader( $queryString, $query_answer_time, $scheme, $restrictdoc, $minRank, $maxNofRanks, $hasNext);

	echo '<div id="searchresult"><ul>';

	if (!$scheme || $scheme == 'BM25' || $scheme == 'BM25pff')
	{
		foreach ($result->ranks as &$rank)
		{
			$highlight = array( "#HL#", "#/HL#");
			$hlmarkup  = array( "<b>", "</b>");
			$content = '';
			$title = '';
			foreach( $rank->summaryElements as &$sumelem ) {
				if( strcmp( $sumelem->name, 'docid' ) == 0 ) {
					$title = $sumelem->value;
				}
				if( strcmp( $sumelem->name, 'phrase' ) == 0
				|| strcmp( $sumelem->name, 'docstart' ) == 0) {
					if( strcmp( $content, "" ) != 0 ) {
						$content .= " --- ";
					}
					$content .= str_replace( $highlight, $hlmarkup, htmlspecialchars( $sumelem->value));
				}
				$link = strtr ($title, array (' ' => '_'));
			}
			echo '<li onclick="parent.location=' . "'https://en.wikipedia.org/wiki/" . $link . "'" . '">';
			echo '<h3>' . $title . '</h3>';
			echo '<div id="rank">';
				echo '<div id="rank_docno">' . "$rank->docno</div>";
				echo '<div id="rank_weight">' . number_format( $rank->weight, 4) . "</div>";
				echo '<div id="rank_abstract">' . "$content</div>";
			echo '</div>';
			echo '</li>';
		}
	}
	else
	{
		foreach ($result as &$rank)
		{
			$link = strtr ($rank->title, array (' ' => '_'));
			echo '<li onclick="parent.location=' . "'https://en.wikipedia.org/wiki/" . $link . "'" . '">';
			echo '<h3>' . $rank->title . '</h3>';
			echo '<div id="search_rank">';
				echo '<div id="rank_weight">' . number_format( $rank->weight, 4) . "</div>";
			echo '</div>';
			echo '</li>';
		}
	}
	echo '</ul></div>';
	$context->close();
}
catch( Exception $e ) {
	echo '<div id="searcherror"><p>';
	echo 'Error: ',  $e->getMessage(), "\n";
	echo '</p></div>';
}
?>

</body>
</html> 


