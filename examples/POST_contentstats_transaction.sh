#!/bin/sh

BASEDIR=`dirname "$0"`
. ${BASEDIR}/servername.sh.inc

TRANSACTION=`curl -H "Accept: text/plain" -X POST "${SERVER_ADDR}/contentstats/wikipedia/transaction"`
curl -d "@/srv/wikipedia/raw/wikipedia0001.xml" -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"

echo "LINK $TRANSACTION"

