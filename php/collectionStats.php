<!DOCTYPE html>
<html>
<body>

<?php
require "strus.php";

try {
	$context = new StrusContext();
	$storage = $context->createStorageClient( "path=/home/patrick/Projects/github/strusWikipediaSearch/storage");

	echo "Number of documents inserted: " . $storage->nofDocumentsInserted() . "!";
}
catch ( Exception $e) {
	echo 'Caught exception: ',  $e->getMessage(), "\n";
}
?>

</body>
</html> 

