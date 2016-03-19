#!/bin/sh

for pp in A B
do
	strusDumpStorage -P r -s path=storage | scripts/getRandomQueries.pl 200 BM25pff $pp > ./stress$pp.sh
	chmod +x ./stress$pp.sh
done



