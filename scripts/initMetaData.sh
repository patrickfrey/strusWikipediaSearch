#!/bin/sh

# This script assumes that the meta data table schema has an element "pageweight Float32" defined

STORAGEPATH=storage

# Initialize the link popularity weight in document meta data (element pageweight):
echo "[2.1] get the link reference statistics"
strusInspect -s "path=$STORAGEPATH" fwstats linkid > resources/linkid_list.txt
echo "[2.2] get the docno -> docid map"
strusInspect -s "path=$STORAGEPATH" attribute title > resources/docid_list.txt
echo "[2.3] calculate a map docno -> number of references to this page"
scripts/calcDocidRefs.pl resources/docid_list.txt resources/linkid_list.txt > resources/docnoref_map.txt
echo "[2.4] calculate a map docno -> link popularity weight"
scripts/calcWeights.pl resources/docnoref_map.txt 'tanh(x/50)' > resources/pageweight_map.txt
echo "[2.5] update the meta data table element pageweight with the link popularity weight"
strusUpdateStorage -s "path=$STORAGEPATH" -m pageweight resources/pageweight_map.txt


