#!/bin/sh

BASEDIR=`dirname "$0"`
. ${BASEDIR}/servername.sh.inc

curl -d "@examples/contentstats.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "http://${SERVER_ADDR}/contentstats/wikipedia"
