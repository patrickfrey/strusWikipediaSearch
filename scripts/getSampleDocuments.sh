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
	$1 $2 $3 $4 $5
	if [ $? -ne 0 ]; then
		exit 1;
	fi
	cd - > /dev/null
}

getFeatures()
{
	for line in `cat $1`; do
		if [ "x$line" != "x" ]; then
			DIR=`echo $line | perl -pe 's@/data/wikipedia/nlpxml/([0-9]*).*@$1@'`
			DOCID=`echo $line | perl -pe 's@/data/wikipedia/nlpxml/[0-9]*/(.*).xml$@$1@'`
			SRV=`expr $DIR % 4`
			if [ "x$SRV" = "x0" ]; then
				PORT=`expr $SRV + 7184`
				cp $line build/doc/xml/
				call ./getStorageDocumentFeatures.pl "http://127.0.0.1:$PORT/storage/istorage" "$DOCID" word
			fi
		fi
	done
}

mkdir -p build/doc/xml
getFeatures $1 | sort | uniq > build/doc/features.txt
call ./getFeatureVectors.pl "$VSERVER1/vstorage/vstorage" @`pwd`/build/doc/features.txt

