<!DOCTYPE html>
<html>
<body>
<p>
<?php
try {
	echo "PHP info: " . phpinfo();
}
catch ( Exception $e) {
	echo 'Caught exception: ',  $e->getMessage(), "\n";
}
?>
</p>
</body>
</html> 

