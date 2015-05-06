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

function evalQuery( $context, $queryString)
{
	$storage = $context->createStorageClient( "" );
	$analyzer = $context->createQueryAnalyzer();
	$queryeval = $context->createQueryEval();

	$weightingBM25 = new WeightingFunction( "BM25");
	$weightingBM25->defineParameter( "k1", 0.75);
	$weightingBM25->defineParameter( "b", 2.1);
	$weightingBM25->defineParameter( "avgdoclen", 10000);

	$sumTitle = new Summarizer( "attribute");
	$sumTitle->defineParameter( "name", "title");

	$sumMatch = new Summarizer( "matchphrase");
	$sumMatch->defineParameter( "type", "orig");
	$sumMatch->defineParameter( "len", 90);
	$sumMatch->defineParameter( "nof", 3);
	$sumMatch->defineParameter( "structseek", 40);
	$sumMatch->defineFeature( "struct", "sentence");
	$sumMatch->defineFeature( "match", "weighted");

	$queryeval->addWeightingFunction( $weightingBM25, array( "weighted"));
	$queryeval->addSummarizer( "TITLE", $sumTitle);
	$queryeval->addSummarizer( "CONTENT", $sumMatch);

	$queryeval->addSelectionFeature( "weighted");

	$query = $queryeval->createQuery( $storage);
	$analyzer->definePhraseType( "text", "stem", "word", 
			["lc",
			["dictmap", "irregular_verbs_en.txt"],
			["stem", "en"],
			["convdia", "en"],
			"lc"]);

	$terms = $analyzer->analyzePhrase( "text", $queryString);
	foreach ($terms as &$term)
	{
		$query->pushTerm( $term->type, $term->value);
		$query->defineFeature( "weighted");
	}

	$query->setMaxNofRanks( 10);
	$query->setMinRank( 0);

	return $query->evaluate();
}

try {
	$queryString = "";
	if (PHP_SAPI == 'cli')
	{
		# ... called from command line (CLI)
		foreach ($argv as $arg)
		{
			if ($queryString != "")
			{
				$queryString .= ' ';
			}
			$queryString .= $arg;
		}
	}
	else
	{
		# ... called from web server
		parse_str( getenv('QUERY_STRING'), $_GET);
		$queryString = $_GET['q'];
	}

	echo "<input id=\"search_input\" class=\"textinput\" type=\"text\" maxlength=\"256\" size=\"32\" name=\"q\" tabindex=\"1\" value=\"$queryString\">";
	echo '<input id="search_button" type="image" src="search_button.jpg" tabindex="2"/>';
	echo '</form>';
	echo '</div>';

	$context = new StrusContext( "localhost:7181" );
	$storage = $context->createStorageClient( "" );

	$results = evalQuery( $context, $queryString);

	echo '<div id="search_ranklist">';
	foreach ($results as &$result)
	{
		$content = "";
		if (property_exists( $result, 'CONTENT'))
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
		echo '<div id="search_rank">';
			echo '<div id="rank_docno">' . "$result->docno</div>";
			echo '<div id="rank_weight">' . number_format( $result->weight, 4) . "</div>";
			echo '<div id="rank_content">';
				echo '<div id="rank_title">' . "$result->TITLE</div>";
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


