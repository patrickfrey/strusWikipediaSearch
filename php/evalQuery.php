<!DOCTYPE html>
<html>
<head>
<title>Search Wikipedia with Strus</title>
<link href="strus.css" rel="stylesheet" type="text/css">
</head>
<body>

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
	$sumMatch->defineParameter( "phraselen", 10);
	$sumMatch->defineParameter( "sumlen", 30);
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
		print( "PUSH TERM " . $term->type . " '" . $term->value . "'\n");
		$query->pushTerm( $term->type, $term->value);
		$query->defineFeature( "weighted");
	}

	$query->setMaxNofRanks( 10);
	$query->setMinRank( 0);

	return $query->evaluate();
}

try {
	$_GET=parse_str(getenv('QUERY_STRING'));
	# $querystring=$_GET['q'];
	$queryString="agriculture";

	$context = new StrusContext( "localhost:7181" );
	$storage = $context->createStorageClient( "" );

	$results = evalQuery( $context, $queryString);

	echo '<div id="search_ranklist">';
	foreach ($results as &$result)
	{
		$content = "";
		foreach ($result->CONTENT as &$sum)
		{
			$content .= "..." . $sum;
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
catch ( LogicException $e) {
	echo '<p>';
	echo '<font color="red">';
	echo 'Caught exception: ',  $e->getMessage(), "\n";
	echo '</font>';
	echo '</p>';
}
catch ( Exception $e) {
	echo '<p>';
	echo '<font color="red">';
	echo 'Caught exception: ',  $e->getMessage(), "\n";
	echo '</font>';
	echo '</p>';
}
?>

</body>
</html> 


