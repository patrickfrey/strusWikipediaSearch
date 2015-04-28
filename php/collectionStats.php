<!DOCTYPE html>
<html>
<body>

<?php
require "strus.php";

try {
	$context = new StrusContext( "localhost:7181" );
	$storage = $context->createStorageClient( "" );

	echo '<p>';
	echo "Number of documents inserted: " . $storage->nofDocumentsInserted() . "!";
	echo '</p>';
}
catch ( Exception $e) {
	echo '<p><font color="red">Caught exception: ',  $e->getMessage()</font></p>, "\n";
}
?>

</body>
</html> 

