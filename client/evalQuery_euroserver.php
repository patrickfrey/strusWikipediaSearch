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
<div id="search_logo">
 <a target="_blank" href="http://project-strus.net">
  <img style="display:block;" width="100%" height="100%" src="strus_logo.jpg" alt="strus logo">
<!-- Copyright: <a href='http://www.123rf.com/profile_guarding123'>guarding123 / 123RF Stock Photo</a>
-->
 </a>
</div>
<form name="search" class method="GET" action="evalQuery.php">

<?php
require "strus.php";

function evalQuery( $context, $queryString, $nofRanks)
{
	$storage = $context->createStorageClient( "" );
	$analyzer = $context->createQueryAnalyzer();
	$queryeval = $context->createQueryEval();

	$analyzer->definePhraseType( "text", "stem", "word", 
			["lc",
			["dictmap", "irregular_verbs_en.txt"],
			["stem", "en"],
			["convdia", "en"],
			"lc"]);

	$weighting_content = new WeightingFunction( "BM25_dpfc");
	$weighting_content->defineParameter( "k1", 0.75);
	$weighting_content->defineParameter( "b", 2.1);
	$weighting_content->defineParameter( "avgdoclen", 500);
	$weighting_content->defineParameter( "doclen_title", "doclen_tist");
	$weighting_content->defineParameter( "titleinc", 2.0);
	$weighting_content->defineFeature( "match", "docfeat");
	$weighting_content->defineFeature( "title", "title");
	$weighting_popularity = new WeightingFunction( "metadata");
	$weighting_popularity->defineParameter( "name", "pageweight");

	$sumTitle = new Summarizer( "attribute");
	$sumTitle->defineParameter( "name", "title");

	$sumMatch = new Summarizer( "matchphrase");
	$sumMatch->defineParameter( "type", "orig");
	$sumMatch->defineParameter( "len", 80);
	$sumMatch->defineParameter( "nof", 3);
	$sumMatch->defineParameter( "structseek", 30);
	$sumMatch->defineFeature( "struct", "sentence");
	$sumMatch->defineFeature( "match", "docfeat");

	$queryeval->addWeightingFunction( $weighting_content, 1.0);
	$queryeval->addWeightingFunction( $weighting_popularity, 1.0);
	$queryeval->addSummarizer( "TITLE", $sumTitle);
	$queryeval->addSummarizer( "CONTENT", $sumMatch);

	$queryeval->addSelectionFeature( "selfeat");

	$query = $queryeval->createQuery( $storage);

	$terms = $analyzer->analyzePhrase( "text", $queryString);
	foreach ($terms as &$term)
	{
		$query->pushTerm( "stem", $term->value);
		$query->pushDuplicate( "stem", $term->value);
		$query->defineFeature( "docfeat");
	}
	$query->pushExpression( "within", count($terms), 100000);
	$query->defineFeature( "selfeat");

	$query->setMaxNofRanks( $nofRanks);
	$query->setMinRank( 0);

	return $query->evaluate();
}

function mergeResults( $nofranks, $list1, $list2)
{
	$nn = 0;
	$ii = array( 0, 0);
	$ie = array( count( $list1), count( $list2));

	if ($ii[0] < $ie[0] && $ii[1] < $ie[1])
	{
		while ($nn < $nofranks)
		{
			$w1 = $list1[ $ii[ 0]]->weight;
			$w2 = $list1[ $ii[ 1]]->weight;
			if ($w1 < $w2)
			{
				$nn = array_push( $results, $list1[ $ii[ 0]]);
				$ii[ 0] += 1;
			}
			elseif ($w1 > $w2)
			{
				$nn = array_push( $results, $list2[ $ii[ 1]]);
				$ii[ 1] += 1;
			}
			else
			{
				$nn = array_push( $results, $list1[ $ii[ 0]], $list2[ $ii[ 1]]);
				$ii[ 0] += 1;
				$ii[ 1] += 1;
			}
		}
	}
	if ($ii[0] < $ie[0])
	{
		while ($nn < $nofranks)
		{
			$nn = array_push( $results, $list1[ $ii[ 0]]);
			$ii[ 0] += 1;
		}
	}
	elseif ($ii[1] < $ie[1])
	{
		while ($nn < $nofranks)
		{
			$nn = array_push( $results, $list2[ $ii[ 1]]);
			$ii[ 1] += 1;
		}
	}
	return $results;
}

class QueryThread extends Thread
{
	private $rpcport;
	private $context;
	private $results;
	private $errormsg;

	public function __construct( $rpcport)
	{
		$this->rpcport = $rpcport;
	}
 
	public function run()
	{
		try
		{
			this->$context = new StrusContext( "localhost:7181" );
			this->$results = evalQuery( $context, $queryString);
		}
		catch( Exception $e)
		{
			this->$errormsg = $e->getMessage();
		}
	}

	public function getResults()
	{
		return this->$results;
	}

	public function getLastError()
	{
		return this->$errormsg;
	}
}

try {
	// Initialize query string:
	$queryString = "";
	$nofRanks = 20;
	if (PHP_SAPI == 'cli')
	{
		# ... called from command line (CLI)
		$ai = 0;
		foreach ($argv as $arg)
		{
			if ($ai > 0)
			{
				if ($queryString != "")
				{
					$queryString .= ' ';
				}
				$queryString .= $arg;
			}
			++$ai;
		}
	}
	else
	{
		# ... called from web server
		parse_str( getenv('QUERY_STRING'), $_GET);
		$queryString = $_GET['q'];
		$nofRanks = intval( $_GET['n']);
	}

	// Initialize and start the threads and evaluate the query:
	$time_start = microtime(true);
	$qrythread = [];
	foreach (range(0, 1) as $ii) {
		$qrythread[ $ii] = new QueryThread($i);
		$qrythread[ $ii]->start();
	}
	// Wait for all to finish:
	foreach (range(0, 1) as $ii) {
		$qrythread[ $ii]->join();
	}
	// Merge query results:
	$results1 = qrythread[ 0]->getResults();
	if (is_null( $results1))
	{
		$results1 = array();
		echo '<p><font color="red">Error in query to server 1: ',  qrythread[ 0]->getLastError(), '</font></p>';
	}
	$results2 = qrythread[ 1]->getResults();
	if (is_null( $results2))
	{
		$results2 = array();
		echo '<p><font color="red">Error in query to server 1: ',  qrythread[ 0]->getLastError(), '</font></p>';
	}
	$results = mergeResults( $nofRanks, $results1, $results2);
	$time_end = microtime(true);
	$query_answer_time = number_format( $time_end - $time_start, 3);

	// Display results:
	echo "<input id=\"search_input\" class=\"textinput\" type=\"text\" maxlength=\"256\" size=\"32\" name=\"q\" tabindex=\"1\" value=\"$queryString\">";
	echo '<input id="search_button" type="image" src="search_button.jpg" tabindex="2"/>';
	echo '</form>';
	echo '</div>';
	echo "<p>query answering time: $query_answer_time seconds</p>";

	foreach ($results as &$result)
	{
		$title = $result->TITLE;
		$link = strtr ($title, array (' ' => '_'));
		$content = "";
		if (property_exists( $result, 'CONTENT'))
		{
			if (is_array( $result->CONTENT))
			{
				foreach ($result->CONTENT as &$sum)
				{
					if ($content != '') 
					{
						$content .= " --- ";
					}
					$content .= $sum;
				}
			}
			else
			{
				$content = $result->CONTENT;
			}
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


