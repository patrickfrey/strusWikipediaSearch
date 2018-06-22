#!/bin/sh

. servername.sh.inc
curl -i -H "Accept: text/plain" -X POST "${SERVER_ADDR}/contentstats/wikipedia/transaction"

