#!/bin/sh

export SSERVER1="http://127.0.0.1:7183"
export ISERVER1="http://127.0.0.1:7184"
export ISERVER2="http://127.0.0.1:7185"
export ISERVER3="http://127.0.0.1:7186"
export ISERVER4="http://127.0.0.1:7187"
export VSERVER1="http://127.0.0.1:7191"

SCRIPTDIR="/home/patrick/github/strusWebService/client_perl"

call()
{
	cd $SCRIPTDIR;
	$0 $1 $2 $3 $4
	cd -
}

for line in `cat $1`; do
	DIR=`echo $line | perl -pe 's@/data/wikipedia/nlpxml/([0-9]*).*@$1@'`
	DOCID=`echo $line | perl -pe 's@/data/wikipedia/nlpxml/[0-9]*/(.*)$@$1@'`
	SRV=`expr $DIR % 4`
	echo "$DIR -> $SRV"
	mkdir -p doc
	if [ "x$SRV" != "x1" ]; then
		PORT=`expr $SRV + 7184`
		echo call ./getStorageDocumentFeatures.pl "http://127.0.0.1:$PORT/storage/istorage" "$DOCID" word
	fi
done

