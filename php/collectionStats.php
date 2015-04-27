<!DOCTYPE html>
<html>
<body>

<?php
require "strus.php";

try {
	$context = new StrusContext( "localhost:7181" );
	$storage = $context->createStorageClient( "" );

	echo "Number of documents inserted: " . $storage->nofDocumentsInserted() . "!";
}
catch ( Exception $e) {
	echo 'Caught exception: ',  $e->getMessage(), "\n";
}
?>

</body>
</html> 

