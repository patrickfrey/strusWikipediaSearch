#!/bin/sh

BASEDIR=`dirname "$0"`
. ${BASEDIR}/servername.sh.inc

# TRANSACTION=`curl -H "Accept: text/plain" -X POST "${SERVER_ADDR}/contentstats/wikipedia/transaction"`
TRANSACTION=`strusWebServiceClient -V -A "text/plain" POST "${SERVER_ADDR}/contentstats/wikipedia/transaction"`

echo "start analyze content statistics ..."
strusWebServiceClient -A "text/plain" PUT ${TRANSACTION} "@/srv/wikipedia/doc/0000/*.xml"

echo "LINK $TRANSACTION"

