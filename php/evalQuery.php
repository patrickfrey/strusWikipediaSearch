<!DOCTYPE html>
<html>
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

	$sumTitle = new Summarizer( "title");
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

	$query = new Query( $queryEval, $storage);
	$analyzer->definePhraseType(
			"text", "stem", Tokenizer( "word"), 
			array(
				Normalizer( "lc"),
				Normalizer( "dictmap", "irregular_verbs_en.txt"),
				Normalizer( "stem", "en"),
				Normalizer( "convdia", "en"),
				Normalizer( "lc")));

	$terms = $analyzer->analyzePhrase( "text", $queryString);
	foreach ($terms as &$term)
	{
		$query->pushTerm( $term->type(), $term->value());
		$query->defineFeature( "stem");
	}

	$query->setMaxNofRanks( 10);
	$query->setMinRank( 0);

	return $query->evaluate();
}

try {
	$_GET=parse_str(getenv('QUERY_STRING'));
	# $querystring=$_GET['q'];
	$queryString="q=agriculture";

	$context = new StrusContext( "localhost:7181" );
	$storage = $context->createStorageClient( "" );

	$results = evalQuery( $context, $queryString);

	echo '<ol>';
	foreach ($results as &$result)
	{
		$title = "";
		$content = "";
		foreach ($result->attributes() as &$attribute)
		{
			if ($attribute->name() == "TITLE")
			{
				$title .= $attribute->value();
			}
			if ($attribute->name() == "CONTENT")
			{
				if ($content != "")
				{
					$content .= " // ";
				}
				$content .= $attribute->value();
			}
		}
		echo '<li>';
		echo '<p>docno=' . $rank->docno()
			. ', weight=' . $rank->weight()
			. ', title=' . $title
			. ', content=' . $content . '</p>' ."\n";
		echo '<li>';
	}
	echo '</ol>';
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


