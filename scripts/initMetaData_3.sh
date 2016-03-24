#!/bin/sh

# This script assumes that the meta data table schema has an element "pageweight Float32" defined

STORAGEPATH=storage

# Initialize the link popularity weight in document meta data (element pageweight):
echo "[2.1] get the link reference statistics"
truncate -s 0 resources/linkid_list.txt
for ii in 1 2 3
do
	strusInspect -s "path=$STORAGEPATH$ii" fwstats linkid >> resources/linkid_list.txt
	echo "[2.2] get the docno -> docid map"
	strusInspect -s "path=$STORAGEPATH$ii" attribute docid | strusAnalyzePhrase -n "text" -q '' - > resources/docid_list$ii.txt
done
for ii in 1 2 3
do
	echo "[2.3] calculate a map docno -> number of references to this page"
	scripts/calcDocidRefs.pl resources/docid_list$ii.txt resources/linkid_list.txt > resources/docnoref_map$ii.txt
	echo "[2.4] calculate a map docno -> link popularity weight"
	scripts/calcWeights.pl resources/docnoref_map$ii.txt 'tanh(x/100)' > resources/pageweight_map$ii.txt
	echo "[2.5] update the meta data table element pageweight with the link popularity weight"
	strusUpdateStorage -s "path=$STORAGEPATH$ii" -m pageweight resources/pageweight_map$ii.txt
done



