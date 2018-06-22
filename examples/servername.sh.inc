if [ x"$SERVER_NAME" = x"" ]
then
	export SERVER_NAME="127.0.0.1";
fi
if [ x"$SERVER_ADDR" = x"" ]
then
	export SERVER_ADDR="${SERVER_NAME}:8080";
fi
