#!/bin/sh

echo "--- Analyze document contents ---"
TRANSACTION=`curl -H "Accept: text/plain" -X POST "${SERVER_ADDR}/contentstats/wikipedia/transaction"`
cd /srv/wikipedia/doc
# for ii in 0 1 2 3 4 5 6 7 8 9; do
for ii in 0; do
	for xx in `ls -d "$ii*"`; do
		for dd in `find /srv/wikipedia/doc/$xx -name "*.xml"`; do
			echo "--- Collect statistics $xx ---"
			curl -d "@$dd" -i -H "Accept: text/plain" -H "Content-Type: application/xml; charset=UTF-8" -X PUT "$TRANSACTION"
		done
	done
done
echo $TRANSACTION

