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
  <img style="display:block;" width="100%" src="strus_logo.jpg" alt="strus logo">
<!-- Copyright: <a href='http://www.123rf.com/profile_guarding123'>guarding123 / 123RF Stock Photo</a>
-->
 </a>
</div>
<form name="search" class method="GET" action="evalQuery.php">

<?php
require "strus.php";

function evalQuery( $context, $queryString, $nofRanks, $minRank)
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

	$queryeval->addWeightingFunction( 1.0, "BM25_dpfc", [
			"k1" => 0.75, "b" => 2.1, "avgdoclen" => 500,
			"doclen_title" => "doclen_tist", "titleinc" => 2.0,
			".match" => "docfeat", ".title" => "title" ]);

	$queryeval->addWeightingFunction( 1.0, "metadata", [ "name" => "pageweight" ] );

	$queryeval->addSummarizer( "TITLE", "attribute", [ "name" => "title" ] );
	$queryeval->addSummarizer( "CONTENT", "matchphrase", [
			"type" => "orig", "len" => 80, "nof" => 3, "structseek" => 30,
			".struct" => "sentence", ".match" => "docfeat" ] );

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
	$query->setMinRank( $minRank);

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
			$w2 = $list2[ $ii[ 1]]->weight;
			if ($w1 < $w2)
			{
				$results[] = $list1[ $ii[ 0]];
				$ii[ 0] += 1;
				if ($ii[0] >= $ie[0])
				{
					break;
				}
				$nn += 1;
			}
			elseif ($w1 > $w2)
			{
				$results[] = $list2[ $ii[ 1]];
				$ii[ 1] += 1;
				if ($ii[1] >= $ie[1])
				{
					break;
				}
				$nn += 1;
			}
			else
			{
				$results[] = $list1[ $ii[ 0]];
				$results[] = $list2[ $ii[ 1]];
				$ii[ 0] += 1;
				$ii[ 1] += 1;
				if ($ii[0] >= $ie[0] || $ii[1] >= $ie[1])
				{
					break;
				}
				$nn += 2;
			}
		}
	}
	if ($ii[0] < $ie[0])
	{
		while ($nn < $nofranks)
		{
			$results[] = $list1[ $ii[ 0]];
			$ii[ 0] += 1;
			if ($ii[0] >= $ie[0])
			{
				break;
			}
			$nn += 1;
		}
	}
	elseif ($ii[1] < $ie[1])
	{
		while ($nn < $nofranks)
		{
			$results[] = $list2[ $ii[ 1]];
			$ii[ 1] += 1;
			if ($ii[1] >= $ie[1])
			{
				break;
			}
			$nn += 1;
		}
	}
	return $results;
}

class QueryThread extends Thread
{
	private $service;
	private $context;
	private $querystring;
	private $minrank
	private $nofranks;
	private $results;
	private $errormsg;

	public function __construct( $service, $querystring, $minrank, $nofranks)
	{
		$this->service = $service;
		$this->querystring = $querystring;
		$this->nofranks = $nofranks;
		$this->minrank = $minrank;
	}
 
	public function run()
	{
		try
		{
			$context = new StrusContext( $this->service);
			$this->results = evalQuery( $context, $this->querystring, $this->minrank, $this->nofranks);
		}
		catch( Exception $e)
		{
			$this->errormsg = $e->getMessage();
		}
	}

	public function getResults()
	{
		return $this->results;
	}

	public function getLastError()
	{
		return $this->errormsg;
	}
}

try {
	// Initialize query string:
	$queryString = "";
	$nofRanks = 20;
	$minRank = 0;
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
		if (array_key_exists( 'n', $_GET))
		{
			$nofRanks = intval( $_GET['n']);
		}
		if (array_key_exists( 'i', $_GET))
		{
			$minRank = intval( $_GET['i']);
		}
	}

	// Initialize and start the threads and evaluate the query:
	$time_start = microtime(true);
	$qrythread = [];
	$server = array( "localhost:7181", "localhost:7182");

	foreach (range(0, 1) as $ii)
	{
		$qrythread[ $ii] = new QueryThread( $server[ $ii], $queryString, $minRank, $nofRanks);
		$qrythread[ $ii]->start();
	}
	// Wait for all to finish:
	foreach (range(0, 1) as $ii)
	{
		$qrythread[ $ii]->join();
	}
	// Merge query results:
	$results1 = $qrythread[ 0]->getResults();
	if (is_null( $results1))
	{
		$results1 = array();
		echo '<p><font color="red">Error in query to server 1: ',  $qrythread[ 0]->getLastError(), '</font></p>';
	}
	$results2 = $qrythread[ 1]->getResults();
	if (is_null( $results2))
	{
		$results2 = array();
		echo '<p><font color="red">Error in query to server 2: ',  $qrythread[ 1]->getLastError(), '</font></p>';
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


