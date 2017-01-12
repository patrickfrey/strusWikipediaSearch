<?php
try
{
	$command = "query";
	$nofRanks = 6;
	$firstRank = 0;
	$scheme = NULL;
	$mode = NULL;
	$query = "";
	$restrict = NULL;

	parse_str( getenv('QUERY_STRING'), $_GET);
	if (array_key_exists( 'n', $_GET))
	{
		$nofRanks = intval( $_GET['n']);
	}
	if (array_key_exists( 'i', $_GET))
	{
		$firstRank = $_GET['i'];
	}
	if (array_key_exists( 'q', $_GET))
	{
		$query = $_GET['q'];
	}
	if (array_key_exists( 's', $_GET))
	{
		$scheme = $_GET['s'];
	}
	if (array_key_exists( 'm', $_GET))
	{
		$mode = $_GET['m'];
	}
	if (array_key_exists( 'd', $_GET))
	{
		$restrict = intval( $_GET['d']);
	}
	$service_url = 'http://demo.project-strus.net/' . $command
			. '?q=' . urlencode($query)
			. '&i=' . $firstRank
			. '&n=' . $nofRanks
			;
	if ($scheme)
	{
		$service_url .= '&s=' . urlencode($scheme);
	}
	if ($mode)
	{
		$service_url .= '&m=' . urlencode($mode);
	}
	if ($restrict)
	{
		$service_url .= '&d=' . $restrict;
	}
	$curl = curl_init( $service_url);
	curl_setopt( $curl, CURLOPT_RETURNTRANSFER, true);
	$response = curl_exec( $curl);
	if (curl_error($curl))
	{
		$response = '<html><head><title>strus error</title></head><body><p>failed to connect to server</p></html>';
	}
	else
	{
		curl_close($curl);
	}
	echo $response;
}
catch( Exception $e ) {
	echo '<html><head><title>strus error</title></head><body><p>failed to connect to server</p></html>';
}
?>
